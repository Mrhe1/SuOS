#include "RgaEngine.hpp"
#include <algorithm>
#include <cstring>
#include <sys/eventfd.h>
#include <unordered_map>
#include"rga/im2d.h"

namespace SuOS::graphics {

// ============== RgaJobHandle 实现 ==============

RgaJobHandle::~RgaJobHandle() {
    if (final_fence_fd >= 0) {
        close(final_fence_fd);
    }
}

void RgaJobHandle::cancel() {
    std::lock_guard<std::mutex> lock(_handle_mtx);
    if (_is_completed_or_canceled) return;

    // 1. 强杀所有已被记录的内核 job
    for (auto job_id : rga_job_ids) {
        if (job_id > 0) {
            imcancelJob(job_id);
        }
    }
    _is_completed_or_canceled = true;

    // 2. 通知引擎清理资源并回调
    _engine->removeHandle(this, RGA_ERR_CANCELED);
}

// ============== RgaEngine 实现 ==============

RgaEngine::RgaEngine() : _running(true) {
    _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    _epoll_thread = std::thread(&RgaEngine::epollWorkerRoutine, this);
}

RgaEngine::~RgaEngine() {
    _running = false;
    if (_epoll_thread.joinable()) {
        _epoll_thread.join();
    }
    close(_epoll_fd);
}

rga_buffer_t RgaEngine::wrapBuffer(std::shared_ptr<VBuffer> buf, const RgaConfig& cfg) {
    if (!buf) return {};

    // 1. 导入原始 fd 获取 handle
    // 这里使用 buf->getWidth/Height 作为分配时的原始参考
    rga_buffer_handle_t handle = importbuffer_fd(buf->getFd(), buf->getWidth(), buf->getHeight(), buf->getColorFormat());

    // 2. 将 handle 包装成 rga_buffer_t
    // 使用带 Stride 的版本（buf->getWidth() 是物理对齐宽度）
    rga_buffer_t rga_buf = wrapbuffer_handle(handle, 
                                             cfg.width,   // 逻辑宽
                                             cfg.height,  // 逻辑高
                                             cfg.format, 
                                             buf->getWidth(),  // wstride: 内存实际每行像素数
                                             buf->getHeight()); // hstride: 内存实际行数
    return rga_buf;
}

IM_USAGE RgaEngine::getRotationUsage(int angle) {
    switch(angle) {
        case 90:  return IM_HAL_TRANSFORM_ROT_90;
        case 180: return IM_HAL_TRANSFORM_ROT_180;
        case 270: return IM_HAL_TRANSFORM_ROT_270;
        default:  return (IM_USAGE)0;
    }
}

std::shared_ptr<RgaJobHandle> RgaEngine::submit(const RgaChain& chain, RgaCallback cb) {
    auto handle = std::make_shared<RgaJobHandle>(this);
    handle->user_callback = cb;

    // 1. 创建临时的 VBuffer 句柄映射池
    // 如果一条 Chain 中的多个 Step 涉及同几块 VBuffer，
    // 我们只在第一次对其进行 importbuffer_fd 映射，后续直接复用，从而提升性能并避免 fd 资源激增
    std::unordered_map<VBuffer*, rga_buffer_handle_t> handle_pool;

    auto get_bundled_buffer = [&](const std::shared_ptr<VBuffer>& buf, const RgaConfig& cfg) -> rga_buffer_t {
        if (!buf) return {};
        VBuffer* ptr = buf.get();
        if (handle_pool.find(ptr) == handle_pool.end()) {
            // 首次遇到此 VBuffer，执行硬件调用并绑定底层缓存
            handle_pool[ptr] = importbuffer_fd(ptr->getFd(), ptr->getWidth(), ptr->getHeight(), ptr->getColorFormat());
            handle->vbuffers_ref.push_back(buf); // 并保活原始对象以防跨任务期内被意外析构
        }
        // 从池子中取出句柄，附加本次操作按需声明的 cfg，生成即插即用的轻型包裹
        return wrapbuffer_handle(handle_pool[ptr], cfg.width, cfg.height, cfg.format, ptr->getWidth(), ptr->getHeight());
    };

    int current_in_fence = -1; 
    int current_out_fence = -1;

    for (const auto& step : chain.steps) {
        im_job_handle_t job_id = imbeginJob();
        handle->rga_job_ids.push_back(job_id);

        // 从池中按需获取组装好的 src / dst 参数，完美支持显存异构和重复组合
        rga_buffer_t r_src = get_bundled_buffer(step.src, step.src_cfg);
        rga_buffer_t r_dst = get_bundled_buffer(step.dst, step.dst_cfg);

        // 无论上层如何复用同一块显存，每一步的区域矩形始终由独立的 src_cfg/dst_cfg 来控制
        im_rect s_rect = {step.src_cfg.x_offset, step.src_cfg.y_offset, step.src_cfg.width, step.src_cfg.height};
        im_rect d_rect = {step.dst_cfg.x_offset, step.dst_cfg.y_offset, step.dst_cfg.width, step.dst_cfg.height};
        im_rect empty_rect = {0};

        switch (step.type) {
            case RgaStepType::COPY:
                improcessTask(job_id, r_src, r_dst, {}, s_rect, d_rect, empty_rect, nullptr, 0);
                break;
            case RgaStepType::ROTATE:
                improcessTask(job_id, r_src, r_dst, {}, s_rect, d_rect, empty_rect, nullptr, getRotationUsage(step.angle));
                break;
            case RgaStepType::FILL: {
                imfillTask(job_id, r_dst, d_rect, step.color);
                break;
            }
            case RgaStepType::BLEND: {
                improcessTask(job_id, r_src, r_dst, {}, s_rect, d_rect, empty_rect, nullptr, step.blend_mode);
                break;
            }
            case RgaStepType::OSD: {
                imosdTask(job_id, r_src, r_dst, d_rect, const_cast<im_osd_t*>(&step.osd_cfg));
                break;
            }
        }

        imendJob(job_id, IM_ASYNC, current_in_fence, &current_out_fence); 

        if (current_in_fence >= 0) {
            close(current_in_fence);
        }
        current_in_fence = current_out_fence; 
    }


    // 将整个 Chain 最后一步输出的 Fence 赋给 handle，准备监听
    handle->final_fence_fd = current_out_fence;

    // 加入存活列表
    {
        std::lock_guard<std::mutex> lock(_handles_mtx);
        _active_handles.push_back(handle);
    }

    // 挂载到 epoll 树上
    if (handle->final_fence_fd >= 0) {
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLERR | EPOLLONESHOT;
        ev.data.ptr = handle.get(); // data.ptr 直接指向原生指针
        epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, handle->final_fence_fd, &ev);
    } else {
        // 如果没有产生合法的 Fence（例如空 Chain），直接回调
        removeHandle(handle.get(), RGA_ERR_HARDWARE);
    }

    return handle;
}

void RgaEngine::removeHandle(RgaJobHandle* handle_ptr, uint32_t final_err_code) {
    std::shared_ptr<RgaJobHandle> keep_alive;

    {
        std::lock_guard<std::mutex> lock(_handles_mtx);
        auto it = std::find_if(_active_handles.begin(), _active_handles.end(),
                               [handle_ptr](const std::shared_ptr<RgaJobHandle>& ptr) {
                                   return ptr.get() == handle_ptr;
                               });
        if (it != _active_handles.end()) {
            keep_alive = *it; // 转移所有权到局部变量，防止在加锁区回调时被虚空释放
            _active_handles.erase(it);
        }
    }

    if (keep_alive) {
        // 从 epoll 树上注销
        if (keep_alive->final_fence_fd >= 0) {
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, keep_alive->final_fence_fd, nullptr);
        }
        // 执行用户级回调
        if (keep_alive->user_callback) {
            keep_alive->user_callback(final_err_code);
        }
    }
}

void RgaEngine::epollWorkerRoutine() {
    constexpr int MAX_EVENTS = 16;
    epoll_event events[MAX_EVENTS];

    while (_running) {
        int n = epoll_wait(_epoll_fd, events, MAX_EVENTS, 100); // 100ms timeout 防止线程无法退出
        if (n < 0 && errno != EINTR) break;

        for (int i = 0; i < n; ++i) {
            RgaJobHandle* handle = static_cast<RgaJobHandle*>(events[i].data.ptr);
            if (!handle) continue;

            uint32_t err_code = RGA_ERR_OK;
            if (events[i].events & EPOLLERR) {
                err_code = RGA_ERR_HARDWARE;
            }

            // Fence 被触发（无论是 OK 还是 Error），进入清理回收流程
            removeHandle(handle, err_code);
        }
    }
}

} // namespace SuOS::graphics