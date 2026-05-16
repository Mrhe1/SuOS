// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_GENERALMSG_TOMAIN_PARSER_HPP
#define UDS_GENERALMSG_TOMAIN_PARSER_HPP
#include "GeneralMsg_toMain_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::General {
    class GeneralMsg_toMainParser {
    public:
        using UsrStopResponseCallback = std::function<void(uint32_t usr, uint32_t part)>;
        struct Callbacks {
            UsrStopResponseCallback onUsrStopResponse;
        };
        GeneralMsg_toMainParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size, uint32_t usr = 0, uint32_t part = 0) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyGeneralEnvelope_toMainBuffer(v)) return;
            auto env = GetGeneralEnvelope_toMain(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case GeneralPayload_toMain_UsrStopResponse: {
                    if (_callbacks.onUsrStopResponse) {
                        auto table = static_cast<const UsrStopResponse*>(payload);
                        (void)table;
                        _callbacks.onUsrStopResponse(usr, part);
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