#pragma once
#ifndef SUOS_CONFIG_H
#define SUOS_CONFIG_H

#include <cstdint>
#include <string>
#include <string_view>

namespace SuOS {
    namespace Config {

        // === Usr IDs (系统应用注册 ID) ===
        namespace Usr {
            constexpr uint32_t ALL = 1000;
            constexpr uint32_t MAIN = 1001;
            constexpr uint32_t APP_MANAGER = 1002;
            constexpr uint32_t GRAPHIC = 1003;
            constexpr uint32_t DRIVER = 1004;
            constexpr uint32_t NETWORK = 1005;
            constexpr uint32_t APP_CONTAINER = 1006;
            constexpr uint32_t ROUTER = 1007;
        }

        namespace UsrPath { 
            constexpr std::string_view ALL = "";
            constexpr std::string_view MAIN = "";
            constexpr std::string_view APP_MANAGER = "";
            constexpr std::string_view GRAPHIC = "";
            constexpr std::string_view DRIVER = "";
            constexpr std::string_view NETWORK = "";
            constexpr std::string_view APP_CONTAINER = "";
            constexpr std::string_view ROUTER = "";
        }

        namespace usrGroup {
            // app属于out但不属于usr故不列出
            constexpr std::string_view core = "core";
            constexpr std::string_view special = "special";
        }

        // === Part IDs (组件/服务注册 ID) ===
        namespace Part {
            constexpr uint32_t ALL = 10000;
            constexpr uint32_t MAIN = 10001;
            constexpr uint32_t APP_MANAGER = 10002;
            constexpr uint32_t APP_RUN = 10003;
            constexpr uint32_t DISPLAY = 10004;
            constexpr uint32_t VIDEO_DECORDING = 10005;
            constexpr uint32_t LOCATION_SERVICE = 10006;
            constexpr uint32_t HEALTH_SERVICE = 10007;
            constexpr uint32_t REALTIME_HEALTH_SERVICE = 10008;
            constexpr uint32_t MQTT_CLIENT = 10009;
            constexpr uint32_t HTTP_CLIENT = 10010;
            constexpr uint32_t APP_CONTAINER = 10011;
            constexpr uint32_t ROUTER = 10012;
            constexpr uint32_t APP_GRAPHICS_RGA = 10013;
            constexpr uint32_t USR_CONTROL = 10014;
            constexpr uint32_t USR_REPORT = 10015;
            constexpr uint32_t DISPLAY_RGA = 10016;
            constexpr uint32_t HEARTBEAT = 10017;
        }

        // namespace CommandId {
        //     constexpr uint32_t heartbeat_id = 100001;
        //     // main命令一个usr（进程）stop
        //     constexpr uint32_t usr_stop_request = 100002;
        //     // usr(进程)回报main应该准备好被关闭
        //     constexpr uint32_t usr_stop_response = 100003;
        //     // main遇到问题，停止进程会被kill
        //     constexpr uint32_t usr_stop_kill = 100004;
        // }

        namespace uid {
            // 待定
            constexpr uint32_t ROUTER = 1001;
        }

        namespace gid {
            // 待定
            constexpr uint32_t ROUTER = 1001;
        }

        namespace configPath {
            constexpr std::string_view config = "/home/hss/SuOS/core/config.yaml";
        }

    } // namespace Config
} // namespace SuOS

#endif // SUOS_CONFIG_H