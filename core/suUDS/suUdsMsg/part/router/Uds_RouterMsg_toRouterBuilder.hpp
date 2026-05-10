#pragma once
#ifndef UDS_ROUTERMSG_TOROUTER_BUILDER_HPP
#define UDS_ROUTERMSG_TOROUTER_BUILDER_HPP
#include "RouterMsg_toRouter_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::Router {
    class RouterMsg_toRouterBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        RouterMsg_toRouterBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 enableAppRegister 消息
        LockGuard BuildenableAppRegister(uint32_t app_id) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateenableAppRegister(fbb_, app_id);
            auto root = CreateRouterEnvelope_toRouter(fbb_, RouterPayload_toRouter_enableAppRegister, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        flatbuffers::FlatBufferBuilder& fbb() { return fbb_; }
    private:
        flatbuffers::FlatBufferBuilder fbb_{ 1024 };
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    };
}
#endif