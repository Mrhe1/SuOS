#pragma once
#include "Uds_ClientMgr.hpp"
#include "suRuntime.hpp"
#include "sysService_df.h"
#include "SuOS_Config.h"
#include <memory>
#include <string>

namespace SuOS::Service {

    class SysService : public std::enable_shared_from_this<SysService> {
    public:
        SysService(int myusr_id, std::shared_ptr<SuOS::Runtime::suRuntime> runtime)
            : _myusr_id(myusr_id), _runtime(runtime) {}

        virtual ~SysService() = default;

        // 【启动入口】由 Main 函数调用
        void start() {
            // 1. 先在线程池中执行耗时的硬件/资源准备工作
            _runtime->execute([this, self = shared_from_this()]() {
                onStart();
            });
            // 2. 准备工作完成后，回到 EventLoop 启动 UDS 连接
            _runtime->dispatch([this]() {
                initClientMgr();
            });
        }

        // 【主动停止】
        void stop(uint32_t cause) {
            if (_clientMgr) {
                _clientMgr->Stop();
            }
            onStop(cause);
        }

    protected:
        // --- 子类需实现的业务钩子 ---
        
        // 耗时初始化： (在线程池跑)
        virtual void onStart() = 0;
        
        // 连接建立： (在 EventLoop 跑)
        virtual void onConnect() = 0;
        
        // 消息分发：解析第二层 FlatBuffers 载荷 (在 EventLoop 跑)
        virtual void onMessage(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_part,
                               uint32_t cmd_id, const std::vector<uint8_t>& payload) = 0;
        
        // stop// 因为内部问题，或者main要求
        virtual void onStop(uint32_t cause) = 0;

        // 发送消息便捷接口（封装 ClientMgr）
        void sendMsg(uint32_t sender_part, uint32_t target_usr, uint32_t target_part, 
                          uint32_t cmd_id, const std::vector<uint8_t>& payload) {
            if (_clientMgr) {
                _clientMgr->sendmsg(sender_part, target_usr, target_part, cmd_id, payload);
            }
        }

    private:
        void initClientMgr() {
            // 构造 ClientManager
            // 假设 ClientManager 内部在连接成功后会有回调
            _clientMgr = std::make_shared<SuOS::Uds::ClientMgr::ClientManager>(
                _myusr_id, 
                _runtime,
                [this](uint32_t err, std::string msg) { handleInternalError(err, msg); },
                [this](uint32_t s_usr, uint32_t s_part, uint32_t r_part, uint32_t cmd, const std::vector<uint8_t>& p) {
                    // 处理进程控制指令
                    if (r_part == SuOS::Config::Part::Usr_Control) {
                        if (cmd == SuOS::Config::CommandId::usr_stop_request) {
                            // 停止// 正常停止
                            stop(SuOS::Service::Df::stopCause::normal);
                        }
                        if (cmd == SuOS::Config::CommandId::usr_stop_kill) {
                            // 强制停止
                            stop(SuOS::Service::Df::stopCause::force);
                        }
                        return;
                    }
                    // 业务消息丢给子类
                    onMessage(s_usr, s_part, r_part, cmd, p);
                },
                [this]() {
                    _runtime->dispatch([this]() {
                        // 触发 onConnect
                        onConnect();
                    });
                }
            );
            _clientMgr->Start();
        }

        void handleInternalError(uint32_t error_type, std::string msg) {
            // 统一错误处理触发 onStop
            if (error_type != SuOS::Uds::ClientMgr::outputErrorCode::UdsClientOK) {
                stop(SuOS::Service::Df::stopCause::uds_error);
            }
        }

    protected:
        int _myusr_id;
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        std::shared_ptr<SuOS::Uds::ClientMgr::ClientManager> _clientMgr;
    };
} // namespace SuOS::Service