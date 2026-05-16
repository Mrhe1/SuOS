#pragma once

#include "Uds_Server.h"
#include "Router_State.hpp"
#include "Router_ErrorCode.h"
#include "Router_Heartbeat.hpp"
#include "Uds_MsgBuilder.hpp"
#include "Uds_MsgParser.hpp"
#include "Uds_RouterMsgBuilder.hpp"
#include "Uds_RouterMsgParser.hpp"
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
        using OnClientStatus = std::function<void(uint32_t client_id, bool online)>;

        RouterManager(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, OnClientStatus status_cb)
            : _runtime(runtime), 
              _status_cb(status_cb),
              _builder(runtime),
              _parser(runtime),
              _router_msg_builder(runtime),
              _heartbeat(runtime, 
                [this](uint32_t tid, const std::vector<uint8_t>& p) { this->raw_send(tid, p); },
                [this](uint32_t cid) { this->handle_client_timeout(cid); }) 
        {}

        uint32_t Start() {
            if (!_runtime->isInEventLoop()) return outputErrorCode::NotInEventLoop;
            
            _server = std::make_shared<SuOS::Uds::Server::Uds_Server>(
                _runtime->getIoContext(),
                "/tmp/suUDS.sock", // UDS Path
                5,                 // Timeout
                std::bind(&RouterManager::on_uds_message, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&RouterManager::on_uds_error, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                std::bind(&RouterManager::on_uds_connect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
            );

            _server->start();
            _heartbeat.startBroadcast([]() -> const std::vector<uint32_t>& {
                // 这里需要一种方式获取当前所有ID，见下文实现
                static std::vector<uint32_t> ids; 
                return ids; // 简化处理，实际开发中需从session map获取
            });

            _state.setState(State::Running);
            return outputErrorCode::OK;
        }

        // 核心转发接口
        void route_message(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_usr, uint32_t receiver_part, const std::vector<uint8_t>& payload) {
            _runtime->dispatch([this, sender_usr, sender_part, receiver_usr, receiver_part, payload]() {
                auto guard = _builder.finalizeEnvelope(sender_part, receiver_usr, receiver_part, payload, sender_usr);
                std::vector<char> data(guard.data(), guard.data() + guard.size());
                
                if (receiver_usr == SuOS::Config::Usr::ROUTER) {
                    // 发给自己的消息（如心跳响应）内部消化
                    if (receiver_part == SuOS::Config::Part::HEARTBEAT) {
                        _heartbeat.updateClient(sender_usr);
                    }

                    if (receiver_part == SuOS::Config::Part::ROUTER) {
                        // 处理来自 Main 的控制命令（如强制断开某个客户端）
                        handle_router_msg(data);
                    }

                    if (receiver_part == SuOS::Config::Part::Usr_Control) {
                        // 处理控制信息
                        // 暂时不用
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
                std::cout << "Client Authorized: PID=" << pid << " ID=" << assigned_id << std::endl;
                // 报告给 Main 模块
                report_to_main(assigned_id, true);
                if (_status_cb) _status_cb(assigned_id, true);
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

        // 3. 认证逻辑（占位）
        bool perform_auth(int pid, int uid, int gid, uint32_t& out_id) {
            // 这里对接你未来的 Auth Class
            // 假设分配规则：1000 + pid
            out_id = 1000 + pid; 
            return true; 
        }

        void report_to_main(uint32_t client_id, bool online) {
            // 向特殊 ID (MAIN) 发送报告
            std::vector<uint8_t> report_data = { online ? (uint8_t)1 : (uint8_t)0 };
            route_message(SuOS::Config::Usr::ROUTER, SuOS::Config::Usr::MAIN, report_data);
        }

        void handle_client_timeout(uint32_t cid) {
            std::cout << "Client " << cid << " heartbeat timeout. Closing." << std::endl;
            _server->close(cid);
            report_to_main(cid, false);
            if (_status_cb) _status_cb(cid, false);
        }

        void on_uds_error(const uint32_t cid, uint32_t error_type, std::string message) {
            _heartbeat.stopClient(cid);
            if (_status_cb) _status_cb(cid, false);
        }

        // 辅助：内部发送
        void raw_send(uint32_t target_id, const std::vector<uint8_t>& payload) {
            auto guard = _builder.finalizeEnvelope(SuOS::Config::Part::Heartbeat, target_id, SuOS::Config::Part::Heartbeat, payload);
            _server->send_async(target_id, std::vector<char>(guard.data(), guard.data() + guard.size()));
        }

        // 处理对象router的msg
        void handle_router_msg(std::vector<char> data) {
            // 这里可以处理来自 Main 的控制命令
            auto guard = _parser.ParseMsg(reinterpret_cast<const uint8_t*>(data.data()), data.size());

        }

        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        std::shared_ptr<SuOS::Uds::Server::Uds_Server> _server;
        RouterStateMgr _state;
        Router_Heartbeat _heartbeat;
        SuOS::Uds::Msg::MessageBuilder _builder;
        SuOS::Uds::Msg::MessageParser _parser;
        SuOS::Uds::Msg::Router::RouterMsgBuilder _router_msg_builder;
        OnClientStatus _status_cb;
    };
}