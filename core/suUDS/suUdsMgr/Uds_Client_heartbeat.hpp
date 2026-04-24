#pragma once
#include "suRuntime.hpp"
#include "Uds_df.h"
#include "Uds_MsgBuilder.hpp"
#include <sys/socket.h>
#include "SuOS_Config.h"


namespace SuOS::Uds::ClientMgr {
    class Uds_Client_heartbeat {
        using sendmsg = std::function<void(uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, uint32_t cmd_id, const std::vector<uint8_t>& sub_payload)>;
        public:
            Uds_Client_heartbeat(sendmsg send_msg, std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _send_msg(send_msg), _runtime(runtime) {}
            void start() {
                _heartbeat_task = _runtime->scheduleAtFixedRate(_heartbeat_interval, _heartbeat_interval, [this]() {
                    // 构造心跳消息
                    _send_msg(_heartbeat_part, _router_id, _heartbeat_part, _heartbeat_id, std::vector<uint8_t>());
                    // 发送心跳消息到路由器
                    // 这里可以使用之前定义的 MessageBuilder 来构造消息
                }, false);
            }

            void stop() {
                if (_heartbeat_task) {
                    _heartbeat_task->cancel();
                    _heartbeat_task = nullptr;
                }
            }
        private:
            sendmsg _send_msg;
            std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
            //int _my_id;
            const int _heartbeat_interval = SuOS::Uds::Df::heartbeat_interval;
            const uint32_t _heartbeat_id = SuOS::Config::CommandId::heartbeat_id;
            const uint32_t _heartbeat_part = SuOS::Config::Part::Heartbeat;
            const uint32_t _router_id = SuOS::Config::Usr::ROUTER;
            std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _heartbeat_task = nullptr;
    };
}