#pragma once
#ifndef UDS_GRAPHICSMSG_TORGA_BUILDER_HPP
#define UDS_GRAPHICSMSG_TORGA_BUILDER_HPP
#include "GraphicsMsg_ToRga_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace SuOS::Uds::Msg::Graphics {
    class GraphicsMsg_ToRgaBuilder {
    public:
        class LockGuard {
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {}
            const uint8_t* data() const { return _fbb.GetBufferPointer(); }
            size_t size() const { return _fbb.GetSize(); }
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        };
        GraphicsMsg_ToRgaBuilder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {}
        // 构建 RgaRequest 消息
        LockGuard BuildRgaRequest(uint32_t job_id, int8_t type, int src_fd, int src_w, int src_h, int src_fmt, int src_x, int src_y, int dst_fd, int dst_w, int dst_h, int dst_fmt, int dst_x, int dst_y, int pat_fd, int pat_w, int pat_h, int pat_fmt, int pat_x, int pat_y, int angle, uint color, int mode, double fx, double fy) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaRequest(fbb_, job_id, type, src_fd, src_w, src_h, src_fmt, src_x, src_y, dst_fd, dst_w, dst_h, dst_fmt, dst_x, dst_y, pat_fd, pat_w, pat_h, pat_fmt, pat_x, pat_y, angle, color, mode, fx, fy);
            auto root = CreateToRgaEnvelope(fbb_, ToRgaPayload_RgaRequest, table.Union());
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