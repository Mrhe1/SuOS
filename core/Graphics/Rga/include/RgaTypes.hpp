#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include"rga/im2d.h"
#include "VramProvider.hpp"

namespace SuOS::graphics {

// 错误码定义：0 表示成功，其他表示不同类型的错误或取消
constexpr uint32_t RGA_ERR_OK = 0;
constexpr uint32_t RGA_ERR_CANCELED = 1;
constexpr uint32_t RGA_ERR_HARDWARE = 2;

// 统一回调：仅使用错误码
using RgaCallback = std::function<void(uint32_t err_code)>;

struct RgaConfig {
    int width;
    int height;
    int format;
    int x_offset = 0; // 新增区域支持
    int y_offset = 0;
};

enum class RgaStepType { COPY, ROTATE, FILL, BLEND, OSD };

struct RgaStep {
    RgaStepType type;
    std::shared_ptr<VBuffer> src;
    std::shared_ptr<VBuffer> dst;
    RgaConfig src_cfg;
    RgaConfig dst_cfg;
    int angle = 0;
    uint32_t color = 0;
    int blend_mode = IM_ALPHA_BLEND_SRC_OVER; // 用于 BLEND
    im_osd_t osd_cfg;                         // 用于 OSD
};

// 链式构造器
class RgaChain {
public:
    RgaChain& addCopy(std::shared_ptr<VBuffer> src, 
                      std::shared_ptr<VBuffer> dst,
                      const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
        RgaStep step;
        step.type = RgaStepType::COPY;
        step.src = src;
        step.dst = dst;
        step.src_cfg = src_cfg;
        step.dst_cfg = dst_cfg;
        steps.push_back(step);
        return *this;
    }

    RgaChain& addRotate(std::shared_ptr<VBuffer> src, 
                        std::shared_ptr<VBuffer> dst,
                        const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int angle) {
        RgaStep step;
        step.type = RgaStepType::ROTATE;
        step.src = src;
        step.dst = dst;
        step.src_cfg = src_cfg;
        step.dst_cfg = dst_cfg;
        step.angle = angle;
        steps.push_back(step);
        return *this;
    }

    RgaChain& addFill(std::shared_ptr<VBuffer> dst, 
                      const RgaConfig& dst_cfg, uint32_t color) {
        RgaStep step;
        step.type = RgaStepType::FILL;
        step.dst = dst;
        step.dst_cfg = dst_cfg;
        step.color = color;
        steps.push_back(step);
        return *this;
    }

    RgaChain& addBlend(std::shared_ptr<VBuffer> src, 
                       std::shared_ptr<VBuffer> dst,
                       const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                       int mode = IM_ALPHA_BLEND_SRC_OVER) {
        RgaStep step;
        step.type = RgaStepType::BLEND;
        step.src = src;
        step.dst = dst;
        step.src_cfg = src_cfg;
        step.dst_cfg = dst_cfg;
        step.blend_mode = mode;
        steps.push_back(step);
        return *this;
    }

    RgaChain& addOsd(std::shared_ptr<VBuffer> src, 
                     std::shared_ptr<VBuffer> dst,
                     const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                     const im_osd_t& osd_cfg) {
        RgaStep step;
        step.type = RgaStepType::OSD;
        step.src = src;
        step.dst = dst;
        step.src_cfg = src_cfg;
        step.dst_cfg = dst_cfg;
        step.osd_cfg = osd_cfg;
        steps.push_back(step);
        return *this;
    }

    std::vector<RgaStep> steps;
};

} // namespace SuOS::graphics