#pragma once
#ifndef UDS_PARTEXAMPLE_PARSER_HPP
#define UDS_PARTEXAMPLE_PARSER_HPP
#include "suUdsPartExample_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
namespace SuOS::Uds::Msg::TestUniversal {
    class PartExampleParser {
    public:
        using ScalarTableCallback = std::function<void(const ScalarTable*)>;
        using DataContainerCallback = std::function<void(const DataContainer*)>;
        using EmptySignalCallback = std::function<void(const EmptySignal*)>;
        using ConfigInfoCallback = std::function<void(const ConfigInfo*)>;
        struct Callbacks {
            ScalarTableCallback onScalarTable;
            DataContainerCallback onDataContainer;
            EmptySignalCallback onEmptySignal;
            ConfigInfoCallback onConfigInfo;
        };
        PartExampleParser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {}
        void Parse(const uint8_t* buffer, size_t size) {
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!VerifyPayloadEnvelopeBuffer(v)) return;
            auto env = GetPayloadEnvelope(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {
                case UdsPayload_ScalarTable:
                    if (_callbacks.onScalarTable) _callbacks.onScalarTable(static_cast<const ScalarTable*>(payload));
                    break;
                case UdsPayload_DataContainer:
                    if (_callbacks.onDataContainer) _callbacks.onDataContainer(static_cast<const DataContainer*>(payload));
                    break;
                case UdsPayload_EmptySignal:
                    if (_callbacks.onEmptySignal) _callbacks.onEmptySignal(static_cast<const EmptySignal*>(payload));
                    break;
                case UdsPayload_ConfigInfo:
                    if (_callbacks.onConfigInfo) _callbacks.onConfigInfo(static_cast<const ConfigInfo*>(payload));
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