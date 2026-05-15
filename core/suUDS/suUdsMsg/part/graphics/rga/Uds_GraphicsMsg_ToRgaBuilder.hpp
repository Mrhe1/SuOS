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
        // 构建 RgaCopy 消息
        LockGuard BuildRgaCopy(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaCopy(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaCopy, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaRotate 消息
        LockGuard BuildRgaRotate(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int angle) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaRotate(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg, angle);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaRotate, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaFill 消息
        LockGuard BuildRgaFill(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, uint32_t color) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaFill(fbb_, dst_fd, &dst_cfg, color);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaFill, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaBlend 消息
        LockGuard BuildRgaBlend(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaBlend(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaBlend, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaOsd 消息
        LockGuard BuildRgaOsd(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& dst_cfg, const ImOsd& osd_cfg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaOsd(fbb_, src_fd, dst_fd, &dst_cfg, &osd_cfg);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaOsd, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaResize 消息
        LockGuard BuildRgaResize(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, double fx, double fy) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaResize(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg, fx, fy);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaResize, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaCrop 消息
        LockGuard BuildRgaCrop(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaCrop(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaCrop, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaConvert 消息
        LockGuard BuildRgaConvert(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaConvert(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaConvert, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaFlip 消息
        LockGuard BuildRgaFlip(uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaFlip(fbb_, src_fd, dst_fd, &src_cfg, &dst_cfg, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaFlip, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaComposite 消息
        LockGuard BuildRgaComposite(uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, uint64_t dst_fd, const RgaConfig& fg_cfg, const RgaConfig& bg_cfg, const RgaConfig& dst_cfg, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaComposite(fbb_, fg_fd, bg_fd, dst_fd, &fg_cfg, &bg_cfg, &dst_cfg, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaComposite, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaColorKey 消息
        LockGuard BuildRgaColorKey(uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, const RgaConfig& fg_cfg, const RgaConfig& bg_dst_cfg, const ImColorKeyRange& range, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaColorKey(fbb_, fg_fd, bg_fd, &fg_cfg, &bg_dst_cfg, &range, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaColorKey, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaMosaic 消息
        LockGuard BuildRgaMosaic(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaMosaic(fbb_, dst_fd, &dst_cfg, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaMosaic, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaRectangle 消息
        LockGuard BuildRgaRectangle(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, uint32_t color, int thickness) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();

            auto table = CreateRgaRectangle(fbb_, dst_fd, &dst_cfg, color, thickness);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaRectangle, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaFillArray 消息
        LockGuard BuildRgaFillArray(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, uint32_t color) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto rects__ = fbb_.CreateVectorOfStructs(rects);
            auto table = CreateRgaFillArray(fbb_, dst_fd, &dst_cfg, rects__, color);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaFillArray, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaRectangleArray 消息
        LockGuard BuildRgaRectangleArray(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, uint32_t color, int thickness) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto rects__ = fbb_.CreateVectorOfStructs(rects);
            auto table = CreateRgaRectangleArray(fbb_, dst_fd, &dst_cfg, rects__, color, thickness);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaRectangleArray, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }
        // 构建 RgaMosaicArray 消息
        LockGuard BuildRgaMosaicArray(uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, int mode) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
            auto rects__ = fbb_.CreateVectorOfStructs(rects);
            auto table = CreateRgaMosaicArray(fbb_, dst_fd, &dst_cfg, rects__, mode);
            auto root = CreateToRgaEnvelope(fbb_, job_id, ToRgaPayload_RgaMosaicArray, table.Union());
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