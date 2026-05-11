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
        cbs.onRgaRequest = [this](uint32_t job_id, int8_t type, 
                                  int src_fd, int src_w, int src_h, int src_fmt, int src_x, int src_y,
                                  int dst_fd, int dst_w, int dst_h, int dst_fmt, int dst_x, int dst_y,
                                  int pat_fd, int pat_w, int pat_h, int pat_fmt, int pat_x, int pat_y,
                                  int angle, uint32_t color, int mode, double fx, double fy) 
        {
            this->handleRgaRequest(job_id, type, src_fd, src_w, src_h, src_fmt, src_x, src_y,
                                   dst_fd, dst_w, dst_h, dst_fmt, dst_x, dst_y,
                                   pat_fd, pat_w, pat_h, pat_fmt, pat_x, pat_y,
                                   angle, color, mode, fx, fy);
        };
        _parser = std::make_unique<GraphicsMsg_ToRgaParser>(_runtime, cbs);
    }

    void onMessage(const uint8_t* data, size_t size) {
        if (_parser) {
            _parser->Parse(data, size);
        }
    }

private:
    void handleRgaRequest(uint32_t job_id, int8_t type, 
                          int src_fd, int src_w, int src_h, int src_fmt, int src_x, int src_y,
                          int dst_fd, int dst_w, int dst_h, int dst_fmt, int dst_x, int dst_y,
                          int pat_fd, int pat_w, int pat_h, int pat_fmt, int pat_x, int pat_y,
                          int angle, uint32_t color, int mode, double fx, double fy)
    {
        using namespace SuOS::graphics;
        // 使用一个名为 RgaUdsService 的公共 User 以复用或者自动释放 imported buffers
        auto usr = VramProvider::getInstance().getOrCreateUsr("RgaUdsService_Imports");
        
        std::shared_ptr<VBuffer> src_buf, dst_buf, pat_buf;
        RgaConfig src_cfg{src_w, src_h, src_fmt, src_x, src_y};
        RgaConfig dst_cfg{dst_w, dst_h, dst_fmt, dst_x, dst_y};
        RgaConfig pat_cfg{pat_w, pat_h, pat_fmt, pat_x, pat_y};
        
        // 当传入有效的 fd 时，我们需要接管
        if (src_fd >= 0) src_buf = usr->importBuffer(src_fd, src_w, src_h, src_fmt, src_w * 4, src_w * src_h * 4);
        if (dst_fd >= 0) dst_buf = usr->importBuffer(dst_fd, dst_w, dst_h, dst_fmt, dst_w * 4, dst_w * dst_h * 4);
        if (pat_fd >= 0) pat_buf = usr->importBuffer(pat_fd, pat_w, pat_h, pat_fmt, pat_w * 4, pat_w * pat_h * 4);
        
        RgaStep step;
        step.type = static_cast<RgaStepType>(type);
        step.src = src_buf;
        step.dst = dst_buf;
        step.pat = pat_buf;
        step.src_cfg = src_cfg;
        step.dst_cfg = dst_cfg;
        step.pat_cfg = pat_cfg;
        step.angle = angle;
        step.color = color;
        step.mode = mode;
        step.fx = fx;
        step.fy = fy;
        
        RgaChain chain;
        chain.steps.push_back(step);
        
        // 在引擎的线程里或者提交，等异步回调回来然后再将 err_code 给 UDS 客户端返回。
        _runtime->postTask([this, chain, job_id, src_buf, dst_buf, pat_buf]() { 
            RgaEngine::getInstance().submit(chain, [this, job_id, src_buf, dst_buf, pat_buf](uint32_t err_code) {
                this->postResponse(job_id, err_code);
                // 这里我们暂存 buf 以避免在 submit 出去前共享指针就被释放，
                // 同时，import 进来的其实可以立刻丢弃，因为真正的 buf 是借用，
                // 具体的按需释放机制可通过清掉 Provider 里该 User 达成，或者让智能指针自然销毁
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
