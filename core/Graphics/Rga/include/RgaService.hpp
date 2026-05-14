#pragma once
#include <memory>
#include <functional>
#include "suRuntime.hpp"
#include "Uds_GraphicsMsg_ToRgaParser.hpp"
#include "Uds_GraphicsMsg_FromRgaBuilder.hpp"
#include "RgaEngine.hpp"
#include "VramProvider.hpp"

namespace SuOS::Uds::Msg::Graphics {

class RgaService {
public:
    using SendFunc = std::function<void(const uint8_t* data, size_t size)>;

    RgaService(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_cb)
        : _runtime(runtime), _send_cb(send_cb), _builder(runtime) 
    {
        GraphicsMsg_ToRgaParser::Callbacks cbs;
        cbs.onRgaCopy = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addCopy(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()});
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaRotate = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int angle) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addRotate(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, angle);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaFill = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, uint32_t color) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addFill(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, color);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaBlend = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addBlend(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaOsd = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig dst_cfg, ImOsd osd_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                SuOS::graphics::im_osd_t params; 
                memset(&params, 0, sizeof(params));
                chain.addOsd(s, d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, params);
            }, src_fd, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaResize = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, double fx, double fy) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addResize(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, fx, fy);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaCrop = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addCrop(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()});
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaConvert = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addConvert(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaFlip = [this](uint32_t job_id, uint64_t src_fd, uint64_t dst_fd, RgaConfig src_cfg, RgaConfig dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addFlip(s, d, 
                    {src_cfg.width(), src_cfg.height(), src_cfg.format(), src_cfg.offset_x(), src_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, mode);
            }, src_fd, dst_fd, -1, src_cfg, dst_cfg, {});
        };
        cbs.onRgaComposite = [this](uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, uint64_t dst_fd, RgaConfig fg_cfg, RgaConfig bg_cfg, RgaConfig dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addComposite(s, p, d, 
                    {fg_cfg.width(), fg_cfg.height(), fg_cfg.format(), fg_cfg.offset_x(), fg_cfg.offset_y()},
                    {bg_cfg.width(), bg_cfg.height(), bg_cfg.format(), bg_cfg.offset_x(), bg_cfg.offset_y()},
                    {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, mode);
            }, fg_fd, dst_fd, bg_fd, fg_cfg, dst_cfg, bg_cfg);
        };
        cbs.onRgaColorKey = [this](uint32_t job_id, uint64_t fg_fd, uint64_t bg_fd, RgaConfig fg_cfg, RgaConfig bg_dst_cfg, ImColorKeyRange range, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                SuOS::graphics::im_colorkey_range r;
                r.max = range.max();
                r.min = range.min();
                chain.addColorKey(s, d, 
                    {fg_cfg.width(), fg_cfg.height(), fg_cfg.format(), fg_cfg.offset_x(), fg_cfg.offset_y()},
                    {bg_dst_cfg.width(), bg_dst_cfg.height(), bg_dst_cfg.format(), bg_dst_cfg.offset_x(), bg_dst_cfg.offset_y()}, r, mode);
            }, fg_fd, bg_fd, -1, fg_cfg, bg_dst_cfg, {});
        };
        cbs.onRgaMosaic = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addMosaic(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, mode);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        cbs.onRgaRectangle = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, uint32_t color, int thickness) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                chain.addRectangle(d, {dst_cfg.width(), dst_cfg.height(), dst_cfg.format(), dst_cfg.offset_x(), dst_cfg.offset_y()}, color, thickness);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };
        
        cbs.onRgaFillArray = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                std::vector<SuOS::graphics::im_rect> r;
                for (const auto& a : rects) {
                    SuOS::graphics::im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addFillArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, color);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        cbs.onRgaRectangleArray = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, uint32_t color, int thickness) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                std::vector<SuOS::graphics::im_rect> r;
                for (const auto& a : rects) {
                    SuOS::graphics::im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addRectangleArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, color, thickness);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        cbs.onRgaMosaicArray = [this](uint32_t job_id, uint64_t dst_fd, RgaConfig dst_cfg, const std::vector<ImRect>& rects, int mode) {
            handleRgaGeneric(job_id, [=](SuOS::graphics::RgaChain& chain, auto s, auto d, auto p) {
                std::vector<SuOS::graphics::im_rect> r;
                for (const auto& a : rects) {
                    SuOS::graphics::im_rect ir;
                    ir.x = a.x(); ir.y = a.y(); ir.width = a.width(); ir.height = a.height();
                    r.push_back(ir);
                }
                chain.addMosaicArray(d, dst_cfg.format(), dst_cfg.width(), dst_cfg.height(), r, mode);
            }, -1, dst_fd, -1, {}, dst_cfg, {});
        };

        _parser = std::make_unique<GraphicsMsg_ToRgaParser>(_runtime, cbs);
    }

    void onMessage(const uint8_t* data, size_t size) {
        if (_parser) {
            _parser->Parse(data, size);
        }
    }

private:
    using ChainBuilder = std::function<void(SuOS::graphics::RgaChain&, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>, 
                                          std::shared_ptr<SuOS::graphics::VBuffer>)>;

    void handleRgaGeneric(uint32_t job_id, ChainBuilder cb, 
                         int64_t s_fd, int64_t d_fd, int64_t p_fd,
                         RgaConfig s_cfg, RgaConfig d_cfg, RgaConfig p_cfg) 
    {
        using namespace SuOS::graphics;
        auto usr = VramProvider::getInstance().getOrCreateUsr("RgaUdsService_Imports");
        
        std::shared_ptr<VBuffer> s, d, p;
        if (s_fd >= 0) s = usr->importBuffer(s_fd, s_cfg.width(), s_cfg.height(), s_cfg.format(), s_cfg.width() * 4, s_cfg.width() * s_cfg.height() * 4);
        if (d_fd >= 0) d = usr->importBuffer(d_fd, d_cfg.width(), d_cfg.height(), d_cfg.format(), d_cfg.width() * 4, d_cfg.width() * d_cfg.height() * 4);
        if (p_fd >= 0) p = usr->importBuffer(p_fd, p_cfg.width(), p_cfg.height(), p_cfg.format(), p_cfg.width() * 4, p_cfg.width() * p_cfg.height() * 4);

        RgaChain chain;
        cb(chain, s, d, p);

        _runtime->postTask([this, chain, job_id, s, d, p]() {
            RgaEngine::getInstance().submit(chain, [this, job_id, s, d, p](uint32_t err_code) {
                this->postResponse(job_id, err_code);
            });
        });
    }

    void postResponse(uint32_t job_id, uint32_t err_code) {
        if (_runtime->isInEventLoop()) {
            sendResponse(job_id, err_code);
        } else {
            _runtime->postTask([this, job_id, err_code]() {
                sendResponse(job_id, err_code);
            });
        }
    }
    
    void sendResponse(uint32_t job_id, uint32_t err_code) {
        auto guard = _builder.BuildRgaResponse(job_id, err_code);
        if (_send_cb) {
            _send_cb(guard.data(), guard.size());
        }
    }

    std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    SendFunc _send_cb;
    GraphicsMsg_FromRgaBuilder _builder;
    std::unique_ptr<GraphicsMsg_ToRgaParser> _parser;
};

} // namespace SuOS::Uds::Msg::Graphics
