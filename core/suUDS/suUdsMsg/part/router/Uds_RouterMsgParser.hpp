#pragma once
#ifndef UDS_ROUTERMSG_PARSER_HPP
#define UDS_ROUTERMSG_PARSER_HPP
#include "RouterMsg_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
namespace SuOS::Uds::Msg::Router {
    class RouterMsgParser {
    public:
        using enableAppRegisterCallback = std::function<void(const enableAppRegister*)>;
        using onConnectCallback = std::function<void(const onConnect*)>;
        using onConnectionLostCallback = std::function<void(const onConnectionLost*)>;
        struct Callbacks {
            enableAppRegisterCallback onenableAppRegister;
            onConnectCallback ononConnect;
            onConnectionLostCallback ononConnectionLost;
        };
        RouterMsgParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyRouterEnvelopeBuffer(v)) return;
            auto env = GetRouterEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case RouterPayload_enableAppRegister:
                    if (_callbacks.onenableAppRegister) _callbacks.onenableAppRegister(static_cast<const enableAppRegister*>(payload));
                    break;
                case RouterPayload_onConnect:
                    if (_callbacks.ononConnect) _callbacks.ononConnect(static_cast<const onConnect*>(payload));
                    break;
                case RouterPayload_onConnectionLost:
                    if (_callbacks.ononConnectionLost) _callbacks.ononConnectionLost(static_cast<const onConnectionLost*>(payload));
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