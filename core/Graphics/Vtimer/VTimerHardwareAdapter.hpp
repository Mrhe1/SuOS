#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <drm.h>

// 引用之前的 VTimer 所在命名空间
#include "vtimer.hpp"

namespace SuOS::Runtime {

class DRMVsyncListener {
public:
    DRMVsyncListener() : drm_fd_(-1), running_(false) {}

    ~DRMVsyncListener() {
        stop();
    }

    /**
     * @brief 初始化并启动 VSync 监听线程
     * @param device DRM 设备路径，RK3506 通常为 /dev/dri/card0
     */
    bool start(const std::string& device = "/dev/dri/card0") {
        drm_fd_ = open(device.c_str(), O_RDWR | O_CLOEXEC);
        if (drm_fd_ < 0) {
            std::cerr << "[DRM] Failed to open " << device << std::endl;
            return false;
        }

        running_ = true;
        // 创建独立线程，避免阻塞主循环或 suRuntime 的 Event Loop
        listener_thread_ = std::thread(&DRMVsyncListener::loop, this);
        
        std::cout << "[DRM] VSync Listener started on thread: " 
                  << listener_thread_.get_id() << std::endl;
        return true;
    }

    void stop() {
        running_ = false;
        if (listener_thread_.joinable()) {
            listener_thread_.join();
        }
        if (drm_fd_ >= 0) {
            close(drm_fd_);
            drm_fd_ = -1;
        }
    }

private:
    void loop() {
        // 配置 VBlank 请求参数
        // DRM_VBLANK_RELATIVE: 基于当前帧的相对等待
        // sequence = 1: 表示等待 1 个 VSync 周期
        drmVBlank vbl;
        vbl.request.type = DRM_VBLANK_RELATIVE;
        vbl.request.sequence = 1;

        while (running_) {
            // 核心阻塞调用：等待硬件信号
            // 这是一个 ioctl 封装，会进入内核态挂起线程
            int ret = drmWaitVBlank(drm_fd_, &vbl);

            if (ret == 0) {
                // --- 硬件脉搏到达！---
                // 触发 VTimerProvider 的分发机制
                VTimerProvider::getInstance().triggerHardwareVSync();
            } else {
                // 如果返回错误（如设备被占用或驱动不支持），稍作等待避免死循环占满 CPU
                std::cerr << "[DRM] drmWaitVBlank error: " << ret << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    int drm_fd_;
    std::atomic<bool> running_;
    std::thread listener_thread_;
};

} // namespace SuOS::Runtime