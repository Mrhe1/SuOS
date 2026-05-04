#pragma once
#ifndef UDS_GENERALMSG_BUILDER_HPP
#define UDS_GENERALMSG_BUILDER_HPP
#include "GeneralMsg_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::General {
    class GeneralMsgBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        GeneralMsgBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 Heartbeat 消息
        LockGuard BuildHeartbeat() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateHeartbeat(fbb_);
            auto root = CreateGeneralEnvelope(fbb_, GeneralPayload_Heartbeat, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 UsrStopRequest 消息
        LockGuard BuildUsrStopRequest() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopRequest(fbb_);
            auto root = CreateGeneralEnvelope(fbb_, GeneralPayload_UsrStopRequest, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 UsrStopResponse 消息
        LockGuard BuildUsrStopResponse() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopResponse(fbb_);
            auto root = CreateGeneralEnvelope(fbb_, GeneralPayload_UsrStopResponse, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 UsrStopKill 消息
        LockGuard BuildUsrStopKill() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopKill(fbb_);
            auto root = CreateGeneralEnvelope(fbb_, GeneralPayload_UsrStopKill, table.Union());
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