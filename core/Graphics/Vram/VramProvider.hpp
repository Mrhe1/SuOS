#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <random>

// === Linux 底层依赖 ===
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <drm/drm.h>       // 需确保编译环境中包含内核 drm 头文件
#include <drm/drm_mode.h>
#include <optional>
#include "rga/rga.h"

namespace SuOS {
namespace graphics {

// --- 前置声明 ---
class VBuffer;
class VramUsr;
class VramProvider;

// --- 基础结构体与枚举 ---

enum class VramError : int {
    OK = 0,
    INVALID_STATE = 1,
    STEP_MISMATCH = 2,
    HARDWARE_BUSY = 3,
    DRM_CREATE_DUMB_FAILED = 4,
    DRM_PRIME_EXPORT_FAILED = 5,
    DRM_MAP_FAILED = 6,
    MMAP_FAILED = 7,
    UNKNOWN_FORMAT = 8,
    NOT_FOUND = 9
};

enum class BufferState {
    IDLE,
    READY,
    RUNNING
};

struct DirtyRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

static uint32_t getBppFromRgaFormat(int format) {
    switch (format) {
        case RK_FORMAT_RGBA_8888: case RK_FORMAT_RGBX_8888:
        case RK_FORMAT_BGRA_8888: case RK_FORMAT_ARGB_8888:
        case RK_FORMAT_XRGB_8888: return 32;
        case RK_FORMAT_RGB_888: case RK_FORMAT_BGR_888: return 24;
        case RK_FORMAT_RGB_565: case RK_FORMAT_BGR_565:
        case RK_FORMAT_RGBA_5551: case RK_FORMAT_RGBA_4444: return 16;
        case RK_FORMAT_YCbCr_420_SP: case RK_FORMAT_YCbCr_420_P:
        case RK_FORMAT_YCrCb_420_SP: case RK_FORMAT_YCrCb_420_P: return 12; 
        case RK_FORMAT_YCbCr_422_SP: case RK_FORMAT_YCbCr_422_P:
        case RK_FORMAT_YCrCb_422_SP: case RK_FORMAT_YCrCb_422_P: return 16;
        default: return 0;
    }
}

struct WorkflowContext {
    uint32_t frame_num = 0;
    int color_fmt = _Rga_SURF_FORMAT::RK_FORMAT_UNKNOWN;
    std::vector<DirtyRect> dirty_rects;
    BufferState state = BufferState::IDLE;
    std::vector<std::string> planned_steps;
    size_t current_step_idx = 0;
    bool step_busy = false;

    void reset() {
        frame_num = 0;
        color_fmt = _Rga_SURF_FORMAT::RK_FORMAT_UNKNOWN;
        dirty_rects.clear();
        state = BufferState::IDLE;
        planned_steps.clear();
        current_step_idx = 0;
        step_busy = false;
    }
};

/**
 * @brief 显存对象 (VBuffer)
 * 继承 enable_shared_from_this 允许其将自身安全地注册到全局 Provider
 */
class VBuffer : public std::enable_shared_from_this<VBuffer> {
public:
    VBuffer(int drm_fd, uint32_t handle, int dma_fd, uint32_t width, uint32_t height, uint32_t pitch, size_t size, void* vaddr, const std::string& owner_name)
        : drm_fd_(drm_fd), handle_(handle), fd_(dma_fd), 
          width_(width), height_(height), pitch_(pitch), size_(size), 
          vaddr_(vaddr), owner_name_(owner_name), share_id_(0) {
        ctx_.reset();
    }

    // 析构与分享接口的实现需要依赖 VramProvider，放置在文件末尾内联实现
    ~VBuffer();

    /**
     * @brief 开启分享，实时生成并获取全局唯一 Share ID
     */
    uint64_t share();

    /**
     * @brief 取消分享，从全局注册表中弃用该 Share ID
     */
    void unshare();

    // ==========================================
    // --- 核心：工作流状态机接口 ---
    // ==========================================

    VramError setupWorkflow(uint32_t frame_num, const std::vector<std::string>& planned_steps, int color_fmt) {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        if (ctx_.state != BufferState::IDLE) return VramError::INVALID_STATE;
        ctx_.frame_num = frame_num;
        ctx_.planned_steps = planned_steps;
        ctx_.color_fmt = color_fmt;
        ctx_.current_step_idx = 0;
        ctx_.dirty_rects.clear();
        ctx_.step_busy = false;
        ctx_.state = BufferState::READY;
        return VramError::OK;
    }

    void addDirtyRect(const DirtyRect& rect) {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        if (ctx_.state == BufferState::READY || ctx_.state == BufferState::RUNNING) {
            ctx_.dirty_rects.push_back(rect);
        }
    }

    VramError startStep(const std::string& tag) {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        if (ctx_.state == BufferState::IDLE) return VramError::INVALID_STATE;
        if (ctx_.step_busy) return VramError::HARDWARE_BUSY;
        if (ctx_.current_step_idx >= ctx_.planned_steps.size() || ctx_.planned_steps[ctx_.current_step_idx] != tag) {
            return VramError::STEP_MISMATCH;
        }
        ctx_.state = BufferState::RUNNING;
        ctx_.step_busy = true; 
        return VramError::OK;
    }

    VramError finishStep(const std::string& tag) {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        if (ctx_.state != BufferState::RUNNING || !ctx_.step_busy) return VramError::INVALID_STATE;
        if (ctx_.planned_steps[ctx_.current_step_idx] != tag) return VramError::STEP_MISMATCH;

        ctx_.step_busy = false; 
        ctx_.current_step_idx++;

        if (ctx_.current_step_idx >= ctx_.planned_steps.size()) {
            ctx_.state = BufferState::IDLE; 
        }
        return VramError::OK;
    }

    VramError cancelWorkflow() {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        if (ctx_.step_busy) return VramError::HARDWARE_BUSY; 
        ctx_.reset();
        return VramError::OK;
    }

    // --- 状态与属性访问器 ---
    uint64_t getShareId() const { 
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        return share_id_; 
    }

    BufferState getState() { 
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        return ctx_.state; 
    }
    uint32_t getFrameNum() { 
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        return ctx_.frame_num; 
    }
    std::vector<DirtyRect> getDirtyRects() {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        return ctx_.dirty_rects;
    }
    int getColorFormat() {
        std::lock_guard<std::mutex> lock(workflow_mtx_);
        return ctx_.color_fmt;
    }

    int getFd() const { return fd_; }            
    void* getVaddr() const { return vaddr_; }    
    size_t getSize() const { return size_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    uint32_t getPitch() const { return pitch_; } 

private:
    int drm_fd_;          
    uint32_t handle_;     
    int fd_;              
    uint32_t width_;
    uint32_t height_;
    uint32_t pitch_;      
    size_t size_;         
    void* vaddr_;         
    std::string owner_name_;

    // 内部工作流和共享状态锁
    mutable std::mutex workflow_mtx_;
    WorkflowContext ctx_;
    uint64_t share_id_;   // 0 表示未分享
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

    VramError createBuffer(uint32_t width, uint32_t height, int color_fmt, std::shared_ptr<VBuffer>& out_buffer) {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        
        uint32_t bpp = getBppFromRgaFormat(color_fmt);
        if (bpp == 0) return VramError::UNKNOWN_FORMAT;

        struct drm_mode_create_dumb creq = {};
        creq.width = width;
        creq.height = height;
        creq.bpp = bpp;

        if (ioctl(drm_fd_, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) return VramError::DRM_CREATE_DUMB_FAILED;

        int dma_fd = -1;
        struct drm_prime_handle prime_req = {};
        prime_req.handle = creq.handle;
        prime_req.flags = DRM_CLOEXEC | DRM_RDWR;

        if (ioctl(drm_fd_, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_req) < 0) {
            struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
            ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
            return VramError::DRM_PRIME_EXPORT_FAILED;
        }
        dma_fd = prime_req.fd;

        struct drm_mode_map_dumb mreq = {};
        mreq.handle = creq.handle;
        if (ioctl(drm_fd_, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
            struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
            ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
            close(dma_fd);
            return VramError::DRM_MAP_FAILED;
        }

        void* vaddr = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd_, mreq.offset);
        if (vaddr == MAP_FAILED) {
            struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
            ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
            close(dma_fd);
            return VramError::MMAP_FAILED;
        }

        memset(vaddr, 0, creq.size);

        out_buffer = std::make_shared<VBuffer>(
            drm_fd_, creq.handle, dma_fd, 
            width, height, creq.pitch, creq.size, vaddr, usr_name_
        );

        active_buffers_.push_back(out_buffer);
        return VramError::OK;
    }

    void releaseAllBuffers() {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        active_buffers_.clear(); 
    }

    std::shared_ptr<VBuffer> getAvailableBuffer(uint32_t width, uint32_t height) {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        for (const auto& buf : active_buffers_) {
            if (buf->getState() == BufferState::IDLE && buf->getWidth() == width && buf->getHeight() == height) {
                return buf; 
            }
        }
        return nullptr;
    }

private:
    std::string usr_name_;
    int drm_fd_;      
    std::mutex usr_mtx_;
    std::vector<std::shared_ptr<VBuffer>> active_buffers_;
};


/**
 * @brief 顶层工厂：持有底层 DRM 驱动描述符和全局显存分享注册表
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

    // ==========================================
    // --- 全局共享中心机制 ---
    // ==========================================

    /**
     * @brief 供 VBuffer 内部调用：注册并生成全局唯一的随机 Token
     */
    uint64_t registerSharedBuffer(std::weak_ptr<VBuffer> weak_buf) {
        std::lock_guard<std::mutex> lock(share_mtx_);
        uint64_t new_id = 0;
        // 确保生成不为0且全局唯一的ID
        do {
            new_id = rng_();
        } while (new_id == 0 || shared_registry_.count(new_id) > 0);

        shared_registry_[new_id] = weak_buf;
        return new_id;
    }

    /**
     * @brief 供 VBuffer 内部调用：撤销已分享的 Token
     */
    void unregisterSharedBuffer(uint64_t share_id) {
        std::lock_guard<std::mutex> lock(share_mtx_);
        shared_registry_.erase(share_id);
    }

    /**
     * @brief 全局查找：通过 Share ID 直接获取 VBuffer，O(1) 复杂度
     */
    std::shared_ptr<VBuffer> getSharedBuffer(uint64_t share_id) {
        std::lock_guard<std::mutex> lock(share_mtx_);
        auto it = shared_registry_.find(share_id);
        if (it != shared_registry_.end()) {
            // 将 weak_ptr 提升为 shared_ptr
            auto sp = it->second.lock();
            if (sp) {
                return sp; // 命中且存活
            }
            // 如果弱引用已过期（说明内存已回收但某种原因没正常 unshare），顺手清理残余
            shared_registry_.erase(it);
        }
        return nullptr;
    }

private:
    VramProvider() {
        const char* card_path = "/dev/dri/card0";
        drm_fd_ = open(card_path, O_RDWR | O_CLOEXEC);

        if (drm_fd_ < 0) {
            std::cerr << "FATAL ERROR: Cannot open DRM device " << card_path << "!\n";
        } else {
            std::cout << "[VramProvider] DRM subsystem initialized. (FD: " << drm_fd_ << ")" << std::endl;
        }

        // 初始化随机数引擎
        std::random_device rd;
        rng_.seed(rd());
    }

    ~VramProvider() {
        users_.clear(); 
        if (drm_fd_ >= 0) {
            close(drm_fd_);
        }
    }

    VramProvider(const VramProvider&) = delete;

    int drm_fd_ = -1;
    std::mutex provider_mtx_;
    std::unordered_map<std::string, std::shared_ptr<VramUsr>> users_;

    // 共享注册表体系
    std::mutex share_mtx_;
    std::mt19937_64 rng_; // 64位随机数引擎
    std::unordered_map<uint64_t, std::weak_ptr<VBuffer>> shared_registry_;
};

// ==========================================
// --- 延迟的 VBuffer 内联函数实现 ---
// ==========================================

inline VBuffer::~VBuffer() {
    // 销毁时主动取消分享，保障全局注册表干净
    unshare();

    // 硬件解绑
    if (vaddr_ != MAP_FAILED && vaddr_ != nullptr) munmap(vaddr_, size_);
    if (fd_ >= 0) close(fd_);

    struct drm_mode_destroy_dumb dreq = {};
    dreq.handle = handle_;
    if (ioctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) < 0) {
        std::cerr << "[VBuffer Error] Failed to destroy DRM dumb buffer!\n";
    }
}

inline uint64_t VBuffer::share() {
    std::lock_guard<std::mutex> lock(workflow_mtx_);
    if (share_id_ != 0) {
        return share_id_; // 已经分享了，直接返回现有 ID
    }
    // 调用 Provider 注册本实例
    share_id_ = VramProvider::getInstance().registerSharedBuffer(shared_from_this());
    return share_id_;
}

inline void VBuffer::unshare() {
    std::lock_guard<std::mutex> lock(workflow_mtx_);
    if (share_id_ != 0) {
        // 调用 Provider 移除注册
        VramProvider::getInstance().unregisterSharedBuffer(share_id_);
        share_id_ = 0;
    }
}

} // namespace graphics
} // namespace SuOS