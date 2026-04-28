#ifndef UDS_CLIENTMGR_ERRORCODE_H
#define UDS_CLIENTMGR_ERRORCODE_H

#include <cstdint>

namespace SuOS::Uds::ClientMgr { 
        // 内部使用的错误码
        namespace Errorcode {
                // 使用 inline constexpr 是 C++17 及以后最推荐的做法
                inline constexpr uint32_t UdsClientOK = 0x00;
                // 指多次重连后最终失败
                inline constexpr uint32_t ConnectionTimeOut = 0x01;
                inline constexpr uint32_t NoSuchFileOrDirectory = 0x02;
                inline constexpr uint32_t UdsBadEroor = 0x03;
                inline constexpr uint32_t CertificationError = 0x04;
                inline constexpr uint32_t HeartbeatTimeout = 0x05;
        }

        // 输出给用户的错误码
        namespace outputErrorCode {
                inline constexpr uint32_t UdsClientOK = 0x00;
                inline constexpr uint32_t NoSuchFileOrDirectory = 0x02;
                inline constexpr uint32_t ReconnectFail = 0x03;
                inline constexpr uint32_t CertificationError = 0x04;
                inline constexpr uint32_t StateError = 0x05;
                // 不在enventloop中调用
                inline constexpr uint32_t NotInEventLoop = 0x06;
                inline constexpr uint32_t ConnectionTimeOut = 0x07;
        }
}

#endif // UDS_CLIENTMGR_ERRORCODE_H