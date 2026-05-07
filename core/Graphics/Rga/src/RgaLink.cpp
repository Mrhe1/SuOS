#include "RgaLink.hpp"

namespace SuOS::graphics {

/**
 * @brief 使用安全锁封装单层 Step 的等待状态。
 * 这样做可以彻底解决“被撤消强制取消的任务回调比正常下一轮循环慢”导致的串线并发Bug。
 */
struct StepState {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    bool success = false;
    std::string err_msg;
};

RgaLink::RgaLink() {
    _worker_thread = std::thread(&RgaLink::workerRoutine, this);
}

RgaLink::~RgaLink() {
    {
        std::lock_guard<std::mutex> lock(_queue_mtx);
        _running = false;
    }
    _queue_cv.notify_all();
    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }
}

void RgaLink::submitChain(std::shared_ptr<RgaChain> chain, VTimerTimePoint deadline, RgaLinkCallback cb) {
    auto context = std::make_shared<LinkContext>();
    context->chain = chain;
    context->deadline = deadline;
    context->callback = cb;

    {
        std::lock_guard<std::mutex> lock(_queue_mtx);
        _queue.push(context);
    }
    _queue_cv.notify_one();
}

void RgaLink::workerRoutine() {
    while (true) {
        std::shared_ptr<LinkContext> ctx;
        {
            std::unique_lock<std::mutex> lock(_queue_mtx);
            _queue_cv.wait(lock, [this] { return !_queue.empty() || !_running; });
            if (!_running && _queue.empty()) break;

            ctx = _queue.front();
            _queue.pop();
        }

        const auto& steps = ctx->chain->steps;
        int completed_steps = 0;
        bool chain_aborted = false;

        // 全文精髓：在 CPU 侧执行业务队列逻辑，逐个下发内核
        for (size_t i = 0; i < steps.size(); ++i) {
            
            // 每次循环前，审视整体时间线。时间早已经超过，此 Step 及以后的全部取消！
            if (std::chrono::steady_clock::now() > ctx->deadline) {
                if (ctx->callback) {
                    ctx->callback(false, "Aborted on step " + std::to_string(i) + " because VTimer Deadline passed", completed_steps);
                }
                chain_aborted = true;
                break;
            }

            const auto& step = steps[i];
            
            // 构建此步被分配的安全箱
            auto state = std::make_shared<StepState>();
            
            auto basic_cb = [state](bool success, const std::string& err) {
                std::lock_guard<std::mutex> l(state->mtx);
                state->success = success;
                state->err_msg = err;
                state->done = true;
                state->cv.notify_one(); // 通知调度主轴往下走
            };

            im_job_handle_t current_handle = 0;
            switch(step.type) {
                case RgaStepType::COPY:
                    current_handle = RgaBasic::asyncCopy(step.src, step.dst, step.src_cfg, step.dst_cfg, basic_cb);
                    break;
                case RgaStepType::ROTATE:
                    current_handle = RgaBasic::asyncRotate(step.src, step.dst, step.src_cfg, step.angle, basic_cb);
                    break;
                case RgaStepType::FILL:
                    current_handle = RgaBasic::asyncFill(step.dst, step.dst_cfg, step.color, basic_cb);
                    break;
            }

            // 本工作线程使用 cv 限时代替 sleep 开始挂起等待硬件的反馈信号
            std::unique_lock<std::mutex> st_lock(state->mtx);
            bool wait_res = state->cv.wait_until(st_lock, ctx->deadline, [state]{ return state->done; });

            // 判断挂起唤醒的原因
            if (!wait_res) {
                // 唤醒原因 1：到了 VTimer 的截止时分红线，基础任务还在内核里出不来。强制取消正在路上的！
                RgaBasic::cancel(current_handle);
                if (ctx->callback) {
                    ctx->callback(false, "VTimer Timeout while executing step " + std::to_string(i), completed_steps);
                }
                chain_aborted = true;
                break;
            } 
            else {
                // 唤醒原因 2：RgaBasic 中断来了，顺畅完成
                if (!state->success) {
                    if (ctx->callback) ctx->callback(false, "RGA Hardware Exception on step " + std::to_string(i) + ": " + state->err_msg, completed_steps);
                    chain_aborted = true;
                    break;
                }
            }

            completed_steps++; // 顺利完成，接通下一步
        }

        // 如果全部 Step 绿灯通行完毕，发出光荣喜报
        if (!chain_aborted && ctx->callback) {
            ctx->callback(true, "All steps completed successively", completed_steps);
        }
    }
}

} // namespace suos::graphics