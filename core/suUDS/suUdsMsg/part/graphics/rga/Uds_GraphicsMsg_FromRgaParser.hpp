#pragma once
#ifndef UDS_GRAPHICSMSG_FROMRGA_PARSER_HPP
#define UDS_GRAPHICSMSG_FROMRGA_PARSER_HPP
#include "GraphicsMsg_FromRga_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::Graphics {
    class GraphicsMsg_FromRgaParser {
    public:
        using RgaResponseCallback = std::function<void(uint32_t job_id, uint32_t err_code)>;
        struct Callbacks {
            RgaResponseCallback onRgaResponse;
        };
        GraphicsMsg_FromRgaParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyFromRgaEnvelopeBuffer(v)) return;
            auto env = GetFromRgaEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case FromRgaPayload_RgaResponse: {
                    if (_callbacks.onRgaResponse) {
                        auto table = static_cast<const RgaResponse*>(payload);
                        (void)table;
                    auto job_id_val = table->job_id();
                    auto err_code_val = table->err_code();
                        _callbacks.onRgaResponse(job_id_val, err_code_val);
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