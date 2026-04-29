#pragma once

#include "suRuntime.hpp"
#include "Uds_df.h"
#include "Uds_MsgBuilder.hpp"
#include <sys/socket.h>
#include "SuOS_Config.h"
#include <chrono>


namespace SuOS::Uds::ClientMgr {
    // 只被动接受心跳->进行超时检查
    // 主动发送->用于router检测
    // 两者无直接关系
    class Uds_Client_heartbeat {
        using sendmsg = std::function<void(uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, uint32_t cmd_id, const std::vector<uint8_t>& sub_payload)>;

        using onConnectLost = std::function<void()>;

        public:
            Uds_Client_heartbeat(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, sendmsg send_msg, onConnectLost onConnect_lost) :
              _runtime(runtime), _send_msg(send_msg), _onConnectLost(onConnect_lost) {}
            void start() {
                // 主动发送
                _heartbeat_task = _runtime->scheduleAtFixedRate(100, _heartbeat_interval, [this]() {
                            _send_msg(_heartbeat_part, _router_id, _heartbeat_part, _heartbeat_id, std::vector<uint8_t>());
                        }, false);

                _overTime_task = _runtime->scheduleAtFixedRate(_heartbeat_timeout, _heartbeat_timeout, [this]() {
                            stop();
                            onConnectLost();
                        }, 1, false);
            }

            void stop() {
                if (_heartbeat_task) {
                    _heartbeat_task->cancel();
                    _heartbeat_task = nullptr;
                }
                if (_overTime_task) {
                    _overTime_task->cancel();
                    _overTime_task = nullptr;
                }
            }

            void onMsg(uint32_t sender_usr, uint32_t sender_part,
                    uint32_t receiver_part, uint32_t cmd_id, const std::vector<uint8_t>& sub_payload) {
                    if (sender_part == _heartbeat_part && sender_usr == _router_id && 
                        cmd_id == SuOS::Config::CommandId::heartbeat_id && receiver_part == _heartbeat_part) {

                       if (_overTime_task) {
                            _overTime_task->cancel();
                        }
                        _overTime_task = _runtime->scheduleAtFixedRate(_heartbeat_timeout, _heartbeat_timeout, [this]() {
                            stop();
                            onConnectLost();
                        }, 1, false);
                    }
                }

        private:
            //std::chrono::steady_clock::time_point _last_heartbeat_time;
            sendmsg _send_msg;
            std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
            onConnectLost _onConnectLost;
            //int _my_id;
            const int _heartbeat_interval = SuOS::Uds::Df::heartbeat_interval;
            const int _heartbeat_timeout = SuOS::Uds::Df::heartbeat_timeout;
            const uint32_t _heartbeat_id = SuOS::Config::CommandId::heartbeat_id;
            const uint32_t _heartbeat_part = SuOS::Config::Part::Heartbeat;
            const uint32_t _router_id = SuOS::Config::Usr::ROUTER;
            std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _heartbeat_task = nullptr;
            // 定时器，用于检测心跳超时
            std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _overTime_task = nullptr;
    };
}