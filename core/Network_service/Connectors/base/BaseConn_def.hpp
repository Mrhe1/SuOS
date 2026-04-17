#ifndef BASECOBB_DEF_HPP
#define BASECOBB_DEF_HPP

#include <cstdint>

namespace baseConn_def {

    /**
     * @brief 错误码定义 
     * 用于 BaseConnection::emitError 中的 code 参数
     */
    enum class ErrorCode : int {
        SUCCESS = 0,
        UNKNOWN_ERROR = -1,
        CONNECTION_FAILED = 1001,
        TIMEOUT = 1002,
        DISCONNECTED = 1003,
        AUTH_FAILED = 1004,
        SEND_BUFFER_FULL = 2001,
        INVALID_PAYLOAD = 2002
    };

    /**
     * @brief 动作类型定义（可选）
     * 如果你的 seq_id 需要按功能分配一段区间，可以在这里定义起始值
     */
    namespace ActionId {
        static constexpr uint32_t Connect_Success = 100;
        static constexpr uint32_t Send_Success = 101;
    }

} // namespace baseConn_def

#endif // BASECOBB_DEF_HPP