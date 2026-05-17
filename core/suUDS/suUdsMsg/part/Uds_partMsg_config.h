#pragma once

#include "SuOS_Config.h"

namespace SuOS::Uds::Msg {
    namespace Router::config {
        // 指RouterMsg_fromRouter.fbs这个消息的part定义
        uint32_t fromRouter_receiverPart = SuOS::Config::Part::MAIN;
        uint32_t fromRouter_senderPart = SuOS::Config::Part::ROUTER;
    }
}