#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <random>
#include <atomic>
#include <algorithm>

namespace suos {
namespace graphics {

class VramUsr; // 前置声明

/**
 * @brief 显存对象 (VBuffer)
 * 最底层的资源载体，封装底层 DMA-BUF 硬件描述符与异步访问状态
 */
class VBuffer {
public:
    VBuffer(int fd, size_t size, void* vaddr, const std::string& owner_name)
        : fd_(fd), size_(size), vaddr_(vaddr), owner_name_(owner_name) {}

    ~VBuffer() {
        // 实际环境：调用 DRM IOCTL 或 DMA-BUF close(fd)
        std::cout << "[VBuffer] Cleanup: FD " << fd_ << " (Owner: " << owner_name_ << ")" << std::endl;
    }

    // --- 异步友好型锁定机制 ---
    void reportAccessStart(const std::string& tag) {
        access_mtx_.lock(); // 手动锁定（非 RAII，适应硬件中断回调）
        current_access_tag_ = tag;
        is_busy_.store(true);
        std::cout << "[VBuffer] BUSY: [" << tag << "] locked FD " << fd_ << std::endl;
    }

    void reportAccessEnd() {
        std::string tag = current_access_tag_;
        is_busy_.store(false);
        current_access_tag_ = "idle";
        access_mtx_.unlock(); // 手动解锁
        std::cout << "[VBuffer] IDLE: [" << tag << "] unlocked FD " << fd_ << std::endl;
    }

    int getFd() const { return fd_; }
    size_t getSize() const { return size_; }
    void* getVaddr() const { return vaddr_; }
    bool isBusy() const { return is_busy_.load(); }

private:
    int fd_;
    size_t size_;
    void* vaddr_;
    std::string owner_name_;
    
    std::mutex access_mtx_;
    std::atomic<bool> is_busy_{false};
    std::string current_access_tag_{"idle"};
};

/**
 * @brief 显存用户实体 (VramUsr)
 * 扮演“资源沙箱/配额组”的角色。代表一个 AppService 进程或系统组件。
 */
class VramUsr {
public:
    explicit VramUsr(const std::string& name) : usr_name_(name) {
        std::cout << "[VramUsr] Created user: " << usr_name_ << std::endl;
    }

    ~VramUsr() {
        std::cout << "[VramUsr] Destroying user: " << usr_name_ << " (Auto-release triggered)" << std::endl;
        releaseAllBuffers();
    }

    /**
     * @brief 为当前用户申请一块显存
     */
    std::shared_ptr<VBuffer> createBuffer(size_t size) {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        
        // 模拟向内核申请物理连续显存
        static int fd_gen = 100; 
        void* dummy_vaddr = reinterpret_cast<void*>(0x50000000 + fd_gen);
        
        auto buffer = std::make_shared<VBuffer>(fd_gen++, size, dummy_vaddr, usr_name_);
        active_buffers_.push_back(buffer);
        return buffer;
    }

    /**
     * @brief 内部方法：批量销毁该用户下的所有显存
     * 可以被 VramUsr 内部调用，也可被外部 Provider 在移除用户前显式调用
     */
    void releaseAllBuffers() {
        std::lock_guard<std::mutex> lock(usr_mtx_);
        if (!active_buffers_.empty()) {
            std::cout << "[VramUsr] Releasing " << active_buffers_.size() << " buffers for " << usr_name_ << std::endl;
            active_buffers_.clear(); // std::shared_ptr 计数减 1，触发 VBuffer 自动析构
        }
    }

    const std::string& getName() const { return usr_name_; }

private:
    std::string usr_name_;
    std::mutex usr_mtx_;
    std::vector<std::shared_ptr<VBuffer>> active_buffers_; // 该用户当前持有的所有显存
};

/**
 * @brief SuOS 显存管理中心 (VramProvider)
 * 顶层单例。负责管理系统内所有 VramUsr 的生命周期，以及提供跨线程/跨进程的全局 Key 寻址
 */
class VramProvider {
public:
    static VramProvider& getInstance() {
        static VramProvider instance;
        return instance;
    }

    // ================= 1. 用户沙箱管理 (VramUsr) =================

    /**
     * @brief 注册并获取一个用户沙箱 (例如分配给新诞生的 Wasm 进程)
     */
    std::shared_ptr<VramUsr> getOrCreateUsr(const std::string& usr_name) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        if (users_.find(usr_name) == users_.end()) {
            users_[usr_name] = std::make_shared<VramUsr>(usr_name);
        }
        return users_[usr_name];
    }

    /**
     * @brief 彻底拔除一个用户及其所有资产 (用于应对 App 崩溃或正常退出)
     */
    void removeUsr(const std::string& usr_name) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        auto it = users_.find(usr_name);
        if (it != users_.end()) {
            // 显式调用 usr 的销毁方法（防御性编程，虽然 erase() 也会触发析构）
            it->second->releaseAllBuffers();
            // 从映射表中移除该用户，触发 VramUsr 析构
            users_.erase(it);
            std::cout << "[Provider] Successfully removed user constraints for: " << usr_name << std::endl;
        }
    }

    // ================= 2. 跨线程全局寻址 (Router / IPC 支持) =================

    /**
     * @brief 将某个 User 下的 buffer 注册到公共空间，并分配一个 UDS 传递用的随机 Key
     */
    uint32_t registerSharedBuffer(std::shared_ptr<VBuffer> buffer) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        uint32_t key;
        do {
            key = dist_(rng_);
        } while (key == 0 || shared_registry_.count(key));

        shared_registry_[key] = buffer; // 存储 weak_ptr，不影响生命周期
        return key;
    }

    /**
     * @brief DisplayService / 合成器通过 Key 找回显存实体
     */
    std::shared_ptr<VBuffer> getBufferByKey(uint32_t key) {
        std::lock_guard<std::mutex> lock(provider_mtx_);
        auto it = shared_registry_.find(key);
        if (it != shared_registry_.end()) {
            return it->second.lock(); // 若原主(VramUsr)已阵亡，这里安全返回 nullptr
        }
        return nullptr;
    }

private:
    VramProvider() : rng_(std::random_device{}()), dist_(1, 0xFFFFFFFF) {}
    ~VramProvider() = default;
    VramProvider(const VramProvider&) = delete;

    std::mutex provider_mtx_;
    
    // Tier 1 存储：所有在册的显存用户
    std::unordered_map<std::string, std::shared_ptr<VramUsr>> users_;
    
    // Tier 2 存储：用于跨进程通讯的 VBuffer 弱引用寻址表
    std::unordered_map<uint32_t, std::weak_ptr<VBuffer>> shared_registry_;

    std::mt19937 rng_;
    std::uniform_int_distribution<uint32_t> dist_;
};

} // namespace graphics
} // namespace suos