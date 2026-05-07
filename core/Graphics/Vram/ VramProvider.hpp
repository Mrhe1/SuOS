#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <stdexcept>

// === Linux 底层依赖 ===
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <drm/drm.h>       // 需确保编译环境中包含内核 drm 头文件
#include <drm/drm_mode.h>

namespace SuOS {
namespace graphics {

class VramUsr;

/**
 * @brief 显存对象 (VBuffer)
 * 现在包含了真实的 DRM Handle 管理与精确的生命周期清理
 */
class VBuffer {
public:
    // 构造时接管真实的内存信息
    VBuffer(int drm_fd, uint32_t handle, int dma_fd, uint32_t width, uint32_t height, uint32_t pitch, size_t size, void* vaddr, const std::string& owner_name)
        : drm_fd_(drm_fd), handle_(handle), fd_(dma_fd), 
          width_(width), height_(height), pitch_(pitch), size_(size), 
          vaddr_(vaddr), owner_name_(owner_name) {}

    ~VBuffer() {
        std::cout << "[VBuffer] Releasing Hardware Memory for: " << owner_name_ << std::endl;
        
        // 1. 解除 CPU 内存映射
        if (vaddr_ != MAP_FAILED && vaddr_ != nullptr) {
            munmap(vaddr_, size_);
        }
        
        // 2. 关闭 DMA-BUF 文件描述符 
        if (fd_ >= 0) {
            close(fd_);
        }

        // 3. 通知 DRM 内核销毁底层的 Dumb Buffer（释放真正的物理内存）
        struct drm_mode_destroy_dumb dreq = {};
        dreq.handle = handle_;
        if (ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) < 0) {
            std::cerr << "[VBuffer Error] Failed to destroy DRM dumb buffer!\n";
        }
        
        std::cout << "[VBuffer] Cleanup complete. FD " << fd_ << " closed." << std::endl;
    }

    // --- 留给硬件异步的标记锁机 ---
    void reportAccessStart(const std::string& tag) {
        access_mtx_.lock();
        current_access_tag_ = tag;
        is_busy_.store(true);
    }
    void reportAccessEnd() {
        is_busy_.store(false);
        current_access_tag_ = "idle";
        access_mtx_.unlock();
    }

    // 访问器
    int getFd() const { return fd_; }            // 传给 RGA 或 VOP 的描述符
    void* getVaddr() const { return vaddr_; }    // CPU (如 LVGL) 渲染用的虚地址
    size_t getSize() const { return size_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    uint32_t getPitch() const { return pitch_; } // 极其重要：实际行的字节宽度（可能因字节对齐大于 width * bpp）

private:
    int drm_fd_;          // 系统 DRM 设备描述符
    uint32_t handle_;     // DRM 内部的句柄 (不可跨进程)
    int fd_;              // DMA-BUF 描述符 (可跨进程, 可喂给 RGA)
    
    uint32_t width_;
    uint32_t height_;
    uint32_t pitch_;      // 步长/跨进
    size_t size_;         // 实际占用物理内存大小
    void* vaddr_;         // CPU 可访问指针
    std::string owner_name_;
    
    std::mutex access_mtx_;
    std::atomic<bool> is_busy_{false};
    std::string current_access_tag_{"idle"};
};


/**
 * @brief 显存用户实体 (VramUsr)
 */
class VramUsr {
public:
    explicit VramUsr(const std::string& name, int drm_fd) : usr_name_(name), drm_fd_(drm_fd) {}

    ~VramUsr() {
        releaseAllBuffers();
    }

    /**
     * @brief 真实硬件分配逻辑
     * 相比分配 size，图形系统分配二维尺寸更为严谨
     * @param bpp: Bits Per Pixel (例如 ARGB8888 填 32)
     */
    std::shared_ptr<VBuffer> createBuffer(uint32_t width, uint32_t height, uint32_t bpp) {
        std::lock_guard<std::mutex> lock(usr_mtx_);

        // 1. 发起 DRM Dumb Buffer 创建请求
        struct drm_mode_create_dumb creq = {};
        creq.width = width;
        creq.height = height;
        creq.bpp = bpp;

        if (ioctl(drm_fd_, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
            throw std::runtime_error("DRM_IOCTL_MODE_CREATE_DUMB failed. Out of memory?");
        }

        // 2. 将内部 Handle 导出为通用的 DMA-BUF FD
        int dma_fd = -1;
        struct drm_prime_handle prime_req = {};
        prime_req.handle = creq.handle;
        prime_req.flags = DRM_CLOEXEC | DRM_RDWR;

        if (ioctl(drm_fd_, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_req) < 0) {
            // 清理已创建的 dumb buffer
            struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
            ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
            throw std::runtime_error("Failed to export DRM handle to DMA-BUF fd");
        }
        dma_fd = prime_req.fd;

        // 3. 准备 CPU 虚拟地址映射 (mmap)
        struct drm_mode_map_dumb mreq = {};
        mreq.handle = creq.handle;
        if (ioctl(drm_fd_, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
            // 省略清理代码，同上
            throw std::runtime_error("DRM_IOCTL_MODE_MAP_DUMB failed");
        }

        // mreq.offset 是 DRM 生成的假偏移量，用于触发内核的内存映射逻辑
        void* vaddr = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd_, mreq.offset);
        if (vaddr == MAP_FAILED) {
            throw std::runtime_error("mmap failed for Dumb Buffer");
        }

        // 4. 清理脏数据（可选，防止出现花屏）
        memset(vaddr, 0, creq.size);

        // 5. 将获取到的所有真实数据塞入 VBuffer 对象
        auto buffer = std::make_shared<VBuffer>(
            drm_fd_, creq.handle, dma_fd, 
            width, height, creq.pitch, creq.size, vaddr, usr_name_
        );
        
        active_buffers_.push_back(buffer);
        std::cout << "[VramUsr] Allocated buffer for " << usr_name_ << " (FD: " << dma_fd << ", Size: " << creq.size << " bytes)" << std::endl;
        return buffer;
    }

    void releaseAllBuffers() {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        if (!active_buffers_.empty()) {
            active_buffers_.clear(); // 触发 VBuffer 析构，执行 munmap 和 close
        }
    }

private:
    std::string usr_name_;
    int drm_fd_;      // 继承自 Provider
    std::mutex usr_mtx_;
    std::vector<std::shared_ptr<VBuffer>> active_buffers_;
};


/**
 * @brief 顶层工厂：持有底层 DRM 驱动描述符
 */
class VramProvider {
public:
    static VramProvider& getInstance() {
        static VramProvider instance;
        return instance;
    }

    std::shared_ptr<VramUsr> getOrCreateUsr(const std::string& usr_name) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        if (users_.find(usr_name) == users_.end()) {
            users_[usr_name] = std::make_shared<VramUsr>(usr_name, drm_fd_);
        }
        return users_[usr_name];
    }

    void removeUsr(const std::string& usr_name) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        auto it = users_.find(usr_name);
        if (it != users_.end()) {
            it->second->releaseAllBuffers();
            users_.erase(it);
        }
    }

private:
    VramProvider() {
        // 全局初始化仅执行一次：打开内核图形卡节点
        // 在 RK 系列芯片中，通常只有一个 card0
        const char* card_path = "/dev/dri/card0";
        drm_fd_ = open(card_path, O_RDWR | O_CLOEXEC);
        
        if (drm_fd_ < 0) {
            std::cerr << "FATAL ERROR: Cannot open DRM device " << card_path << "!\n";
            std::cerr << "Are you running as root? Is the graphics driver loaded?\n";
            // 在生产环境可以丢出异常或降级模式
        } else {
            std::cout << "[VramProvider] DRM subsystem initialized. (FD: " << drm_fd_ << ")" << std::endl;
        }
    }

    ~VramProvider() {
        users_.clear(); // 先清理所有 User 里的内存
        if (drm_fd_ >= 0) {
            close(drm_fd_);
        }
    }

    VramProvider(const VramProvider&) = delete;

    int drm_fd_ = -1;
    std::mutex provider_mtx_;
    std::unordered_map<std::string, std::shared_ptr<VramUsr>> users_;
};

} // namespace graphics
} // namespace suos