#pragma once

#ifndef UDS_PART_EXAMPLE_PARSER_HPP
#define UDS_PART_EXAMPLE_PARSER_HPP

#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <stdexcept>

namespace SuOS::Uds::Msg::PartExample {

    class PartExampleParser {
    public:
        // 回调定义
        using NoArgsCallback = std::function<void(const NoArgs*)>;
        using CmdRequestCallback = std::function<void(const CmdRequest*)>;
        using CmdResponseCallback = std::function<void(const CmdResponse*)>;
        using EventNotifyCallback = std::function<void(const EventNotify*)>;
        using ErrorResponseCallback = std::function<void(const ErrorResponse*)>;

        struct Callbacks {
            NoArgsCallback onNoArgs;
            CmdRequestCallback onCmdRequest;
            CmdResponseCallback onCmdResponse;
            EventNotifyCallback onEventNotify;
            ErrorResponseCallback onErrorResponse;
        };

        PartExampleParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) 
            : _runtime(runtime), _callbacks(cbs) {}

        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");

            flatbuffers::Verifier verifier(buffer, size);
            if (!VerifyPayloadEnvelopeBuffer(verifier)) {
                return;
            }

            auto envelope = GetPayloadEnvelope(buffer);
            auto payload_type = envelope->payload_type();
            auto payload = envelope->payload();

            switch (payload_type) {
                case UdsPayload_NoArgs:
                    if (_callbacks.onNoArgs) _callbacks.onNoArgs(static_cast<const NoArgs*>(payload));
                    break;
                case UdsPayload_CmdRequest:
                    if (_callbacks.onCmdRequest) _callbacks.onCmdRequest(static_cast<const CmdRequest*>(payload));
                    break;
                case UdsPayload_CmdResponse:
                    if (_callbacks.onCmdResponse) _callbacks.onCmdResponse(static_cast<const CmdResponse*>(payload));
                    break;
                case UdsPayload_EventNotify:
                    if (_callbacks.onEventNotify) _callbacks.onEventNotify(static_cast<const EventNotify*>(payload));
                    break;
                case UdsPayload_ErrorResponse:
                    if (_callbacks.onErrorResponse) _callbacks.onErrorResponse(static_cast<const ErrorResponse*>(payload));
                    break;
                default:
                    break;
            }
        }

    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        Callbacks _callbacks;
    };

} // namespace SuOS::Uds::Msg::PartExample

#endif // UDS_PART_EXAMPLE_PARSER_HPP
