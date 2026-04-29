#pragma once

#include <cstdint>
namespace SuOS::Service::Df {
    namespace stopCause {
        // 被外部（main）要求正常停止
        inline constexpr uint32_t normal = 0;
        // 被外部（main）要求强制停止
        inline constexpr uint32_t force = 1;
        // 运行时异常
        inline constexpr uint32_t runtime_error = 2;
        // uds异常
        inline constexpr uint32_t uds_error = 3;
    }
}