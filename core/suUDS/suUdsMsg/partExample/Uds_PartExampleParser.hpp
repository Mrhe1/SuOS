#pragma once
#ifndef UDS_PARTEXAMPLE_PARSER_HPP
#define UDS_PARTEXAMPLE_PARSER_HPP
#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::PartExample {
    class PartExampleParser {
    public:
        using NoArgsCallback = std::function<void()>;
        using CmdRequestCallback = std::function<void(uint32_t seq_no, const std::vector<uint8_t>& parameters)>;
        using CmdResponseCallback = std::function<void(uint32_t seq_no, uint32_t result_code, const std::vector<uint8_t>& data)>;
        using EventNotifyCallback = std::function<void(uint32_t event_id, uint32_t severity, const std::vector<uint8_t>& detail)>;
        using ErrorResponseCallback = std::function<void(uint32_t seq_no, uint32_t error_code, const std::string& error_msg)>;
        struct Callbacks {
            NoArgsCallback onNoArgs;
            CmdRequestCallback onCmdRequest;
            CmdResponseCallback onCmdResponse;
            EventNotifyCallback onEventNotify;
            ErrorResponseCallback onErrorResponse;
        };
        PartExampleParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyPayloadEnvelopeBuffer(v)) return;
            auto env = GetPayloadEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case UdsPayload_NoArgs: {
                    if (_callbacks.onNoArgs) {
                        auto table = static_cast<const NoArgs*>(payload);
                        (void)table;
                        _callbacks.onNoArgs();
                    }
                    break;
                }
                case UdsPayload_CmdRequest: {
                    if (_callbacks.onCmdRequest) {
                        auto table = static_cast<const CmdRequest*>(payload);
                        (void)table;
                    auto seq_no_val = table->seq_no();
                    std::vector<uint8_t> parameters_val;
                    if (table->parameters()) parameters_val.assign(table->parameters()->begin(), table->parameters()->end());
                        _callbacks.onCmdRequest(seq_no_val, parameters_val);
                    }
                    break;
                }
                case UdsPayload_CmdResponse: {
                    if (_callbacks.onCmdResponse) {
                        auto table = static_cast<const CmdResponse*>(payload);
                        (void)table;
                    auto seq_no_val = table->seq_no();
                    auto result_code_val = table->result_code();
                    std::vector<uint8_t> data_val;
                    if (table->data()) data_val.assign(table->data()->begin(), table->data()->end());
                        _callbacks.onCmdResponse(seq_no_val, result_code_val, data_val);
                    }
                    break;
                }
                case UdsPayload_EventNotify: {
                    if (_callbacks.onEventNotify) {
                        auto table = static_cast<const EventNotify*>(payload);
                        (void)table;
                    auto event_id_val = table->event_id();
                    auto severity_val = table->severity();
                    std::vector<uint8_t> detail_val;
                    if (table->detail()) detail_val.assign(table->detail()->begin(), table->detail()->end());
                        _callbacks.onEventNotify(event_id_val, severity_val, detail_val);
                    }
                    break;
                }
                case UdsPayload_ErrorResponse: {
                    if (_callbacks.onErrorResponse) {
                        auto table = static_cast<const ErrorResponse*>(payload);
                        (void)table;
                    auto seq_no_val = table->seq_no();
                    auto error_code_val = table->error_code();
                    std::string error_msg_val = table->error_msg() ? table->error_msg()->str() : "";
                        _callbacks.onErrorResponse(seq_no_val, error_code_val, error_msg_val);
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