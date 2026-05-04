#pragma once
#ifndef UDS_GENERALMSG_PARSER_HPP
#define UDS_GENERALMSG_PARSER_HPP
#include "GeneralMsg_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
namespace SuOS::Uds::Msg::General {
    class GeneralMsgParser {
    public:
        using HeartbeatCallback = std::function<void(const Heartbeat*)>;
        using UsrStopRequestCallback = std::function<void(const UsrStopRequest*)>;
        using UsrStopResponseCallback = std::function<void(const UsrStopResponse*)>;
        using UsrStopKillCallback = std::function<void(const UsrStopKill*)>;
        struct Callbacks {
            HeartbeatCallback onHeartbeat;
            UsrStopRequestCallback onUsrStopRequest;
            UsrStopResponseCallback onUsrStopResponse;
            UsrStopKillCallback onUsrStopKill;
        };
        GeneralMsgParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyGeneralEnvelopeBuffer(v)) return;
            auto env = GetGeneralEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case GeneralPayload_Heartbeat:
                    if (_callbacks.onHeartbeat) _callbacks.onHeartbeat(static_cast<const Heartbeat*>(payload));
                    break;
                case GeneralPayload_UsrStopRequest:
                    if (_callbacks.onUsrStopRequest) _callbacks.onUsrStopRequest(static_cast<const UsrStopRequest*>(payload));
                    break;
                case GeneralPayload_UsrStopResponse:
                    if (_callbacks.onUsrStopResponse) _callbacks.onUsrStopResponse(static_cast<const UsrStopResponse*>(payload));
                    break;
                case GeneralPayload_UsrStopKill:
                    if (_callbacks.onUsrStopKill) _callbacks.onUsrStopKill(static_cast<const UsrStopKill*>(payload));
                    break;
                default: break;
            }
        }
    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        Callbacks _callbacks;
    };
}
#endif