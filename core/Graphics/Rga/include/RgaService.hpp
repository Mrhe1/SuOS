#pragma once
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include "suRuntime.hpp"
#include "SuOS_Config.h"
#include "Uds_GraphicsMsg_ToRgaParser.hpp"
#include "Uds_GraphicsMsg_FromRgaBuilder.hpp"
#include "RgaEngine.hpp"
#include "VramProvider.hpp"

namespace SuOS::Uds::Msg::Graphics {

class RgaService {
public:
    using SendFunc = std::function<void(uint32_t sender_part, uint32_t receiver_usr, uint32_t receiver_part, const std::vector<uint8_t>& payload)>;

    RgaService(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_cb)
        : _runtime(runtime), _send_cb(send_cb), _builder(runtime) 
    {
        GraphicsMsg_ToRgaParser::Callbacks cbs;
        cbs.onRgaCopy = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addCopy(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()});
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaRotate = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int angle) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addRotate(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, angle);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaFill = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, uint32_t color) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                chain.addFill(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, color);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaBlend = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addBlend(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaOsd = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& dst_cfg, const ImOsd& osd_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                im_osd_t params; 
                memset(&params, 0, sizeof(params));
                params.osd_mode = osd_cfg.osd_mode();
                // block_size and flags are not in im_osd_t but in our fbs ImOsd
                // Usually we map what's available in rga.h
                chain.addOsd(s, d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, params);
            }, src_fd, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaResize = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, double fx, double fy) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addResize(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, fx, fy);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaCrop = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addCrop(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()});
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaConvert = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addConvert(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaFlip = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                chain.addFlip(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.x_offset(), src_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaComposite = [this](uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, uint64_t dst_fd, const RgaConfig& fg_cfg, const RgaConfig& bg_cfg, const RgaConfig& dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addComposite(s, p, d, 
                    {fg_cfg.width(), fg_cfg.height(), fg_cfg.format(), fg_cfg.x_offset(), fg_cfg.y_offset()},
                    {bg_cfg.width(), bg_cfg.height(), bg_cfg.format(), bg_cfg.x_offset(), bg_cfg.y_offset()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, mode);
            }, fg_fd, dst_fd, bg_fd, fg_cfg, dst_cfg, bg_cfg);
        };
        cbs.onRgaColorKey = [this](uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, const RgaConfig& fg_cfg, const RgaConfig& bg_dst_cfg, const ImColorKeyRange& range, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)p;
                im_colorkey_range r;
                r.max = range.max();
                r.min = range.min();
                chain.addColorKey(s, d, 
                    {fg_cfg.width(), fg_cfg.height(), fg_cfg.format(), fg_cfg.x_offset(), fg_cfg.y_offset()},
                    {bg_dst_cfg.width(), bg_dst_cfg.height(), bg_dst_cfg.format(), bg_dst_cfg.x_offset(), bg_dst_cfg.y_offset()}, r, mode);
            }, fg_fd, bg_fd, -1, fg_cfg, bg_dst_cfg, {});
        };
        cbs.onRgaMosaic = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                chain.addMosaic(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, mode);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaRectangle = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, uint32_t color, int thickness) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                chain.addRectangle(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.x_offset(), dst_cfg.y_offset()}, color, thickness);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        
        cbs.onRgaFillArray = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, uint32_t color) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                std::vector<im_rect> r;
                for (const auto& a : rects) {
                    im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addFillArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, color);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        cbs.onRgaRectangleArray = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, uint32_t color, int thickness) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                std::vector<im_rect> r;
                for (const auto& a : rects) {
                    im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addRectangleArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, color, thickness);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        cbs.onRgaMosaicArray = [this](uint32_t job_id, uint64_t dst_fd, const RgaConfig& dst_cfg, const std::vector<ImRect>& rects, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                (void)s; (void)p;
                std::vector<im_rect> r;
                for (const auto& a : rects) {
                    im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addMosaicArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, mode);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        _parser = std::make_unique<GraphicsMsg_ToRgaParser>(_runtime, cbs);
    }

    void onMessage(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_part, const std::vector<uint8_t>& payload) {
        if (_parser) {
            _active_requesters[receiver_part] = {sender_usr, sender_part};
            _parser->Parse(payload.data(), payload.size());
        }
    }

private:
    struct Requester {
        uint32_t usr;
        uint32_t part;
    };

    using ChainBuilder = std::function<void(SuOS::graphics::RgaChain&, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>)>;

    void handleRgaGeneric(uint32_t job_id, ChainBuilder cb, 
                         int64_t s_id, int64_t d_id, int64_t p_id,
                         RgaConfig const& s_cfg, RgaConfig const& d_cfg, RgaConfig const& p_cfg) 
    {
        (void)s_cfg; (void)d_cfg; (void)p_cfg;
        using namespace SuOS::graphics;
        
        std::shared_ptr<VBuffer> s, d, p;
        if (s_id >= 0) s = VramProvider::getInstance().getSharedBuffer(s_id);
        if (d_id >= 0) d = VramProvider::getInstance().getSharedBuffer(d_id);
        if (p_id >= 0) p = VramProvider::getInstance().getSharedBuffer(p_id);

        RgaChain chain;
        cb(chain, s, d, p);

        _runtime->dispatch([this, chain, job_id, s, d, p]() mutable {
            RgaEngine::getInstance().submit(chain, [this, job_id, s, d, p](uint32_t err_code) {
                this->sendResponse(job_id, err_code);
            });
        });
    }

    void sendResponse(uint32_t job_id, uint32_t err_code) {
        auto guard = _builder.BuildRgaResponse(job_id, err_code);
        std::vector<uint8_t> payload(guard.data(), guard.data() + guard.size());
        
        // Find requester based on job_id or other mechanism
        if (!_active_requesters.empty()) {
            auto const& req = _active_requesters.begin()->second; // Simplified demo
            if (_send_cb) {
                _send_cb(SuOS::Config::Part::DISPLAY, req.usr, req.part, payload);
            }
        }
    }

    std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    SendFunc _send_cb;
    GraphicsMsg_FromRgaBuilder _builder;
    std::unique_ptr<GraphicsMsg_ToRgaParser> _parser;
    std::map<uint32_t, Requester> _active_requesters;
};

} // namespace SuOS::Uds::Msg::Graphics
