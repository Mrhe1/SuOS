#pragma once
#ifndef UDS_GRAPHICSMSG_FROMRGA_BUILDER_HPP
#define UDS_GRAPHICSMSG_FROMRGA_BUILDER_HPP
#include "GraphicsMsg_FromRga_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::Graphics {
    class GraphicsMsg_FromRgaBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        GraphicsMsg_FromRgaBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 RgaResponse 消息
        LockGuard BuildRgaResponse(uint32_t job_id, uint32_t err_code) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaResponse(fbb_, job_id, err_code);
            auto root = CreateFromRgaEnvelope(fbb_, FromRgaPayload_RgaResponse, table.Union());
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