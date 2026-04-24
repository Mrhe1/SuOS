#ifndef UDS_MSG_BUILDER_HPP
#define UDS_MSG_BUILDER_HPP

#include "suUdsMsg_generated.h"
#include "suRuntime.hpp"
#include <mutex>

/// <warning> ///////////////////////////////////////
/// 仅限单线程调用///////////////////////////////////
/// </warning> //////////////////////////////////////
namespace SuOS::Uds::MsgBuilder {
    class MessageBuilder {
    public:
        // 定义一个内部类作为“资源锁句柄”
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb)
                : _fbb(fbb) {}

            LockGuard(LockGuard&& other) noexcept
                : _fbb(other._fbb) {}

            // 外部通过这个句柄获取数据
            uint8_t* data() { return _fbb.GetBufferPointer(); }
            size_t size() { return _fbb.GetSize(); }

        private:
            //std::unique_lock<std::mutex> _lock; // 利用 unique_lock 管理生命周期
            flatbuffers::FlatBufferBuilder& _fbb;

            // 禁止拷贝
            LockGuard(const LockGuard&) = delete;
            LockGuard& operator=(const LockGuard&) = delete;
        }; 

        MessageBuilder(uint32_t sender_usr, std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _sender_usr(sender_usr), _runtime(runtime) {}

        virtual ~MessageBuilder() = default;

        // 构造消息
        LockGuard finalizeEnvelope(uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, uint32_t cmd_id, const std::vector<uint8_t>& sub_payload) {
            //_mutex.lock();
            // 检查是否在事件循环中
            if(!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            
            fbb_.Clear();
            auto payload_vec = fbb_.CreateVector(sub_payload);
            auto root = SuOS::Uds::Msg::CreateMessageEnvelope
            (fbb_, _sender_usr, sender_part, receiver_usr, receiver_part, cmd_id, payload_vec);
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

    protected:
        flatbuffers::FlatBufferBuilder fbb_{ 1024 };
      
        uint32_t _sender_usr;       
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        //std::mutex _mutex;
    };
}

#endif // UDS_MSG_BUILDER_HPP