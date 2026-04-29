#pragma once
#ifndef SU_DATABASE_HPP
#define SU_DATABASE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include "yaml-cpp/yaml.h"
//#include "nlohmann/json.hpp"
#include "Datastruct.h"
#include "suDbError.h"

using namespace boost::multi_index;
//using json = nlohmann::json;

namespace SuOS::Database {
    class RegistryManager {
    public:
        RegistryManager() = default;
        // 禁止外部实例化
        RegistryManager(const RegistryManager&) = delete;
        RegistryManager& operator=(const RegistryManager&) = delete;

        // --- 静态全局访问接口 ---
        
        // 单例获取
        static RegistryManager& instance() {
            static RegistryManager inst;
            return inst;
        }

        // --- 1. 统一加载方法 ---
        bool loadAll(const std::string& filename) {
            try {
                // 调用之前实现的 loadFromYaml
                loadFromYaml(usrReg, partReg, appReg, filename);
                return true;
            }
            catch (const std::exception& e) {
                std::cerr << "Load Failed: " << e.what() << std::endl;
                return false;
            }
        }

        void unloadAll() {
            usrReg.clear();
            partReg.clear();
            appReg.clear();
        }

        // --- 2. 用户查询 (按 ID) ---
        // 返回指针，找不到返回 nullptr，避免拷贝
        const UsrConfig* getUsrById(int id) const {
            auto& index = usrReg.get<by_id>();
            auto it = index.find(id);
            return (it != index.end()) ? &(*it) : nullptr;
        }

        // --- 3. 用户查询 (按 Name) ---
        const UsrConfig* getUsrByName(const std::string& name) const {
            auto& index = usrReg.get<by_name>();
            auto it = index.find(name);
            return (it != index.end()) ? &(*it) : nullptr;
        }

        // --- 4. 用户查询 (按 Group) ---
        std::vector<UsrConfig> getUsrByGroup(const std::string& group) const {
            //  获取对应的非唯一索引视图
            auto& group_index = usrReg.get<by_group>();
            // 使用 equal_range 获取 [起点, 终点) 的迭代器对
            auto range = group_index.equal_range(group);
            
            // 遍历这个范围
            std::vector<UsrConfig> result;
            for (auto it = range.first; it != range.second; ++it) {
                result.push_back(*it);
            }
            return result;
        }

        // --- 5. 修改状态 (按 ID 修改) ---
        // MultiIndex 修改元素需要使用 modify 接口
        bool updateAppStatus(int id, const std::string& newStatus) {
            auto& index = appReg.get<by_id>();
            auto it = index.find(id);
            if (it == index.end()) return false;

            return index.modify(it, [&](APPConfig& app) {
                app.status = newStatus;
                });
        }

        // --- 2. part查询 (按 ID) ---
        // 返回指针，找不到返回 nullptr，避免拷贝
        const PartConfig* getPartById(int id) const {
            auto& index = partReg.get<by_id>();
            auto it = index.find(id);
            return (it != index.end()) ? &(*it) : nullptr;
        }

        // --- 3. part查询 (按 Name) ---
        const PartConfig* getPartByName(const std::string& name) const {
            auto& index = partReg.get<by_name>();
            auto it = index.find(name);
            return (it != index.end()) ? &(*it) : nullptr;
        }

        // --- 4. part查询 (按 father) ---
        std::vector<PartConfig> getPartByFather(const std::string& father) const {
            //  获取对应的非唯一索引视图
            auto& group_index = partReg.get<by_father>();
            // 使用 equal_range 获取 [起点, 终点) 的迭代器对
            auto range = group_index.equal_range(father);

            // 遍历这个范围
            std::vector<PartConfig> result;
            for (auto it = range.first; it != range.second; ++it) {
                result.push_back(*it);
            }
            return result;
        }

        // --- 5. 获取某个父级下的所有零件 ---
        std::vector<PartConfig> getPartsByFather(const std::string& father) const {
            std::vector<PartConfig> result;
            auto& index = partReg.get<by_father>();
            auto range = index.equal_range(father); // 获取相同 father_name 的范围

            for (auto it = range.first; it != range.second; ++it) {
                result.push_back(*it);
            }
            return result;
        }

        // --- 6. 插入新 App ---
        bool addApp(const APPConfig& app) {
            auto res = appReg.insert(app);
            return res.second; // 返回是否插入成功（如果 ID 或 Name 已存在会失败）
        }



    private:
        void loadFromYaml(
            SuOS::Database::UsrRes& usrReg,
            SuOS::Database::PartRes& partReg,
            SuOS::Database::APPRes& appReg,
            const std::string& filename)
        {
            YAML::Node config = YAML::LoadFile(filename);

            // 1. 加载 usrs 部分
            if (config["usrs"]) { 
                usrReg.clear();
                for (const auto& item : config["usrs"]) {
                    SuOS::Database::UsrConfig usr;
                    usr.id = item["id"].as<int>();
                    usr.name = item["name"].as<std::string>();
                    usr.group = item["group"].as<std::string>();
                    usr.acl = item["acl"].as<std::string>();
                    usr.status = item["status"].as<std::string>();

                    // 处理固定大小数组 int[3]
                    if (item["linux_id"].IsSequence() && item["linux_id"].size() == 3) {
                        for (int i = 0; i < 3; ++i) {
                            usr.linux_id[i] = item["linux_id"][i].as<int>();
                        }
                    }
                    usrReg.insert(usr);
                }
            }

            // 2. 加载 part 部分
            if (config["part"]) {
                partReg.clear();
                for (const auto& item : config["part"]) {
                    SuOS::Database::PartConfig part;
                    part.id = item["id"].as<int>();
                    part.name = item["name"].as<std::string>();
                    part.father_name = item["father_name"].as<std::string>();
                    part.acl = item["acl"].as<std::string>();

                    partReg.insert(part);
                }
            }

            // 3. 加载 apps 部分
            if (config["apps"]) {
                appReg.clear();
                for (const auto& item : config["apps"]) {
                    SuOS::Database::APPConfig app;
                    app.id = item["id"].as<int>();
                    app.name = item["name"].as<std::string>();
                    app.status = item["status"].as<std::string>();

                    // 处理 linux_id [3]
                    if (item["linux_id"].IsSequence() && item["linux_id"].size() == 3) {
                        for (int i = 0; i < 3; ++i) {
                            app.linux_id[i] = item["linux_id"][i].as<int>();
                        }
                    }

                    // 处理 acl 列表 
                    if (item["acl"] && item["acl"].IsSequence()) {
                        for (const auto& acl_item : item["acl"]) {
                            app.acl.push_back(acl_item.as<std::string>());
                        }
                    }

                    appReg.insert(app);
                }
            }
        }

        // --- 核心数据容器 ---
        UsrRes usrReg;
        PartRes partReg;
        APPRes appReg;
    };
}

#endif // SU_DATABASE_HPP