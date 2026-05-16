#pragma once
#include "RgaTypes.hpp"
#include "rga/im2d.h"
#include <mutex>
#include <thread>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <memory>

namespace SuOS::graphics {

class RgaEngine;

/**
 * @brief RGA 异步任务句柄
 * 包含输入输出 VBuffer 引用、文件描述符、内核 job ids 以及回调函数
 */
class RgaJobHandle {
public:
    RgaJobHandle(RgaEngine* engine) : _engine(engine) {}
    ~RgaJobHandle();

    // 提供给用户的取消接口，任意 step 均可主动掐断
    void cancel();

    // --- 以下为引擎内部使用字段 ---
    std::vector<std::shared_ptr<VBuffer>> vbuffers_ref; // 保活链
    std::vector<im_job_handle_t> rga_job_ids; // 压入内核的事务 id，用于 cancel
    int final_fence_fd = -1; // 最后一个任务的输出 Fence FD，绑定 epoll
    RgaCallback user_callback;

private:
    RgaEngine* _engine;
    bool _is_completed_or_canceled = false;
    std::mutex _handle_mtx;
};


/**
 * @brief RGA Epoll 异步驱动引擎 (合并了 Basic 与 Link)
 */
class RgaEngine {
public:
    static RgaEngine& getInstance() {
        static RgaEngine instance;
        return instance;
    }

    /**
     * @brief 提交 RGA 任务链
     * @param chain 任务链描述
     * @param cb 回调函数。如果为 nullptr，则本函数会返回最后一个任务的 fence fd；
     *           如果提供了回调，任务完成后会自动触发回调，并返回 0。
     * @return int 成功时返回 fence fd (无回调) 或 0 (有回调)；失败返回 -1
     */
    int submit(const RgaChain& chain, RgaCallback cb = nullptr, int in_fence_fd = -1);

    // 引擎内部：从 epoll 树和活跃列表中移除句柄
    void removeHandle(RgaJobHandle* handle_ptr, uint32_t final_err_code);

private:
    RgaEngine();
    ~RgaEngine();

    void epollWorkerRoutine();
    rga_buffer_t wrapBuffer(std::shared_ptr<VBuffer> buf, const RgaConfig& cfg);
    IM_USAGE getRotationUsage(int angle);
    // format定义见芯片rk的rga.h
    int getColorFormatMapping(int fmt); // --- IGNORE ---

    int _epoll_fd;
    bool _running;
    std::thread _epoll_thread;

    // 维护正在硬件中流转的 Handle 的生命周期
    std::vector<std::shared_ptr<RgaJobHandle>> _active_handles;
    std::mutex _handles_mtx;
};

} // namespace SuOS::graphics