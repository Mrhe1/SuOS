#pragma once
#include "SuOS_Config.h"
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
        RouterAuthManager() = default;

        // 拉取所有usr及app
        std::vector<uint32_t> getAllUsrs() {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<uint32_t> ids;
            ids.reserve(entityGroupMap_.size());
            for (const auto& pair : entityGroupMap_) {
                ids.push_back(pair.first);
            }
            return ids;
        }

        // 移除已注册实体 (断开连接时调用)
        void removeEntity(uint32_t entityId) {
            std::lock_guard<std::mutex> lock(mtx_);
            entityGroupMap_.erase(entityId);
        }

        // 被动通知一：全量覆盖实时的允许外接 App 列表 (Main 通过 UDS 发送时调用)
        void setAllowedApps(const std::vector<uint32_t>& activeAppIds) {
            std::lock_guard<std::mutex> lock(mtx_);
            allowedOutApps_.clear();
            for (const auto& id : activeAppIds) {
                allowedOutApps_.insert(id);
            }
        }

        // 被动通知二：单个应用状态更新 (Main通知 App 上下线)
        void updateAppAllowedState(uint32_t appId, bool isAllowed) {
            std::lock_guard<std::mutex> lock(mtx_);
            if (isAllowed) {
                allowedOutApps_.insert(appId);
            }
            else {
                allowedOutApps_.erase(appId);
            }
        }

        // 2. 连接鉴权
        // 修改：现在主要通过 execPath 查找实体，不再通过 entityName
        // 返回：成功返回 entityId，失败返回 -1 (或其他约定错误码)
        int authenticateConnection(int pid, int uid, int gid, const std::string& execPath) {
            auto& db = Database::RegistryManager::instance();
            (void) pid;
            // 查找所有 Usr
            // 为了保持逻辑清晰，假设我们会通过 path 校验。
            
            // A. 尝试在 Usr (Core) 中匹配
            const Database::UsrConfig* foundUsr = db.getUsrByPath(execPath);

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
                    if (allowedOutApps_.find(foundApp->id) == allowedOutApps_.end()) {
                        std::cerr << "Auth Fail: App Path '" << execPath << "' (ID: "<< foundApp->id <<") is not in real-time allowed list.\n";
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
                auto it = entityGroupMap_.find(senderId);
                if (it == entityGroupMap_.end()) return false;
                senderGroup = it->second;
            }

            // 如果 Sender 是 Core，放行
            if (senderGroup == "core") {
                return true;
            }

            // 到这里 Sender 是 Out 层 (App)
            auto& db = Database::RegistryManager::instance();
            auto senderApp = db.getAppById(senderId);
            if (!senderApp) return false;

            // 获取 Receiver 的 Group
            std::string receiverGroup;
            {
                std::lock_guard<std::mutex> lock(mtx_);
                auto it = entityGroupMap_.find(receiverId);
                if (it != entityGroupMap_.end()) {
                    receiverGroup = it->second;
                }
            }

            // 规则一: Out -> Out 不允许
            if (receiverGroup == "out") {
                std::cerr << "Msg Auth Fail: Inter-App communication forbidden.\n";
                return false;
            }

            // 规则二: Out -> Core (Usr 或 Part)
            // 获取 Receiver 的 ACL 策略和名称
            std::string receiverAclPolicy;
            //uint32_t receiverId_ = receiverId; // 已经是ID
            auto recUsr = db.getUsrById(receiverId);
            if (recUsr) {
                receiverAclPolicy = recUsr->acl;
            } 

            if (receiverAclPolicy.empty()) return false;

            // 1. 如果是 core_only，不允许 Out 访问
            if (receiverAclPolicy == SuOS::Config::partAcl::core_only) {
                std::cerr << "Msg Auth Fail: Receiver ID '" << receiverId << "' is core_only.\n";
                return false;
            }

            // 2. 如果是 all_accessible，直接允许
            if (receiverAclPolicy == SuOS::Config::partAcl::all_accessible) {
                return true;
            }

            // 3. 如果是 out_optional，检查 Sender 的 ACL 列表是否包含此 Receiver
            if (receiverAclPolicy == SuOS::Config::partAcl::out_optional) {
                auto receiver = db.getAppById(receiverId);
                // receiver_的part——name
                auto receiver_name = receiver->name;
                // 假设 senderApp->acl 存储的是目标 ID
                for (const auto& allowedName : senderApp->acl) {
                    if (allowedName == receiver_name) {
                        return true;
                    }
                }
                std::cerr << "Msg Auth Fail: App ID '" << senderId << "' not in ACL for receiver ID '" << receiverId << "'.\n";
                return false;
            }

            std::cerr << "Msg Auth Fail: Unknown ACL policy '" << receiverAclPolicy << "' for ID '" << receiverId << "'.\n";
            return false;
        }

    private:
        //RouterAuthManager() = default;

        std::mutex mtx_;
        std::unordered_set<uint32_t> allowedOutApps_;
        std::unordered_map<uint32_t, std::string> entityGroupMap_; // 记录 ID -> "core"/"out"

        // ----- 辅助验证方法 -----
        
        bool checkLinuxId(const int expected_id[3], int real_pid, int real_uid, int real_gid) const {
            // yaml中配置约定：linux_id 数组下如果为 -1，就跳过该项
            // 注意：不再检查 pid (real_pid 传入 -1)
            (void)real_pid;
            // if (expected_id[0] != -1 && expected_id[0] != real_pid) return false;
            if (expected_id[1] != -1 && expected_id[1] != real_uid) return false;
            if (expected_id[2] != -1 && expected_id[2] != real_gid) return false;
            return true;
        }

    };
} 

#endif // SU_ROUTER_AUTH_HPP