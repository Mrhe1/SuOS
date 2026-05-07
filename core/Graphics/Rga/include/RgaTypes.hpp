#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <chrono>

#include "im2d.h"
#include "RgaUtils.h"
#include "vram_provider.hpp" // 苏系统的显存管理模块

namespace SuOS::graphics {

// 视频/图像格式基础配置
struct RgaConfig {
    int width;
    int height;
    int format; // 如 RK_FORMAT_RGB_565, RK_FORMAT_RGBA_8888
};

// 回调函数模板定义
using RgaBasicCallback = std::function<void(bool success, const std::string& error_msg)>;
using RgaLinkCallback = std::function<void(bool success, const std::string& msg, int completed_steps)>;

// 对接 VTimer 的时间点
using VTimerTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

// 单一具体任务的类型
enum class RgaStepType {
    COPY,
    ROTATE,
    FILL
};

// 单步操作配置封装
struct RgaStep {
    RgaStepType type;
    
    std::shared_ptr<VramProvider::VBuffer> src;
    std::shared_ptr<VramProvider::VBuffer> dst;
    
    RgaConfig src_cfg;
    RgaConfig dst_cfg;
    
    int angle = 0;         // 针对 ROTATE
    uint32_t color = 0;    // 针对 FILL
};

// 任务链（由多个 Step 组成）
class RgaChain {
public:
    void addCopy(std::shared_ptr<VramProvider::VBuffer> src, 
                 std::shared_ptr<VramProvider::VBuffer> dst,
                 const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
        steps.push_back({RgaStepType::COPY, src, dst, src_cfg, dst_cfg, 0, 0});
    }

    void addRotate(std::shared_ptr<VramProvider::VBuffer> src, 
                   std::shared_ptr<VramProvider::VBuffer> dst,
                   const RgaConfig& src_cfg, int angle) {
        RgaConfig dst_cfg = src_cfg; // 旋转的目标通常与原始属性密切相关
        steps.push_back({RgaStepType::ROTATE, src, dst, src_cfg, dst_cfg, angle, 0});
    }

    void addFill(std::shared_ptr<VramProvider::VBuffer> dst, 
                 const RgaConfig& dst_cfg, uint32_t color) {
        steps.push_back({RgaStepType::FILL, nullptr, dst, {}, dst_cfg, 0, color});
    }

    std::vector<RgaStep> steps;
};

} // namespace suos::graphics