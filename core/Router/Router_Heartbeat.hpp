#pragma once
#include "suRuntime.hpp"
#include "SuOS_Config.h"
#include "Uds_df.h"
#include <map>
#include <functional>
#include <memory>

namespace SuOS::Uds::Router {
    class Router_Heartbeat {
    public:
        using OnTimeout = std::function<void(uint32_t client_id)>;
        using SendFunc = std::function<void(uint32_t target_id, const std::vector<uint8_t>& payload)>;

        Router_Heartbeat(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_func, OnTimeout on_timeout)
            : _runtime(runtime), _send_func(send_func), _on_timeout(on_timeout) {}

        // 启动全局广播心跳（向所有连接的客户端发）
        void startBroadcast(const std::vector<uint32_t>& (*get_clients)()) {
            _broadcast_task = _runtime->scheduleAtFixedRate(1000, 1600, [this, get_clients]() {
                auto clients = get_clients();
                for (auto cid : clients) {
                    _send_func(cid, {});
                }
            }, false);
        }

        // 当收到某个客户端的心跳包时，重置其超时计数
        void updateClient(uint32_t client_id) {
            if (_timeout_tasks.count(client_id)) {
                _timeout_tasks[client_id]->cancel();
            }

            // 设置超时
            _timeout_tasks[client_id] = _runtime->scheduleAtFixedRate(SuOS::Uds::ClientMgr::Df::heartbeat_timeout, 
                SuOS::Uds::ClientMgr::Df::heartbeat_timeout, [this, client_id]() {
                _on_timeout(client_id);
                stopClient(client_id);
            }, 1, false);
        }

        void stopClient(uint32_t client_id) {
            if (_timeout_tasks.count(client_id)) {
                _timeout_tasks[client_id]->cancel();
                _timeout_tasks.erase(client_id);
            }
        }

        void stopAll() {
            if (_broadcast_task) _broadcast_task->cancel();
            for (auto& pair : _timeout_tasks) pair.second->cancel();
            _timeout_tasks.clear();
        }

    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        SendFunc _send_func;
        OnTimeout _on_timeout;
        std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _broadcast_task;
        std::map<uint32_t, std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask>> _timeout_tasks;
    };
}