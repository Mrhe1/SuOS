// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_ROUTERMSG_FROMROUTER_PARSER_HPP
#define UDS_ROUTERMSG_FROMROUTER_PARSER_HPP
#include "RouterMsg_fromRouter_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::Router {
    class RouterMsg_fromRouterParser {
    public:
        using onConnectCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t id)>;
        using onConnectionLostCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t id)>;
        struct Callbacks {
            onConnectCallback ononConnect;
            onConnectionLostCallback ononConnectionLost;
        };
        RouterMsg_fromRouterParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size, uint32_t usr = 0, uint32_t part = 0) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyRouterEnvelope_fromRouterBuffer(v)) return;
            auto env = GetRouterEnvelope_fromRouter(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case RouterPayload_fromRouter_onConnect: {
                    if (_callbacks.ononConnect) {
                        auto table = static_cast<const onConnect*>(payload);
                        (void)table;
                    auto id_val = table->id();
                        _callbacks.ononConnect(usr, part, id_val);
                    }
                    break;
                }
                case RouterPayload_fromRouter_onConnectionLost: {
                    if (_callbacks.ononConnectionLost) {
                        auto table = static_cast<const onConnectionLost*>(payload);
                        (void)table;
                    auto id_val = table->id();
                        _callbacks.ononConnectionLost(usr, part, id_val);
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