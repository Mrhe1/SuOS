// AUTO GENERATED DO NOT EDIT
#pragma once
#ifndef UDS_GENERALMSG_FROMMAIN_PARSER_HPP
#define UDS_GENERALMSG_FROMMAIN_PARSER_HPP
#include "GeneralMsg_fromMain_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace SuOS::Uds::Msg::General {
    class GeneralMsg_fromMainParser {
    public:
        using UsrStopRequestCallback = std::function<void(uint32_t usr, uint32_t part)>;
        using UsrStopKillCallback = std::function<void(uint32_t usr, uint32_t part)>;
        struct Callbacks {
            UsrStopRequestCallback onUsrStopRequest;
            UsrStopKillCallback onUsrStopKill;
        };
        GeneralMsg_fromMainParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size, uint32_t usr = 0, uint32_t part = 0) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyGeneralEnvelope_fromMainBuffer(v)) return;
            auto env = GetGeneralEnvelope_fromMain(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case GeneralPayload_fromMain_UsrStopRequest: {
                    if (_callbacks.onUsrStopRequest) {
                        auto table = static_cast<const UsrStopRequest*>(payload);
                        (void)table;
                        _callbacks.onUsrStopRequest(usr, part);
                    }
                    break;
                }
                case GeneralPayload_fromMain_UsrStopKill: {
                    if (_callbacks.onUsrStopKill) {
                        auto table = static_cast<const UsrStopKill*>(payload);
                        (void)table;
                        _callbacks.onUsrStopKill(usr, part);
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