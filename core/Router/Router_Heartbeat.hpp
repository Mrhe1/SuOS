#pragma once
#include "suRuntime.hpp"
#include "SuOS_Config.h"
#include "Uds_df.h"
#include <map>
#include <set>
#include <functional>
#include <memory>

namespace SuOS::Uds::Router {
    class Router_Heartbeat {
    public:
        using OnTimeout = std::function<void(uint32_t client_id)>;
        using SendFunc = std::function<void(uint32_t target_id, const std::vector<uint8_t>& payload)>;

        Router_Heartbeat(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_func, OnTimeout on_timeout)
            : _runtime(runtime), _send_func(send_func), _on_timeout(on_timeout) {}

        // 启动全局广播心跳
        void startBroadcast() {
            _broadcast_task = _runtime->scheduleAtFixedRate(SuOS::Uds::ClientMgr::Df::heartbeat_interval, 
                SuOS::Uds::ClientMgr::Df::heartbeat_interval, [this]() {
                if (_active_clients.empty()) return;
                for (auto cid : _active_clients) {
                    _send_func(cid, {});
                }
            }, false);
        }

        // 新增客户端到心跳管理列表
        void addClient(uint32_t client_id) {
            _active_clients.insert(client_id);
            updateClient(client_id);
        }

        // 当收到某个客户端的心跳包时，重置其超时计数
        void updateClient(uint32_t client_id) {
            _active_clients.insert(client_id);
            if (_timeout_tasks.count(client_id)) {
                _timeout_tasks[client_id]->cancel();
            }

            // 设置超时
            _timeout_tasks[client_id] = _runtime->scheduleAtFixedRate(SuOS::Uds::ClientMgr::Df::heartbeat_timeout, 
                SuOS::Uds::ClientMgr::Df::heartbeat_timeout, [this, client_id]() {
                stopClient(client_id);
                _on_timeout(client_id);                
            }, 1, false);
        }

        void stopClient(uint32_t client_id) {
            _active_clients.erase(client_id);
            if (_timeout_tasks.count(client_id)) {
                _timeout_tasks[client_id]->cancel();
                _timeout_tasks.erase(client_id);
            }
        }

        void stopAll() {
            if (_broadcast_task) _broadcast_task->cancel();
            for (auto& pair : _timeout_tasks) pair.second->cancel();
            _timeout_tasks.clear();
            _active_clients.clear();
        }

    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        SendFunc _send_func;
        OnTimeout _on_timeout;
        std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _broadcast_task;
        std::set<uint32_t> _active_clients;
        std::map<uint32_t, std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask>> _timeout_tasks;
    };
}