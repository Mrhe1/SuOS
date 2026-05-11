#pragma once
#include <memory>
#include <functional>
#include "suRuntime.hpp"
#include "Uds_GraphicsMsg_ToRgaBuilder.hpp"
#include "Uds_GraphicsMsg_FromRgaParser.hpp"

namespace SuOS::Uds::Msg::Graphics {

class RgaServiceClient {
public:
    using SendFunc = std::function<void(const uint8_t* data, size_t size)>;
    using ResponseCallback = std::function<void(uint32_t job_id, uint32_t err_code)>;

    RgaServiceClient(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, SendFunc send_cb, ResponseCallback resp_cb)
        : _runtime(runtime), _send_cb(send_cb), _builder(runtime) 
    {
        GraphicsMsg_FromRgaParser::Callbacks cbs;
        cbs.onRgaResponse = [this, resp_cb](uint32_t job_id, uint32_t err_code) {
            if (resp_cb) {
                resp_cb(job_id, err_code);
            }
        };
        _parser = std::make_unique<GraphicsMsg_FromRgaParser>(_runtime, cbs);
    }

    void onMessage(const uint8_t* data, size_t size) {
        if (_parser) {
            _parser->Parse(data, size);
        }
    }

    // Client 发送 Rga 请求
    void sendRgaRequest(uint32_t job_id, int8_t type, 
                        int src_fd, int src_w, int src_h, int src_fmt, int src_x, int src_y,
                        int dst_fd, int dst_w, int dst_h, int dst_fmt, int dst_x, int dst_y,
                        int pat_fd, int pat_w, int pat_h, int pat_fmt, int pat_x, int pat_y,
                        int angle, uint32_t color, int mode, double fx, double fy) 
    {
        if (_runtime->isInEventLoop()) {
            auto guard = _builder.BuildRgaRequest(job_id, type, 
                                                src_fd, src_w, src_h, src_fmt, src_x, src_y,
                                                dst_fd, dst_w, dst_h, dst_fmt, dst_x, dst_y,
                                                pat_fd, pat_w, pat_h, pat_fmt, pat_x, pat_y,
                                                angle, color, mode, fx, fy);
            if (_send_cb) {
                _send_cb(guard.data(), guard.size());
            }
        } else {
            _runtime->postTask([=]() {
                sendRgaRequest(job_id, type, 
                              src_fd, src_w, src_h, src_fmt, src_x, src_y,
                              dst_fd, dst_w, dst_h, dst_fmt, dst_x, dst_y,
                              pat_fd, pat_w, pat_h, pat_fmt, pat_x, pat_y,
                              angle, color, mode, fx, fy);
            });
        }
    }

private:
    std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    SendFunc _send_cb;
    GraphicsMsg_ToRgaBuilder _builder;
    std::unique_ptr<GraphicsMsg_FromRgaParser> _parser;
};

} // namespace SuOS::Uds::Msg::Graphics
