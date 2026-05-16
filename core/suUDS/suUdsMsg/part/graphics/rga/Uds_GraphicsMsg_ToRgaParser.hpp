// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_GRAPHICSMSG_TORGA_PARSER_HPP
#define UDS_GRAPHICSMSG_TORGA_PARSER_HPP
#include "GraphicsMsg_ToRga_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::Graphics {
    class GraphicsMsg_ToRgaParser {
    public:
        using RgaCopyCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg)>;
        using RgaRotateCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int angle)>;
        using RgaFillCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, uint32_t color)>;
        using RgaBlendCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode)>;
        using RgaOsdCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig dst_cfg, ImOsd osd_cfg)>;
        using RgaResizeCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, double fx, double fy)>;
        using RgaCropCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg)>;
        using RgaConvertCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode)>;
        using RgaFlipCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode)>;
        using RgaCompositeCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, uint64_t dst_fd, RgaConfig fg_cfg, RgaConfig bg_cfg, RgaConfig dst_cfg, int mode)>;
        using RgaColorKeyCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, RgaConfig fg_cfg, RgaConfig bg_dst_cfg, ImColorKeyRange range, int mode)>;
        using RgaMosaicCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, int mode)>;
        using RgaRectangleCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, uint32_t color, int thickness)>;
        using RgaFillArrayCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color)>;
        using RgaRectangleArrayCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color, int thickness)>;
        using RgaMosaicArrayCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, int mode)>;
        struct Callbacks {
            RgaCopyCallback onRgaCopy;
            RgaRotateCallback onRgaRotate;
            RgaFillCallback onRgaFill;
            RgaBlendCallback onRgaBlend;
            RgaOsdCallback onRgaOsd;
            RgaResizeCallback onRgaResize;
            RgaCropCallback onRgaCrop;
            RgaConvertCallback onRgaConvert;
            RgaFlipCallback onRgaFlip;
            RgaCompositeCallback onRgaComposite;
            RgaColorKeyCallback onRgaColorKey;
            RgaMosaicCallback onRgaMosaic;
            RgaRectangleCallback onRgaRectangle;
            RgaFillArrayCallback onRgaFillArray;
            RgaRectangleArrayCallback onRgaRectangleArray;
            RgaMosaicArrayCallback onRgaMosaicArray;
        };
        GraphicsMsg_ToRgaParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size, uint32_t usr = 0, uint32_t part = 0) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyToRgaEnvelopeBuffer(v)) return;
            auto env = GetToRgaEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case ToRgaPayload_RgaCopy: {
                    if (_callbacks.onRgaCopy) {
                        auto table = static_cast<const RgaCopy*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                        _callbacks.onRgaCopy(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaRotate: {
                    if (_callbacks.onRgaRotate) {
                        auto table = static_cast<const RgaRotate*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto angle_val = table->angle();
                        _callbacks.onRgaRotate(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val, angle_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaFill: {
                    if (_callbacks.onRgaFill) {
                        auto table = static_cast<const RgaFill*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    auto color_val = table->color();
                        _callbacks.onRgaFill(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, color_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaBlend: {
                    if (_callbacks.onRgaBlend) {
                        auto table = static_cast<const RgaBlend*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto mode_val = table->mode();
                        _callbacks.onRgaBlend(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaOsd: {
                    if (_callbacks.onRgaOsd) {
                        auto table = static_cast<const RgaOsd*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    auto osd_cfg_val = table->osd_cfg();
                        _callbacks.onRgaOsd(usr, part, job_id_val, src_fd_val, dst_fd_val, *dst_cfg_val, *osd_cfg_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaResize: {
                    if (_callbacks.onRgaResize) {
                        auto table = static_cast<const RgaResize*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto fx_val = table->fx();
                    auto fy_val = table->fy();
                        _callbacks.onRgaResize(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val, fx_val, fy_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaCrop: {
                    if (_callbacks.onRgaCrop) {
                        auto table = static_cast<const RgaCrop*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                        _callbacks.onRgaCrop(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaConvert: {
                    if (_callbacks.onRgaConvert) {
                        auto table = static_cast<const RgaConvert*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto mode_val = table->mode();
                        _callbacks.onRgaConvert(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaFlip: {
                    if (_callbacks.onRgaFlip) {
                        auto table = static_cast<const RgaFlip*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto src_fd_val = table->src_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto src_cfg_val = table->src_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto mode_val = table->mode();
                        _callbacks.onRgaFlip(usr, part, job_id_val, src_fd_val, dst_fd_val, *src_cfg_val, *dst_cfg_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaComposite: {
                    if (_callbacks.onRgaComposite) {
                        auto table = static_cast<const RgaComposite*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto fg_fd_val = table->fg_fd();
                    auto bg_fd_val = table->bg_fd();
                    auto dst_fd_val = table->dst_fd();
                    auto fg_cfg_val = table->fg_cfg();
                    auto bg_cfg_val = table->bg_cfg();
                    auto dst_cfg_val = table->dst_cfg();
                    auto mode_val = table->mode();
                        _callbacks.onRgaComposite(usr, part, job_id_val, fg_fd_val, bg_fd_val, dst_fd_val, *fg_cfg_val, *bg_cfg_val, *dst_cfg_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaColorKey: {
                    if (_callbacks.onRgaColorKey) {
                        auto table = static_cast<const RgaColorKey*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto fg_fd_val = table->fg_fd();
                    auto bg_fd_val = table->bg_fd();
                    auto fg_cfg_val = table->fg_cfg();
                    auto bg_dst_cfg_val = table->bg_dst_cfg();
                    auto range_val = table->range();
                    auto mode_val = table->mode();
                        _callbacks.onRgaColorKey(usr, part, job_id_val, fg_fd_val, bg_fd_val, *fg_cfg_val, *bg_dst_cfg_val, *range_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaMosaic: {
                    if (_callbacks.onRgaMosaic) {
                        auto table = static_cast<const RgaMosaic*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    auto mode_val = table->mode();
                        _callbacks.onRgaMosaic(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, mode_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaRectangle: {
                    if (_callbacks.onRgaRectangle) {
                        auto table = static_cast<const RgaRectangle*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    auto color_val = table->color();
                    auto thickness_val = table->thickness();
                        _callbacks.onRgaRectangle(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, color_val, thickness_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaFillArray: {
                    if (_callbacks.onRgaFillArray) {
                        auto table = static_cast<const RgaFillArray*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    std::vector<ImRect> rects_val;
                    if (auto vec_ptr = table->rects()) {
                        rects_val.reserve(vec_ptr->size());
                        for (auto item : *vec_ptr) rects_val.push_back(*item);
                    }
                    auto color_val = table->color();
                        _callbacks.onRgaFillArray(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, rects_val, color_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaRectangleArray: {
                    if (_callbacks.onRgaRectangleArray) {
                        auto table = static_cast<const RgaRectangleArray*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    std::vector<ImRect> rects_val;
                    if (auto vec_ptr = table->rects()) {
                        rects_val.reserve(vec_ptr->size());
                        for (auto item : *vec_ptr) rects_val.push_back(*item);
                    }
                    auto color_val = table->color();
                    auto thickness_val = table->thickness();
                        _callbacks.onRgaRectangleArray(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, rects_val, color_val, thickness_val);
                    }
                    break;
                }
                case ToRgaPayload_RgaMosaicArray: {
                    if (_callbacks.onRgaMosaicArray) {
                        auto table = static_cast<const RgaMosaicArray*>(payload);
                        (void)table;
                    auto job_id_val = env->job_id();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_cfg_val = table->dst_cfg();
                    std::vector<ImRect> rects_val;
                    if (auto vec_ptr = table->rects()) {
                        rects_val.reserve(vec_ptr->size());
                        for (auto item : *vec_ptr) rects_val.push_back(*item);
                    }
                    auto mode_val = table->mode();
                        _callbacks.onRgaMosaicArray(usr, part, job_id_val, dst_fd_val, *dst_cfg_val, rects_val, mode_val);
                    }
                    break;
                }
                default: break;
            }
        }
    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        Callbacks _callbacks;
    };
}
#endif