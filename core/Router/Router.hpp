#pragma once

#include "suRouterAuth.hpp"
#include <unistd.h>
#include "Uds_Server.h"
#include "Router_State.hpp"
#include "Router_ErrorCode.h"
#include "Router_Heartbeat.hpp"
#include "Uds_MsgBuilder.hpp"
#include "Uds_MsgParser.hpp"
#include "Uds_RouterMsg_fromRouterBuilder.hpp"
#include "Uds_RouterMsg_toRouterParser.hpp"
#include "Uds_GeneralMsg_fromMainParser.hpp"
#include "Uds_GeneralMsg_toMainBuilder.hpp"
#include "Uds_GeneralMsg_toMainParser.hpp"
#include "suRuntime.hpp"
#include "SuOS_Config.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace SuOS::Uds::Router {

    class RouterManager : public std::enable_shared_from_this<RouterManager> {
    public:
        //using OnClientStatus = std::function<void(uint32_t client_id, bool online)>;

        RouterManager(std::shared_ptr<SuOS::Runtime::suRuntime> runtime)
            : _runtime(runtime), 
              _builder(runtime),
              _parser(runtime),
              _router_msg_builder(runtime),
              _router_msg_parser(runtime, {
                  [this](uint32_t usr, uint32_t part, uint32_t app_id) {
                      this->handle_enable_app_register(usr, part, app_id);
                  }
              }),
              _general_msg_parser(runtime, {
                  [this](uint32_t usr, uint32_t part) { this->handle_stop_request(usr, part); },
                  [this](uint32_t usr, uint32_t part) { this->handle_stop_kill(usr, part); }
              }),
              _general_msg_builder(runtime),
              _heartbeat(runtime, 
                [this](uint32_t tid, const std::vector<uint8_t>& p) { this->raw_send(tid, p); },
                [this](uint32_t cid) { this->handle_client_timeout(cid); }) {}

        uint32_t Stop() {
            if (!_runtime->isInEventLoop()) return outputErrorCode::NotInEventLoop;
            _heartbeat.stopAll();
            if (_server) {
                _server->stop();
                _server.reset();
            }
            _state.setState(State::Stopped);
            return outputErrorCode::OK;
        }

        uint32_t Start() {
            if (!_runtime->isInEventLoop()) return outputErrorCode::NotInEventLoop;
            
            auto currentState = _state.getState();
            // if (currentState == State::Stopped) return outputErrorCode::UnknownError; 
            if (currentState == State::Running) return outputErrorCode::OK;

            _server = std::make_shared<SuOS::Uds::Server::Uds_Server>(
                _runtime->getIoContext(),
                SuOS::Uds::ClientMgr::Df::uds_path, // UDS Path                 // Timeout
                std::bind(&RouterManager::on_uds_message, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&RouterManager::on_uds_error, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                std::bind(&RouterManager::on_uds_connect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
            );

            _server->start();

            _state.setState(State::Running);
            return outputErrorCode::OK;
        }

        // 核心转发接口
        void route_message(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_usr, uint32_t receiver_part, const std::vector<uint8_t>& payload) {
            _runtime->dispatch([this, sender_usr, sender_part, receiver_usr, receiver_part, payload]() {
                if (receiver_usr != SuOS::Config::Usr::ROUTER && sender_usr != SuOS::Config::Usr::ROUTER) {
                    if (!_auth.authorizeMessage(sender_usr, receiver_usr)) {
                        std::cerr << "Router Msg Auth Failed: " << sender_usr << " -> " << receiver_usr << std::endl;
                        return;
                    }
                }
                
                auto guard = _builder.finalizeEnvelope(sender_part, receiver_usr, receiver_part, payload, sender_usr);
                std::vector<char> data(guard.data(), guard.data() + guard.size());
                
                if (receiver_usr == SuOS::Config::Usr::ROUTER) {
                    // 发给自己的消息（如心跳响应）内部消化
                    if (receiver_part == SuOS::Config::Part::HEARTBEAT) {
                        _heartbeat.updateClient(sender_usr);
                    }

                    if (receiver_part == SuOS::Config::Part::ROUTER) {
                        // 处理来自 Main 或其他授权组件的 Router 命令
                        _router_msg_parser.Parse(payload.data(), payload.size(), sender_usr, sender_part);
                    }

                    if (receiver_part == SuOS::Config::Part::USR_CONTROL) {
                        // 处理通用控制消息 (例如来自 Main 的 STOP 指令)
                        _general_msg_parser.Parse(payload.data(), payload.size(), sender_usr, sender_part);
                    }
                } else {
                    // 转发给目标 Client
                    _server->send_async(receiver_usr, data);
                }
            });
        }

    private:
        // 1. 认证回调
        SuOS::Uds::Server::Uds_Server::accept_result on_uds_connect(int pid, int uid, int gid) {
            uint32_t assigned_id = 0;
            bool authorized = perform_auth(pid, uid, gid, assigned_id);

            if (authorized) {
                // 1. 如果 Main 还没连上，只允许 MAIN ID (1001) 的连接
                if (!_main_online && assigned_id != SuOS::Config::Usr::MAIN) {
                    std::cerr << "Router: Rejected connection from " << assigned_id << " because Main is offline." << std::endl;
                    return {false, 0};
                }

                std::cout << "Client Authorized: PID=" << pid << " ID=" << assigned_id << std::endl;
                
                if (assigned_id == SuOS::Config::Usr::MAIN) {
                    _main_online = true;
                }

                if (!_heartbeat_started) {
                    _heartbeat.startBroadcast();
                    _heartbeat_started = true;
                }

                _heartbeat.updateClient(assigned_id);

                // 报告给 Main 模块
                report_to_main(assigned_id, true);
            }
            return {authorized, assigned_id};
        }

        // 2. 消息处理
        void on_uds_message(uint32_t cid, const std::vector<char> data) {
            auto guard = _parser.ParseMsg(reinterpret_cast<const uint8_t*>(data.data()), data.size());
            if (!guard.isValid()) return;

            // 关键安全检查：验证发送者声明的 ID 是否与 UDS 连接绑定的 ID 一致
            if (guard.getSenderUsr() != cid) {
                std::cerr << "Security Alert: Spoofed sender ID from client " << cid << std::endl;
                return;
            }

            uint32_t target_usr = guard.getReceiverUsr();
            uint32_t target_part = guard.getReceiverPart();
            uint32_t sender_usr = guard.getSenderUsr();
            uint32_t sender_part = guard.getSenderPart();
            
            // 数据提取
            std::vector<uint8_t> payload(guard.payloadData(), guard.payloadData() + guard.payloadSize());

            route_message(sender_usr, sender_part, target_usr, target_part, payload);
        }

        // 3. 认证逻辑
        bool perform_auth(int pid, int uid, int gid, uint32_t& out_id) {
            std::string execPath;
            char buf[512];
            ssize_t len = readlink(("/proc/" + std::to_string(pid) + "/exe").c_str(), buf, sizeof(buf) - 1);
            if (len != -1) {
                buf[len] = '\0';
                execPath = std::string(buf);
            } else {
                std::cerr << "Router: Failed to readlink for PID " << pid << std::endl;
                return false;
            }

            int auth_id = _auth.authenticateConnection(pid, uid, gid, execPath);
            if (auth_id == -1) return false;
            
            out_id = static_cast<uint32_t>(auth_id);
            return true; 
        }

        void report_to_main(uint32_t client_id, bool online) {
            // 使用 RouterMsg_fromRouterBuilder 构建内部负载
            if (online) {
                auto inner = _router_msg_builder.BuildonConnect(client_id);
                std::vector<uint8_t> payload(inner.data(), inner.data() + inner.size());
                route_message(SuOS::Config::Usr::ROUTER, SuOS::Config::Part::ROUTER, 
                             SuOS::Config::Usr::MAIN, SuOS::Config::Part::USR_REPORT, payload);
            } else {
                auto inner = _router_msg_builder.BuildonConnectionLost(client_id);
                std::vector<uint8_t> payload(inner.data(), inner.data() + inner.size());
                route_message(SuOS::Config::Usr::ROUTER, SuOS::Config::Part::ROUTER, 
                             SuOS::Config::Usr::MAIN, SuOS::Config::Part::USR_REPORT, payload);
            }
        }

        void handle_client_timeout(uint32_t cid) {
            std::cout << "Client " << cid << " heartbeat timeout. Closing." << std::endl;
            _server->close(cid);
            process_disconnect(cid);
        }

        void on_uds_error(const uint32_t cid, uint32_t error_type, std::string message) {
            std::cerr << "UDS Error on client " << cid << " [" << error_type << "]: " << message << std::endl;
            // 构造错误消息发送给 Main
            auto guard = _general_msg_builder.BuildonUsrError(error_type, message);
            std::vector<uint8_t> payload(guard.data(), guard.data() + guard.size());
            route_message(SuOS::Config::Usr::ROUTER, SuOS::Config::Part::ROUTER, 
                         SuOS::Config::Usr::MAIN, SuOS::Config::Part::USR_REPORT, payload);

            process_disconnect(cid);
        }

        void process_disconnect(uint32_t cid) {
            _heartbeat.stopClient(cid);
            _auth.removeEntity(cid);
            report_to_main(cid, false);
            
            if (cid == SuOS::Config::Usr::MAIN) {
                std::cout << "Router: Main disconnected. Stopping Router Service." << std::endl;
                _main_online = false;
                Stop();
            }
        }

        // 辅助：内部发送//用于心跳
        void raw_send(uint32_t target_id, const std::vector<uint8_t>& payload) {
            auto guard = _builder.finalizeEnvelope(SuOS::Config::Part::HEARTBEAT, target_id, SuOS::Config::Part::HEARTBEAT, payload);
            _server->send_async(target_id, std::vector<char>(guard.data(), guard.data() + guard.size()));
        }

        // 处理对象router的msg
        void handle_enable_app_register(uint32_t sender_usr, uint32_t sender_part, uint32_t app_id) {
            // 权限检查：只有 Main 有权执行此操作
            if (sender_usr != SuOS::Config::Usr::MAIN) return;
            
            std::cout << "Router: Enabling App Register for AppID=" << app_id << std::endl;
            _auth.updateAppAllowedState(app_id, true);
        }

        void handle_stop_request(uint32_t usr, uint32_t part) {
            if (usr == SuOS::Config::Usr::MAIN) {
                std::cout << "Router: Received Stop Request from Main." << std::endl;
                Stop();
            }
        }

        void handle_stop_kill(uint32_t usr, uint32_t part) {
            if (usr == SuOS::Config::Usr::MAIN) {
                std::cout << "Router: Received Stop Kill from Main. Exiting." << std::endl;
                Stop();
                // 在异步环境中通常由外部管理生命周期，此处仅停止服务
            }
        }

        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        std::shared_ptr<SuOS::Uds::Server::Uds_Server> _server;
        RouterStateMgr _state;
        Router_Heartbeat _heartbeat;
        SuOS::Uds::Msg::MessageBuilder _builder;
        SuOS::Uds::Msg::MessageParser _parser;
        SuOS::Uds::Msg::Router::RouterMsg_fromRouterBuilder _router_msg_builder;
        SuOS::Uds::Msg::Router::RouterMsg_toRouterParser _router_msg_parser;
        SuOS::Uds::Msg::General::GeneralMsg_fromMainParser _general_msg_parser;
        SuOS::Uds::Msg::General::GeneralMsg_toMainBuilder _general_msg_builder;
        SuOS::Router::RouterAuthManager _auth;
        bool _heartbeat_started = false;
        bool _main_online = false;
    };
}