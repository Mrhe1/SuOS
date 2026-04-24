#ifndef UDS_MSG_PARSER_HPP
#define UDS_MSG_PARSER_HPP

#include "suUdsMsg_generated.h"
#include "suRuntime.hpp"
#include <mutex>

/// <warning> ///////////////////////////////////////
/// 仅限单线程调用///////////////////////////////////
/// </warning> //////////////////////////////////////
namespace SuOS::Uds::MsgParser {
    class MessageParser {
    public:
        class LockGuard {
        public:
            // 使用 std::adopt_lock 接收已经 lock 的互斥量
            LockGuard(const SuOS::Uds::Msg::MessageEnvelope* envelope)
                : _envelope(envelope) {}

            // 必须显式支持移动构造，否则无法 return
            LockGuard(LockGuard&& other) noexcept
                : _envelope(other._envelope) {}

            bool isValid() const { return _envelope != nullptr; }
            uint32_t getSenderUsr() const { return _envelope->sender_usr(); }
            uint32_t getSenderPart() const { return _envelope->sender_part(); }
            uint32_t getReceiverUsr() const { return _envelope->receiver_usr(); }
            uint32_t getReceiverPart() const { return _envelope->receiver_part(); }
            uint32_t getCmdId() const { return _envelope->cmd_id(); }
            const uint8_t* payloadData() const { return _envelope->payload()->data(); }
            size_t payloadSize() const { return _envelope->payload()->size(); }
            

        private:
            //std::unique_lock<std::mutex> _lock; // 利用 unique_lock 管理生命周期
            const SuOS::Uds::Msg::MessageEnvelope* _envelope;

            // 禁用拷贝，防止锁被意外复制
            LockGuard(const LockGuard&) = delete;
            LockGuard& operator=(const LockGuard&) = delete;
        };

        MessageParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构造时直接进行内存校验，确保包没损坏
        LockGuard ParseMsg(const uint8_t* buffer, size_t size) {
            // 检查是否在事件循环中
            if(!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            
            _verifier = std::make_unique<flatbuffers::Verifier>(buffer, size);
            if (SuOS::Uds::Msg::VerifyMessageEnvelopeBuffer(*_verifier)) {
                //_mutex.lock(); 
                _envelope = SuOS::Uds::Msg::GetMessageEnvelope(buffer);
                return LockGuard(_envelope);
            }
            return LockGuard(nullptr);
        }

    private:
        const SuOS::Uds::Msg::MessageEnvelope* _envelope = nullptr;
        std::unique_ptr<flatbuffers::Verifier> _verifier;
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        //std::mutex _mutex;
        //std::mutex _dummy_mutex; // 用于校验失败时的占位
    };
} // namespace SuOS::Uds::MsgParser

#endif // UDS_MSG_PARSER_HPP