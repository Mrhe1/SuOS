// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_GENERALMSG_TOMAIN_BUILDER_HPP
#define UDS_GENERALMSG_TOMAIN_BUILDER_HPP
#include "GeneralMsg_toMain_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::General {
    class GeneralMsg_toMainBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        GeneralMsg_toMainBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 UsrStopResponse 消息
        LockGuard BuildUsrStopResponse() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateUsrStopResponse(fbb_);
            auto root = CreateGeneralEnvelope_toMain(fbb_, GeneralPayload_toMain_UsrStopResponse, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 onUsrError 消息
        LockGuard BuildonUsrError(uint32_t code, const std::string& msg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto msg__ = fbb_.CreateString(msg);
            auto table = CreateonUsrError(fbb_, code, msg__);
            auto root = CreateGeneralEnvelope_toMain(fbb_, GeneralPayload_toMain_onUsrError, table.Union());
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