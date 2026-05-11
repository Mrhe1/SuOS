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
        using RgaRequestCallback = std::function<void(uint32_t job_id, int8_t type, int src_fd, int src_w, int src_h, int src_fmt, int src_x, int src_y, int dst_fd, int dst_w, int dst_h, int dst_fmt, int dst_x, int dst_y, int pat_fd, int pat_w, int pat_h, int pat_fmt, int pat_x, int pat_y, int angle, uint color, int mode, double fx, double fy)>;
        struct Callbacks {
            RgaRequestCallback onRgaRequest;
        };
        GraphicsMsg_ToRgaParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyToRgaEnvelopeBuffer(v)) return;
            auto env = GetToRgaEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case ToRgaPayload_RgaRequest: {
                    if (_callbacks.onRgaRequest) {
                        auto table = static_cast<const RgaRequest*>(payload);
                        (void)table;
                    auto job_id_val = table->job_id();
                    auto type_val = table->type();
                    auto src_fd_val = table->src_fd();
                    auto src_w_val = table->src_w();
                    auto src_h_val = table->src_h();
                    auto src_fmt_val = table->src_fmt();
                    auto src_x_val = table->src_x();
                    auto src_y_val = table->src_y();
                    auto dst_fd_val = table->dst_fd();
                    auto dst_w_val = table->dst_w();
                    auto dst_h_val = table->dst_h();
                    auto dst_fmt_val = table->dst_fmt();
                    auto dst_x_val = table->dst_x();
                    auto dst_y_val = table->dst_y();
                    auto pat_fd_val = table->pat_fd();
                    auto pat_w_val = table->pat_w();
                    auto pat_h_val = table->pat_h();
                    auto pat_fmt_val = table->pat_fmt();
                    auto pat_x_val = table->pat_x();
                    auto pat_y_val = table->pat_y();
                    auto angle_val = table->angle();
                    auto color_val = table->color();
                    auto mode_val = table->mode();
                    auto fx_val = table->fx();
                    auto fy_val = table->fy();
                        _callbacks.onRgaRequest(job_id_val, type_val, src_fd_val, src_w_val, src_h_val, src_fmt_val, src_x_val, src_y_val, dst_fd_val, dst_w_val, dst_h_val, dst_fmt_val, dst_x_val, dst_y_val, pat_fd_val, pat_w_val, pat_h_val, pat_fmt_val, pat_x_val, pat_y_val, angle_val, color_val, mode_val, fx_val, fy_val);
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