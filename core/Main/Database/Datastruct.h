#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <fstream>
#include <iostream>

using namespace boost::multi_index;

namespace SuOS {
    namespace Database {

        // 1. 业务对象
        struct UsrConfig {
            int id;
            std::string name;
            std::string group;
            int linux_id[3];
            std::string acl;
            std::string status;
        };

        // 2. 定义 MultiIndex 容器：支持通过 ID 和 Name 快速查找
        typedef multi_index_container<
            UsrConfig,
            indexed_by<
            hashed_unique<tag<struct by_id>, member<UsrConfig, int, &UsrConfig::id>>,
            hashed_unique<tag<struct by_name>, member<UsrConfig, std::string, &UsrConfig::name>>,
            hashed_non_unique<tag<struct by_group>, member<UsrConfig, std::string, &UsrConfig::group>>
            >
        > UsrRes;

        // 1. 业务对象
        struct PartConfig {
            int id;
            std::string name;
            std::string father_name;
            std::string acl;
        };

        // 2. 定义 MultiIndex 容器：支持通过 ID 和 Name 快速查找
        typedef multi_index_container<
            PartConfig,
            indexed_by<
            hashed_unique<tag<struct by_id>, member<PartConfig, int, &PartConfig::id>>,
            hashed_unique<tag<struct by_name>, member<PartConfig, std::string, &PartConfig::name>>,
            hashed_non_unique<tag<struct by_father>, member<PartConfig, std::string, &PartConfig::father_name>>
            >
        > PartRes;


        // 1. 业务对象
        struct APPConfig {
            int id;
            std::string name;
            int linux_id[3];
            std::string status;
            std::vector<std::string> acl;
        };

        // 2. 定义 MultiIndex 容器：支持通过 ID 和 Name 快速查找
        typedef multi_index_container<
            APPConfig,
            indexed_by<
            hashed_unique<tag<struct by_id>, member<APPConfig, int, &APPConfig::id>>,
            hashed_unique<tag<struct by_name>, member<APPConfig, std::string, &APPConfig::name>>,
            hashed_non_unique<tag<struct by_status>, member<APPConfig, std::string, &APPConfig::status>>
            >
        > APPRes; 
    }
}