#pragma once
#ifndef UDS_PARTEXAMPLE_BUILDER_HPP
#define UDS_PARTEXAMPLE_BUILDER_HPP
#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::TestUniversal {
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
        // 构建 ScalarTable 消息
        LockGuard BuildScalarTable(uint8_t val_u8, uint32_t val_u32, int64_t val_i64, float val_float, bool val_bool) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateScalarTable(fbb_, val_u8, val_u32, val_i64, val_float, val_bool);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_ScalarTable, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 DataContainer 消息
        LockGuard BuildDataContainer(const std::string& name, const std::vector<uint8_t>& raw_data, const std::vector<uint32_t>& indices) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto name__ = fbb_.CreateString(name);
            auto raw_data__ = fbb_.CreateVector(raw_data);
            auto indices__ = fbb_.CreateVector(indices);
            auto table = CreateDataContainer(fbb_, name__, raw_data__, indices__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_DataContainer, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 EmptySignal 消息
        LockGuard BuildEmptySignal() {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateEmptySignal(fbb_);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_EmptySignal, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 ConfigInfo 消息
        LockGuard BuildConfigInfo(uint32_t id, const std::string& description) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto description__ = fbb_.CreateString(description);
            auto table = CreateConfigInfo(fbb_, id, description__);
            auto root = CreatePayloadEnvelope(fbb_, UdsPayload_ConfigInfo, table.Union());
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