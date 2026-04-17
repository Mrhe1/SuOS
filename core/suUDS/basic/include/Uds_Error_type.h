#ifndef UDS_ERROR_TYPE_H
#define UDS_ERROR_TYPE_H

#include <cstdint>

namespace SuOS {
    namespace UdsError { 
        // 使用 inline constexpr 是 C++17 及以后最推荐的做法
        inline constexpr uint32_t ConnectionRefused = 0x01;
        inline constexpr uint32_t NoSuchFileOrDirectory = 0x02;
        inline constexpr uint32_t AddressInUse = 0x03;
        inline constexpr uint32_t ConnectionClosed = 0x04;
        inline constexpr uint32_t NoDescriptors = 0x05;
        inline constexpr uint32_t ConnectTimedOut = 0x06;
    }
}

#endif // UDS_ERROR_TYPE_H