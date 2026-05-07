#pragma once
#include "RgaTypes.hpp"
#include <thread>

namespace SuOS::graphics {

/**
 * @brief RGA 基础操作库 (全静态类)
 * 职责：负责将单个 SuOS Buffer 派发到 RGA 硬件执行，独立线程监听等待中断。
 */
class RgaBasic {
public:
    // 异步执行拷贝
    static im_job_handle_t asyncCopy(std::shared_ptr<VramProvider::VBuffer> src,
                                     std::shared_ptr<VramProvider::VBuffer> dst,
                                     const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                                     RgaBasicCallback cb);

    // 异步执行旋转
    static im_job_handle_t asyncRotate(std::shared_ptr<VramProvider::VBuffer> src,
                                       std::shared_ptr<VramProvider::VBuffer> dst,
                                       const RgaConfig& src_cfg, int angle,
                                       RgaBasicCallback cb);

    // 异步执行颜色块填充
    static im_job_handle_t asyncFill(std::shared_ptr<VramProvider::VBuffer> dst,
                                     const RgaConfig& dst_cfg, uint32_t color,
                                     RgaBasicCallback cb);

    // 强行终止某一项内核底层任务
    static void cancel(im_job_handle_t job_handle);

private:
    static rga_buffer_t wrapBuffer(std::shared_ptr<VramProvider::VBuffer> buf, const RgaConfig& cfg);
    static im_usage getRotationUsage(int angle);
    
    // 核心异步监视分发逻辑
    static void monitorHardware(im_job_handle_t handle, 
                                std::shared_ptr<VramProvider::VBuffer> src_keepalive,
                                std::shared_ptr<VramProvider::VBuffer> dst_keepalive,
                                RgaBasicCallback cb);
};

} // namespace suos::graphics