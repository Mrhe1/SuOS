#pragma once

#ifndef UDS_ROUTER_ERRORCODE_H
#define UDS_ROUTER_ERRORCODE_H

#include <cstdint>

namespace SuOS::Uds::Router { 
    // 内部逻辑错误码
    namespace Errorcode {
        inline constexpr uint32_t OK = 0x00;
        inline constexpr uint32_t AuthFailed = 0x01;       // 认证失败
        inline constexpr uint32_t InvalidSender = 0x02;    // 身份冒充（sender_usr不匹配）
        inline constexpr uint32_t HeartbeatTimeout = 0x03; // 客户端心跳超时
        inline constexpr uint32_t ForwardingFailed = 0x04; // 消息转发失败（目标不存在）
        inline constexpr uint32_t UdsServerError = 0x05;   // 底层 Server 错误
    }

    // 暴露给外部调用的状态/错误码
    namespace outputErrorCode {
        inline constexpr uint32_t OK = 0x00;
        inline constexpr uint32_t ServerStartFailed = 0x01;
        inline constexpr uint32_t NotInEventLoop = 0x02;
        inline constexpr uint32_t StateError = 0x03;
    }
}

#endif