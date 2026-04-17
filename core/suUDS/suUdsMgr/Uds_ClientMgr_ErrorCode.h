#ifndef UDS_CLIENTMGR_ERRORCODE_H
#define UDS_CLIENTMGR_ERRORCODE_H

#include <cstdint>

namespace SuOS::Uds::ClientMgr::Errorcode { 
        // 使用 inline constexpr 是 C++17 及以后最推荐的做法
        inline constexpr uint32_t UdsClientOK = 0x00;
        inline constexpr uint32_t ConnectionTimeOut = 0x01;
        inline constexpr uint32_t NoSuchFileOrDirectory = 0x02;
        inline constexpr uint32_t UdsBadEroor = 0x03;
        inline constexpr uint32_t CertificationError = 0x04;
        inline constexpr uint32_t StateError = 0x05;
        //inline constexpr uint32_t ConnectTimedOut = 0x06;
}

#endif // UDS_CLIENTMGR_ERRORCODE_H