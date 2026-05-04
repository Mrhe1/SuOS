#pragma once

#ifndef UDS_PART_EXAMPLE_BUILDER_HPP
#define UDS_PART_EXAMPLE_BUILDER_HPP

#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>

namespace SuOS::Uds::Msg::PartExample {

    class PartExampleBuilder {
    public:
        // 资源句柄，用于提取生成的 FlatBuffer 数据
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            LockGuard(LockGuard&& other) noexcept : _fbb(other._fbb) {}

            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }

        private:
            flatbuffers::FlatBufferBuilder& _fbb;
            LockGuard(const LockGuard&) = delete;
            LockGuard& operator=(const LockGuard&) = delete;
        };

        PartExampleBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        virtual ~PartExampleBuilder() = default;

        // 构建 NoArgs 消息
        LockGuard BuildNoArgs() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto noargs = CreateNoArgs(fbb_);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_NoArgs, noargs.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        // 构建 CmdRequest 消息
        LockGuard BuildCmdRequest(uint32_t seq_no, const std::vector<uint8_t>& parameters) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto params_vec = fbb_.CreateVector(parameters);
            auto cmd = CreateCmdRequest(fbb_, seq_no, params_vec);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_CmdRequest, cmd.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        // 构建 CmdResponse 消息
        LockGuard BuildCmdResponse(uint32_t seq_no, uint32_t result_code, const std::vector<uint8_t>& data) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto data_vec = fbb_.CreateVector(data);
            auto res = CreateCmdResponse(fbb_, seq_no, result_code, data_vec);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_CmdResponse, res.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        // 构建 EventNotify 消息
        LockGuard BuildEventNotify(uint32_t event_id, uint32_t severity, const std::vector<uint8_t>& detail) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto detail_vec = fbb_.CreateVector(detail);
            auto ev = CreateEventNotify(fbb_, event_id, severity, detail_vec);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_EventNotify, ev.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        // 构建 ErrorResponse 消息
        LockGuard BuildErrorResponse(uint32_t seq_no, uint32_t error_code, const std::string& error_msg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto msg_str = fbb_.CreateString(error_msg);
            auto err = CreateErrorResponse(fbb_, seq_no, error_code, msg_str);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_ErrorResponse, err.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }

        // 辅助创建方法
        flatbuffers::FlatBufferBuilder& fbb() { return fbb_; }

    private:
        flatbuffers::FlatBufferBuilder fbb_{ 1024 };
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    };

} // namespace SuOS::Uds::Msg::PartExample

#endif // UDS_PART_EXAMPLE_BUILDER_HPP
