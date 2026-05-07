#pragma once
#ifndef SU_VTIMER_HPP
#define SU_VTIMER_HPP

#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <boost/asio.hpp>

// 假设已包含你提供的 suRuntime 头文件
#include "su_runtime.hpp" 

namespace SuOS {
namespace Runtime {

// 定义子帧时间偏移 (单位：微秒)
enum class VSyncOffset : uint32_t {
    t0 = 0,        // 0 ms (VSync 触发瞬间)
    t1 = 600,      // 0.6 ms (例如：用于处理紧急的输入事件)
    t2 = 8000,     // 8.0 ms (例如：用于发起 RGA 合成)
    t3 = 16000     // 16.0 ms (紧贴下一次 VSync，通常用于收尾)
};

class VTimerProvider {
public:
    // 内部类：表示一个具体的定时任务
    struct VTask {
        VSyncOffset offset;
        std::function<void()> func;
        std::string owner_name;
    };

    // 内部类：VUsr (用户句柄)，负责提交任务
    class VUsr {
    public:
        VUsr(const std::string& name, VTimerProvider& provider)
            : name_(name), provider_(provider) {}

        /**
         * @brief 提交任务到指定的 VSync 周期与偏移时刻
         * @param frames_ahead 目标 VSync，0代表当前即将到来的下一个VSync，1代表下下个，以此类推
         * @param offset 子帧时间点 (t0, t1, t2, t3)
         * @param task 具体的执行任务
         */
        void submitTask(uint32_t frames_ahead, VSyncOffset offset, std::function<void()> task) {
            provider_.scheduleTask(name_, frames_ahead, offset, std::move(task));
        }

        const std::string& getName() const { return name_; }

    private:
        std::string name_;
        VTimerProvider& provider_;
    };

public:
    // 静态单例接口
    static VTimerProvider& getInstance() {
        static VTimerProvider instance;
        return instance;
    }

    // 必须在系统启动时绑定当前的 suRuntime
    void attachRuntime(suRuntime* runtime) {
        std::lock_guard<std::mutex> lock(mtx_);
        runtime_ = runtime;
    }

    // 创建用户句柄
    std::unique_ptr<VUsr> createVUsr(const std::string& name) {
        return std::make_unique<VUsr>(name, *this);
    }

    /**
     * @brief 硬件驱动级调用：触发 VSync
     * 当底层的 DRM/VOP 产生 VSync 中断时，硬件驱动调用此函数。
     * 这将释放当前帧绑定的所有延时任务。
     */
    void triggerHardwareVSync() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!runtime_) return;

        uint64_t current_target = current_frame_count_;
        current_frame_count_++; // 推进全局帧计数器

        // 查找当前帧是否有挂载的任务
        auto it = frame_task_queues_.find(current_target);
        if (it == frame_task_queues_.end()) return;

        // 取出当前帧的所有任务
        std::vector<VTask> tasks_to_run = std::move(it->second);
        frame_task_queues_.erase(it);

        // 分发任务：通过 Boost.Asio 定时器进行微秒级延迟分发
        for (auto& task : tasks_to_run) {
            if (task.offset == VSyncOffset::t0) {
                // t0 任务无延迟，直接扔进 Runtime 线程池
                runtime_->execute(std::move(task.func));
            } else {
                // 带有子帧延迟的任务
                uint32_t delay_us = static_cast<uint32_t>(task.offset);
                auto timer = std::make_shared<boost::asio::steady_timer>(
                    runtime_->getIoContext(), std::chrono::microseconds(delay_us));

                // 捕获 task.func 和 runtime 指针
                timer->async_wait([timer, func = std::move(task.func), runtime = this->runtime_](const boost::system::error_code& ec) {
                    if (!ec && runtime) {
                        // 时间一到，扔进线程池执行
                        runtime->execute(func);
                    }
                });
            }
        }
    }

private:
    friend class VUsr;

    VTimerProvider() : runtime_(nullptr), current_frame_count_(0) {}
    ~VTimerProvider() = default;
    VTimerProvider(const VTimerProvider&) = delete;

    void scheduleTask(const std::string& owner, uint32_t frames_ahead, VSyncOffset offset, std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mtx_);
        uint64_t target_frame = current_frame_count_ + frames_ahead;
        
        frame_task_queues_[target_frame].push_back({offset, std::move(task), owner});
    }

    SuOS::Runtime::suRuntime* runtime_;
    std::mutex mtx_;
    
    // 全局自增帧计数器
    uint64_t current_frame_count_;

    // 核心存储表：帧号 -> 任务列表
    std::unordered_map<uint64_t, std::vector<VTask>> frame_task_queues_;
};

} // namespace Runtime
} // namespace SuOS
#endif // SU_VTIMER_HPP