#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "RgaBasic.hpp"

namespace SuOS::graphics {

/**
 * @brief RGA 链式任务调度核心 (单例管理多任务)
 * 职责：按 CPU 时序接力触发任务链，保证不积压内核。接受 VTimer 死亡倒计时。
 */
class RgaLink {
public:
    static RgaLink& getInstance() {
        static RgaLink instance;
        return instance;
    }

    /**
     * @brief 提交一条执行链给后台排队处理
     * @param chain 配置好的步骤集合
     * @param deadline 此条链路被严词要求的最晚截止时间点（VTimer 下发）
     * @param cb 全部跑通后的通知回调，带有跑成功的 step 数统计。
     */
    void submitChain(std::shared_ptr<RgaChain> chain, VTimerTimePoint deadline, RgaLinkCallback cb);

private:
    RgaLink();
    ~RgaLink();
    
    struct LinkContext {
        std::shared_ptr<RgaChain> chain;
        VTimerTimePoint deadline;
        RgaLinkCallback callback;
    };

    void workerRoutine();

    std::queue<std::shared_ptr<LinkContext>> _queue;
    std::mutex _queue_mtx;
    std::condition_variable _queue_cv;
    std::thread _worker_thread;
    bool _running = true;
};

} // namespace suos::graphics