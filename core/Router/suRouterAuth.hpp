#pragma once
#ifndef SU_ROUTER_AUTH_HPP
#define SU_ROUTER_AUTH_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <functional>

// 此处需根据你的实际工程路径调整头文件包含路径
#include "../Main/Database/suDbRegistryMgr.hpp"

namespace SuOS::Router {

    class RouterAuthManager {
    public:
        // ==========================================
        //  1. 动态允许列表机制 (支持实时拉取或被动推送)
        // ==========================================

        // 由底层 UDS 模块绑定拉取函数（主动调用获取实时的允许连接app）
        using PullAllowedAppsCallback = std::function<std::vector<std::string>()>;
        RouterAuthManager(PullAllowedAppsCallback cb): pullCb_(std::move(cb)) {}

        // Router 主动同步：调用已注册的 UDS 通讯拉取最新的白名单
        bool syncAllowedApps() {
            if (pullCb_) {
                auto apps = pullCb_();
                setAllowedApps(apps);
                return true;
            }
            return false;
        }

        // 被动通知一：全量覆盖实时的允许外接 App 列表 (Main 通过 UDS 发送时调用)
        void setAllowedApps(const std::vector<std::string>& activeApps) {
            std::lock_guard<std::mutex> lock(mtx_);
            allowedOutApps_.clear();
            for (const auto& app : activeApps) {
                allowedOutApps_.insert(app);
            }
        }

        // 被动通知二：单个应用状态更新 (Main通知 App 上下线)
        void updateAppAllowedState(const std::string& appName, bool isAllowed) {
            std::lock_guard<std::mutex> lock(mtx_);
            if (isAllowed) {
                allowedOutApps_.insert(appName);
            }
            else {
                allowedOutApps_.erase(appName);
            }
        }

        // 2. 连接鉴权
        // 修改：现在主要通过 execPath 查找实体，不再通过 entityName
        // 返回：成功返回 entityId，失败返回 -1 (或其他约定错误码)
        int authenticateConnection(int pid, int uid, int gid, const std::string& execPath) {
            auto& db = Database::RegistryManager::instance();
            
            // 查找所有 Usr
            // 注意：由于数据库当前是以 ID 和 Name 为索引，这里需要遍历或后续为 path 增加索引
            // 为了保持逻辑清晰，假设我们会通过 path 校验。
            
            // A. 尝试在 Usr (Core) 中匹配
            const Database::UsrConfig* foundUsr = nullptr;
            // 遍历所有 Usr (当前数据库未提供 getByPath，暂用全量获取逻辑的简化模拟)
            // 实际建议在 RegistryManager 增加通过 path 查找
            auto usrs = db.getUsrByGroup("core"); // 仅作示意，实际应匹配 path
            // 补充：这里需要 RegistryManager 支持按 path 查找。我先按逻辑写。
            
            foundUsr = db.getUsrByPath(execPath);

            if (foundUsr) {
                // 校验 UID, GID (数据库中 PID 约定为 -1 则忽略)
                if (!checkLinuxId(foundUsr->linux_id, -1, uid, gid)) {
                    std::cerr << "Auth Fail: User Path '" << execPath << "' Linux_ID mismatch.\n";
                    return -1;
                }

                // 记录到本地映射
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    entityGroupMap_[foundUsr->id] = "core";
                }
                return foundUsr->id;
            }

            // B. 尝试在 App (Out) 中匹配
            const Database::APPConfig* foundApp = db.getAppByPath(execPath);
            if (foundApp) {
                // 1) 检查是否在实时允许列表中
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    if (allowedOutApps_.find(foundApp->name) == allowedOutApps_.end()) {
                        std::cerr << "Auth Fail: App Path '" << execPath << "' (Name: "<< foundApp->name <<") is not in real-time allowed list.\n";
                        return -1;
                    }
                }

                // 2) 校验 UID, GID
                if (!checkLinuxId(foundApp->linux_id, -1, uid, gid)) {
                    std::cerr << "Auth Fail: App Path '" << execPath << "' Linux_ID mismatch.\n";
                    return -1;
                }

                // 3) 记录映射
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    entityGroupMap_[foundApp->id] = "out";
                }
                return foundApp->id;
            }

            std::cerr << "Auth Fail: No entity found for Path '" << execPath << "'.\n";
            return -1;
        }

        // ==========================================
        //  3. 消息通讯鉴权
        // ==========================================
        // 接收 senderId 和 receiverId
        bool authorizeMessage(int senderId, int receiverId) {
            std::string senderGroup;
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (entityGroupMap_.find(senderId) == entityGroupMap_.end()) return false;
                senderGroup = entityGroupMap_[senderId];
            }

            // 如果 Sender 是 Core，放行
            if (senderGroup == "core") {
                return true;
            }

            // 到这里 Sender 是 Out 层
            auto& db = Database::RegistryManager::instance();
            auto senderApp = db.getAppById(senderId);
            if (!senderApp) return false;

            // 获取 Receiver 的 Group
            std::string receiverGroup;
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (entityGroupMap_.find(receiverId) != entityGroupMap_.end()) {
                    receiverGroup = entityGroupMap_[receiverId];
                }
            }

            // 规则一: Out -> Out 不允许
            if (receiverGroup == "out") {
                std::cerr << "Msg Auth Fail: Inter-App communication forbidden.\n";
                return false;
            }

            // 规则二: Out -> Core (Usr 或 Part)
            // 需要检查 SenderApp 的 ACL。ACL 存的是名字。
            // 我们需要看 receiverId 对应的名字是否在 senderApp->acl 中。
            
            std::string receiverName;
            auto recUsr = db.getUsrById(receiverId);
            if (recUsr) {
                receiverName = recUsr->name;
            } else {
                auto recPart = db.getPartById(receiverId);
                if (recPart) receiverName = recPart->name;
            }

            if (receiverName.empty()) return false;

            for (const auto& allowedTarget : senderApp->acl) {
                if (allowedTarget == receiverName) {
                    return true;
                }
            }

            std::cerr << "Msg Auth Fail: App '" << senderApp->name << "' lacks ACL for '" << receiverName << "'.\n";
            return false;
        }

    private:
        RouterAuthManager() = default;

        std::mutex mtx_;
        std::unordered_set<std::string> allowedOutApps_;
        std::unordered_map<int, std::string> entityGroupMap_; // 记录 ID -> "core"/"out"
        PullAllowedAppsCallback pullCb_; 


        // ----- 辅助验证方法 -----
        
        bool checkLinuxId(const int expected_id[3], int real_pid, int real_uid, int real_gid) const {
            // yaml中配置约定：linux_id 数组下如果为 -1，就跳过该项
            // 注意：不再检查 pid (real_pid 传入 -1)
            // if (expected_id[0] != -1 && expected_id[0] != real_pid) return false;
            if (expected_id[1] != -1 && expected_id[1] != real_uid) return false;
            if (expected_id[2] != -1 && expected_id[2] != real_gid) return false;
            return true;
        }

        bool validateExecPath(const std::string& entityName, const std::string& execPath) const {
            // TODO: 由于你目前的 config.yaml 暂未解析执行文件路径配置，
            // 此处留空为一个 placeholder。若增加配置如 `exec_path: "/opt/app/..."`，
            // 可通过传入的 entityName 在缓存或数据库中对照。
            return true;
        }
    };
} 

#endif // SU_ROUTER_AUTH_HPP