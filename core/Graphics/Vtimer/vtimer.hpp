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
#include "suRuntime.hpp" 

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
         * @brief 提交任务到绝对的 VSync 周期与偏移时刻
         * @param vsync_num 目标 VSync 绝对序列号
         * @param offset 子帧时间点 (t0, t1, t2, t3)
         * @param task 具体的执行任务
         */
        void submitTask(uint64_t vsync_num, VSyncOffset offset, std::function<void()> task) {
            provider_.scheduleTask(name_, vsync_num, offset, std::move(task));
        }

        /**
         * @brief 获取当前 VSync 计数和自上次 VSync 以来的微秒数
         * @param out_vsync_num 输出当前的 VSync 序号
         * @param out_us_offset 输出当前相对于 VSync 起点的微秒偏移
         */
        void getCurrentTime(uint64_t& out_vsync_num, uint64_t& out_us_offset) const {
            provider_.getCurrentTime(out_vsync_num, out_us_offset);
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
     * @brief 获取当前的 VSync 计数及当前的微秒偏移量
     */
    void getCurrentTime(uint64_t& vsync_num, uint64_t& us_offset) {
        std::lock_guard<std::mutex> lock(mtx_);
        vsync_num = current_frame_count_;
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - last_vsync_time_);
        us_offset = duration.count();
    }

    /**
     * @brief 硬件驱动级调用：触发 VSync
     * 当底层的 DRM/VOP 产生 VSync 中断时，硬件驱动调用此函数。
     */
    void triggerHardwareVSync() {
        std::lock_guard<std::mutex> lock(mtx_);
        last_vsync_time_ = std::chrono::steady_clock::now();
        
        uint64_t trigger_frame = current_frame_count_; // 当前被触发处理的任务帧号
        current_frame_count_++; // 下一帧的基准

        // 查找当前帧是否有挂载的任务
        auto it = frame_task_queues_.find(trigger_frame);
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

    void scheduleTask(const std::string& owner, uint64_t vsync_num, VSyncOffset offset, std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mtx_);
        // 只有目标帧号大于或等于当前正在处理/等待的帧时才有效
        if (vsync_num >= current_frame_count_) {
            frame_task_queues_[vsync_num].push_back({offset, std::move(task), owner});
        }
    }

    SuOS::Runtime::suRuntime* runtime_;
    std::mutex mtx_;
    
    // 全局自增帧计数器
    uint64_t current_frame_count_;
    // 上一次 VSync 触发的任务时间点
    std::chrono::steady_clock::time_point last_vsync_time_;

    // 核心存储表：帧号 -> 任务列表
    std::unordered_map<uint64_t, std::vector<VTask>> frame_task_queues_;
};

} // namespace Runtime
} // namespace SuOS
#endif // SU_VTIMER_HPP