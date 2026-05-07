#include "RgaBasic.hpp"

namespace SuOS::graphics {

rga_buffer_t RgaBasic::wrapBuffer(std::shared_ptr<VramProvider::VBuffer> buf, const RgaConfig& cfg) {
    if (!buf) return {};
    rga_buffer_t rga_buf = importbuffer_fd(buf->getFd(), cfg.width, cfg.height, cfg.format);
    rga_buf.rect.x = 0;
    rga_buf.rect.y = 0;
    rga_buf.rect.width = cfg.width;
    rga_buf.rect.height = cfg.height;
    return rga_buf;
}

im_usage RgaBasic::getRotationUsage(int angle) {
    switch(angle) {
        case 90:  return IM_HAL_TRANSFORM_ROT_90;
        case 180: return IM_HAL_TRANSFORM_ROT_180;
        case 270: return IM_HAL_TRANSFORM_ROT_270;
        default:  return (im_usage)0;
    }
}

void RgaBasic::cancel(im_job_handle_t job_handle) {
    if (job_handle > 0) {
        imcancel(job_handle);
    }
}

// ============== 核心的异步内核调度与生命周期保活 ==============
void RgaBasic::monitorHardware(im_job_handle_t handle, 
                               std::shared_ptr<VramProvider::VBuffer> src_keepalive, // 控制 VBuffer 不被虚空释放
                               std::shared_ptr<VramProvider::VBuffer> dst_keepalive,
                               RgaBasicCallback cb) {
    // 拉起一个游离态的轻量线程去死等内核硬件完成信号 (规避阻塞调用者)
    std::thread([handle, src_keepalive, dst_keepalive, cb]() {
        // imend 在 Rockchip 内核 ioctl 中等待硬件产生中断，是真正的阻塞源头
        IM_STATUS status = imend(handle);
        
        if (cb) {
            if (status == IM_STATUS_SUCCESS) {
                cb(true, "");
            } else {
                cb(false, "Kernel processing failed with RGA error code: " + std::to_string(status));
            }
        }
        // 当此 lambda 域退出时，src_keepalive 和 dst_keepalive 将会自动归还释放
    }).detach();
}

im_job_handle_t RgaBasic::asyncCopy(std::shared_ptr<VramProvider::VBuffer> src,
                                    std::shared_ptr<VramProvider::VBuffer> dst,
                                    const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                                    RgaBasicCallback cb) {
    im_job_handle_t handle = imbegin(); // 创建一个新的内核事务
    rga_buffer_t r_src = wrapBuffer(src, src_cfg);
    rga_buffer_t r_dst = wrapBuffer(dst, dst_cfg);
    
    imcopy_task(handle, r_src, r_dst);
    monitorHardware(handle, src, dst, cb);
    return handle;
}

im_job_handle_t RgaBasic::asyncRotate(std::shared_ptr<VramProvider::VBuffer> src,
                                      std::shared_ptr<VramProvider::VBuffer> dst,
                                      const RgaConfig& src_cfg, int angle,
                                      RgaBasicCallback cb) {
    im_job_handle_t handle = imbegin();
    rga_buffer_t r_src = wrapBuffer(src, src_cfg);
    rga_buffer_t r_dst = wrapBuffer(dst, src_cfg);
    
    imrotate_task(handle, r_src, r_dst, getRotationUsage(angle));
    monitorHardware(handle, src, dst, cb);
    return handle;
}

im_job_handle_t RgaBasic::asyncFill(std::shared_ptr<VramProvider::VBuffer> dst,
                                    const RgaConfig& dst_cfg, uint32_t color,
                                    RgaBasicCallback cb) {
    im_job_handle_t handle = imbegin();
    rga_buffer_t r_dst = wrapBuffer(dst, dst_cfg);
    im_rect rect = {0, 0, dst_cfg.width, dst_cfg.height};
    
    imfill_task(handle, r_dst, rect, color);
    monitorHardware(handle, nullptr, dst, cb);
    return handle;
}

} // namespace suos::graphics