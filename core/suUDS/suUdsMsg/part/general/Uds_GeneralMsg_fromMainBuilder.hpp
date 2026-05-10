#pragma once
#ifndef UDS_GENERALMSG_FROMMAIN_BUILDER_HPP
#define UDS_GENERALMSG_FROMMAIN_BUILDER_HPP
#include "GeneralMsg_fromMain_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::General {
    class GeneralMsg_fromMainBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        GeneralMsg_fromMainBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 UsrStopRequest 消息
        LockGuard BuildUsrStopRequest() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopRequest(fbb_);
            auto root = CreateGeneralEnvelope_fromMain(fbb_, GeneralPayload_fromMain_UsrStopRequest, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 UsrStopKill 消息
        LockGuard BuildUsrStopKill() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopKill(fbb_);
            auto root = CreateGeneralEnvelope_fromMain(fbb_, GeneralPayload_fromMain_UsrStopKill, table.Union());
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