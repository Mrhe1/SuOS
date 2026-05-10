#pragma once
#ifndef UDS_PARTEXAMPLE_BUILDER_HPP
#define UDS_PARTEXAMPLE_BUILDER_HPP
#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::PartExample {
    class PartExampleBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        PartExampleBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 NoArgs 消息
        LockGuard BuildNoArgs() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateNoArgs(fbb_);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_NoArgs, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 CmdRequest 消息
        LockGuard BuildCmdRequest(uint32_t seq_no, const std::vector<uint8_t>& parameters) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto parameters__ = fbb_.CreateVector(parameters);
            auto table = CreateCmdRequest(fbb_, seq_no, parameters__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_CmdRequest, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 CmdResponse 消息
        LockGuard BuildCmdResponse(uint32_t seq_no, uint32_t result_code, const std::vector<uint8_t>& data) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto data__ = fbb_.CreateVector(data);
            auto table = CreateCmdResponse(fbb_, seq_no, result_code, data__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_CmdResponse, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 EventNotify 消息
        LockGuard BuildEventNotify(uint32_t event_id, uint32_t severity, const std::vector<uint8_t>& detail) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto detail__ = fbb_.CreateVector(detail);
            auto table = CreateEventNotify(fbb_, event_id, severity, detail__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_EventNotify, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 ErrorResponse 消息
        LockGuard BuildErrorResponse(uint32_t seq_no, uint32_t error_code, const std::string& error_msg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto error_msg__ = fbb_.CreateString(error_msg);
            auto table = CreateErrorResponse(fbb_, seq_no, error_code, error_msg__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_ErrorResponse, table.Union());
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