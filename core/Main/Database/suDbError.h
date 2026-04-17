#ifndef SU_DbError_H
#define SU_DbError_H

#include <cstdint>

namespace SuOS { 
    namespace DbError {
        // 使用 inline constexpr 是 C++17 及以后最推荐的做法
        inline constexpr uint32_t Success = 0x01;
        inline constexpr uint32_t LoadYamlFail = 0x02;
        inline constexpr uint32_t FoundNothing = 0x03;
        //inline constexpr uint32_t AddressInUse = 0x04;
    }
}

#endif // SU_DbError_H