// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_ROUTERMSG_FROMROUTER_BUILDER_HPP
#define UDS_ROUTERMSG_FROMROUTER_BUILDER_HPP
#include "RouterMsg_fromRouter_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::Router {
    class RouterMsg_fromRouterBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        RouterMsg_fromRouterBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 onConnect 消息
        LockGuard BuildonConnect(uint32_t id) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateonConnect(fbb_, id);
            auto root = CreateRouterEnvelope_fromRouter(fbb_, RouterPayload_fromRouter_onConnect, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 onConnectionLost 消息
        LockGuard BuildonConnectionLost(uint32_t id) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateonConnectionLost(fbb_, id);
            auto root = CreateRouterEnvelope_fromRouter(fbb_, RouterPayload_fromRouter_onConnectionLost, table.Union());
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