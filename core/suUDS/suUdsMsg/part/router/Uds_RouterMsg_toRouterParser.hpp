// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_ROUTERMSG_TOROUTER_PARSER_HPP
#define UDS_ROUTERMSG_TOROUTER_PARSER_HPP
#include "RouterMsg_toRouter_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::Router {
    class RouterMsg_toRouterParser {
    public:
        using enableAppRegisterCallback = std::function<void(uint32_t usr, uint32_t part, uint32_t app_id)>;
        struct Callbacks {
            enableAppRegisterCallback onenableAppRegister;
        };
        RouterMsg_toRouterParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size, uint32_t usr = 0, uint32_t part = 0) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyRouterEnvelope_toRouterBuffer(v)) return;
            auto env = GetRouterEnvelope_toRouter(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case RouterPayload_toRouter_enableAppRegister: {
                    if (_callbacks.onenableAppRegister) {
                        auto table = static_cast<const enableAppRegister*>(payload);
                        (void)table;
                    auto app_id_val = table->app_id();
                        _callbacks.onenableAppRegister(usr, part, app_id_val);
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