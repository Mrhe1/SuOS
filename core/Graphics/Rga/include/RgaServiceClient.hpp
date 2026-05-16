
#pragma once
#include <memory>
#include <functional>
#include <map>
#include <random>
#include <vector>
#include "suRuntime.hpp"
#include "SuOS_Config.h"
#include "Uds_GraphicsMsg_ToRgaBuilder.hpp"
#include "Uds_GraphicsMsg_FromRgaParser.hpp"

namespace SuOS::Uds::Msg::Graphics {

class RgaServiceClient {
public:
    using SendFunc = std::function<void(uint32_t sender_part, uint32_t receiver_usr, uint32_t receiver_part, const std::vector<uint8_t>& payload)>;

    using JobCallback = std::function<void(uint32_t err_code)>;

    RgaServiceClient(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_cb)
        : _runtime(runtime), _send_cb(send_cb), _builder(runtime) 
    {
        _gen.seed(_rd());
        GraphicsMsg_FromRgaParser::Callbacks cbs;

        cbs.onRgaResponse = [this](uint32_t s, uint32_t p, uint32_t job_id, uint32_t err_code) {
            (void)s; (void)p;
            auto it = _pending_jobs.find(job_id);
            if (it != _pending_jobs.end()) {
                if (it->second) it->second(err_code);
                _pending_jobs.erase(it);
            }
        };
        _parser = std::make_unique<GraphicsMsg_FromRgaParser>(_runtime, cbs);
    }

    void onMessage(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_part, const std::vector<uint8_t>& payload) {
        (void)sender_usr; (void)sender_part; (void)receiver_part;
        if (_parser) {
            _parser->Parse(payload.data(), payload.size());
        }
    }

    // --- RGA Operations ---

    void sendRgaCopy(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg]() {
            auto guard = _builder.BuildRgaCopy(job_id, src_id, dst_id, src_cfg, dst_cfg);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaRotate(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, int angle, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg, angle]() {
            auto guard = _builder.BuildRgaRotate(job_id, src_id, dst_id, src_cfg, dst_cfg, angle);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaFill(uint64_t dst_id, RgaConfig dst_cfg, uint32_t color, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, color]() {
            auto guard = _builder.BuildRgaFill(job_id, dst_id, dst_cfg, color);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaBlend(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg, mode]() {
            auto guard = _builder.BuildRgaBlend(job_id, src_id, dst_id, src_cfg, dst_cfg, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaOsd(uint64_t src_id, uint64_t dst_id, RgaConfig dst_cfg, ImOsd osd_cfg, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, dst_cfg, osd_cfg]() {
            auto guard = _builder.BuildRgaOsd(job_id, src_id, dst_id, dst_cfg, osd_cfg);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaResize(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, double fx, double fy, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg, fx, fy]() {
            auto guard = _builder.BuildRgaResize(job_id, src_id, dst_id, src_cfg, dst_cfg, fx, fy);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaCrop(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg]() {
            auto guard = _builder.BuildRgaCrop(job_id, src_id, dst_id, src_cfg, dst_cfg);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaConvert(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg, mode]() {
            auto guard = _builder.BuildRgaConvert(job_id, src_id, dst_id, src_cfg, dst_cfg, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaFlip(uint64_t src_id, uint64_t dst_id, RgaConfig src_cfg, RgaConfig dst_cfg, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, src_id, dst_id, src_cfg, dst_cfg, mode]() {
            auto guard = _builder.BuildRgaFlip(job_id, src_id, dst_id, src_cfg, dst_cfg, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaComposite(uint64_t fg_id, uint64_t bg_id, uint64_t dst_id, RgaConfig fg_cfg, RgaConfig bg_cfg, RgaConfig dst_cfg, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, fg_id, bg_id, dst_id, fg_cfg, bg_cfg, dst_cfg, mode]() {
            auto guard = _builder.BuildRgaComposite(job_id, fg_id, bg_id, dst_id, fg_cfg, bg_cfg, dst_cfg, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaColorKey(uint64_t fg_id, uint64_t bg_id, RgaConfig fg_cfg, RgaConfig bg_dst_cfg, ImColorKeyRange range, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, fg_id, bg_id, fg_cfg, bg_dst_cfg, range, mode]() {
            auto guard = _builder.BuildRgaColorKey(job_id, fg_id, bg_id, fg_cfg, bg_dst_cfg, range, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaMosaic(uint64_t dst_id, RgaConfig dst_cfg, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, mode]() {
            auto guard = _builder.BuildRgaMosaic(job_id, dst_id, dst_cfg, mode);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaRectangle(uint64_t dst_id, RgaConfig dst_cfg, uint32_t color, int thickness, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, color, thickness]() {
            auto guard = _builder.BuildRgaRectangle(job_id, dst_id, dst_cfg, color, thickness);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaFillArray(uint64_t dst_id, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, rects, color]() {
            auto guard = _builder.BuildRgaFillArray(job_id, dst_id, dst_cfg, rects, color);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaRectangleArray(uint64_t dst_id, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color, int thickness, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, rects, color, thickness]() {
            auto guard = _builder.BuildRgaRectangleArray(job_id, dst_id, dst_cfg, rects, color, thickness);
            doSend(guard.data(), guard.size());
        });
    }

    void sendRgaMosaicArray(uint64_t dst_id, RgaConfig dst_cfg, const std::vector<ImRect>& rects, int mode, JobCallback cb = nullptr) {
        uint32_t job_id = generateJobId(cb);
        _runtime->dispatch([this, job_id, dst_id, dst_cfg, rects, mode]() {
            auto guard = _builder.BuildRgaMosaicArray(job_id, dst_id, dst_cfg, rects, mode);
            doSend(guard.data(), guard.size());
        });
    }

private:
    uint32_t generateJobId(JobCallback cb) {
        uint32_t job_id = _dist(_gen);
        if (cb) _pending_jobs[job_id] = cb;
        return job_id;
    }

    void doSend(const uint8_t* data, size_t size) {
        if (_send_cb) {
            std::vector<uint8_t> payload(data, data + size);
            _send_cb(_part_id, SuOS::Config::Usr::GRAPHIC, SuOS::Config::Part::DISPLAY_RGA, payload);
        }
    }

    std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    uint32_t _part_id = SuOS::Config::Part::APP_GRAPHICS_RGA;
    SendFunc _send_cb;
    GraphicsMsg_ToRgaBuilder _builder;
    std::unique_ptr<GraphicsMsg_FromRgaParser> _parser;

    std::map<uint32_t, JobCallback> _pending_jobs;
    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_int_distribution<uint32_t> _dist{1, 0xFFFFFFFF};
};
}