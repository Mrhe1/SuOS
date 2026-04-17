#ifndef SU_RUNTIME_HPP
#define SU_RUNTIME_HPP

#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include <functional> 
#include <chrono>

namespace SuOS::Runtime {
    class suRuntime {
    public:
        // 任务句柄，用于外部取消
        class ScheduledTask {
        public:
            explicit ScheduledTask(std::shared_ptr<boost::asio::steady_timer> t) : timer_(std::move(t)) {}

            void cancel() {
                if (timer_) {
                    timer_->cancel(); // 触发 asio 的取消信号
                }
            }

        private:
            std::shared_ptr<boost::asio::steady_timer> timer_;
        };

        suRuntime(std::size_t pool_size = 2)
            : work_guard_(boost::asio::make_work_guard(io_context_)),
            worker_pool_(pool_size) {

            // 使用 promise 确保在构造函数返回前，event_thread_id_ 已经被正确赋值
            std::promise<std::thread::id> id_promise;
            auto id_future = id_promise.get_future();

            event_thread_ = std::thread([this, &id_promise]() {
                event_thread_id_ = std::this_thread::get_id(); // 记录 IO 线程 ID
                id_promise.set_value(event_thread_id_);
                io_context_.run();
                });

            id_future.wait(); // 等待线程启动并赋值完成
        }

        ~suRuntime() {
            stop();
        }

        // 检查当前线程是否为 IO Event Loop 线程
        bool isInEventLoop() const {
            return std::this_thread::get_id() == event_thread_id_;
        }

        /**
         * @brief 确保任务在 Event Loop 中执行
         * 如果当前就在 IO 线程，立即同步执行；
         * 如果在其他线程（如 worker 线程或主线程），则 post 到 IO 线程。
         */
        void dispatch(std::function<void()> task) {
            if (isInEventLoop()) {
                task(); // 直接执行，无需排队
            }
            else {
                boost::asio::post(io_context_, std::move(task)); // 投递切换
            }
        }

        // 立即异步执行 (在 Thread Pool 中运行)
        void execute(std::function<void()> task) {
            boost::asio::post(worker_pool_, std::move(task));
        }

        // 延时执行 (Event Loop 计时 -> 到期后投递给 Thread Pool)
        void schedule(int delay_ms, bool heavy = true, std::function<void()> task) {
            // 使用 shared_ptr 管理 timer 生命周期，防止函数返回后 timer 被销毁
            auto timer = std::make_shared<boost::asio::steady_timer>(
                io_context_, std::chrono::milliseconds(delay_ms));

            timer->async_wait([this, task, timer](const boost::system::error_code& ec) {
                if (!ec) {
                    if (heavy) {
                        this->execute(task);      // 扔给线程池
                    }
                    else {
                        task();                   // 在 Event Loop 原地执行
                    }
                }
                });
        }

        // 回到主循环执行 (有些 UI 或状态操作必须在 Event Loop 线程)
        void runOnEventLoop(std::function<void()> task) {
            boost::asio::post(io_context_, std::move(task));
        }

        // 固定频率调度
        // initial_delay_ms: 第一次执行前的延迟
        // period_ms: 之后每次执行的间隔
        // 返回一个 shared_ptr 方便外部持有
        std::shared_ptr<ScheduledTask> scheduleAtFixedRate(int initial_delay_ms, int period_ms, bool heavy = true, std::function<void()> task) {

            auto timer = std::make_shared<boost::asio::steady_timer>(
                io_context_, std::chrono::milliseconds(initial_delay_ms));

            // 创建句柄返回给用户
            auto handle = std::make_shared<ScheduledTask>(timer);

            // 内部递归闭包
            auto self = std::make_shared<std::function<void(const boost::system::error_code&)>>();

            *self = [this, timer, period_ms, task, heavy, self](const boost::system::error_code& ec) {
                // 重要：如果 ec 为 operation_aborted，说明外部调用了 cancel()，直接退出递归
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                if (ec) return; // 其他错误

                // 执行业务逻辑
                if (heavy) {
                    this->execute(task);
                }
                else {
                    task();
                }

                // 排期下一次
                timer->expires_at(timer->expiry() + std::chrono::milliseconds(period_ms));
                timer->async_wait(*self);
                };

            timer->async_wait(*self);
            return handle;
        }

        void execute(std::function<void()> task) {
            boost::asio::post(worker_pool_, std::move(task));
        }

        void getIoContext() {
            return io_context_;
        }

        void stop() {
            io_context_.stop();
            if (event_thread_.joinable()) event_thread_.join();
            worker_pool_.join();
        }

    private:
        boost::asio::io_context io_context_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
        boost::asio::thread_pool worker_pool_;
        std::thread event_thread_;
        std::thread::id event_thread_id_;
    };
}
#endif // SU_RUNTIME_HPP