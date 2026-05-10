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

enum class RgaStepType { 
    COPY, 
    ROTATE, 
    FILL, 
    BLEND, 
    OSD, 
    RESIZE, 
    CROP, 
    COLOR_CONVERT, 
    FLIP, 
    COMPOSITE, 
    COLOR_KEY, 
    MOSAIC, 
    RECTANGLE,
    FILL_ARRAY,
    RECTANGLE_ARRAY,
    MOSAIC_ARRAY
};

struct RgaStep {
    RgaStepType type;
    std::shared_ptr<VBuffer> src;
    std::shared_ptr<VBuffer> dst;
    std::shared_ptr<VBuffer> pat; // 用于 composite 或其他三参数任务
    RgaConfig src_cfg;
    RgaConfig dst_cfg;
    RgaConfig pat_cfg;
    
    int angle = 0;
    uint32_t color = 0;
    int mode = 0;           // 兼用 blend_mode, flip_mode, mosaic_mode, etc.
    int thickness = -1;      // 用于 RECTANGLE
    double fx = 0;          // 用于 RESIZE
    double fy = 0;
    
    im_osd_t osd_cfg;
    im_colorkey_range ck_range;
    std::vector<im_rect> rects; // 用于 Array 系列任务
};

// 链式构造器
class RgaChain {
public:
    // 拷贝任务：将 src 拷贝到 dst
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

    // 旋转任务：将 src 进行指定角度旋转后存入 dst
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

    // 填充任务：使用指定颜色填充目标区域
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

    // Alpha 混合任务：将 src 混合到 dst
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
        step.mode = mode;
        steps.push_back(step);
        return *this;
    }

    // OSD 叠加任务：将 osd_src 叠加到背景 dst
    RgaChain& addOsd(std::shared_ptr<VBuffer> osd_src, 
                     std::shared_ptr<VBuffer> bg_dst,
                     const RgaConfig& osd_cfg_rect, 
                     const im_osd_t& osd_params) {
        RgaStep step;
        step.type = RgaStepType::OSD;
        step.src = osd_src;
        step.dst = bg_dst;
        step.dst_cfg = osd_cfg_rect; // osd_rect maps to dst_cfg for position
        step.osd_cfg = osd_params;
        steps.push_back(step);
        return *this;
    }

    // 缩放任务：支持指定 X 和 Y 方向的缩放比例
    RgaChain& addResize(std::shared_ptr<VBuffer> src, std::shared_ptr<VBuffer> dst,
                        const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                        double fx = 0, double fy = 0) {
        RgaStep step;
        step.type = RgaStepType::RESIZE;
        step.src = src; step.dst = dst;
        step.src_cfg = src_cfg; step.dst_cfg = dst_cfg;
        step.fx = fx; step.fy = fy;
        steps.push_back(step);
        return *this;
    }

    // 裁剪任务：从 src 中裁剪指定区域到 dst
    RgaChain& addCrop(std::shared_ptr<VBuffer> src, std::shared_ptr<VBuffer> dst,
                      const RgaConfig& src_cfg, const RgaConfig& dst_cfg) {
        RgaStep step;
        step.type = RgaStepType::CROP;
        step.src = src; step.dst = dst;
        step.src_cfg = src_cfg; step.dst_cfg = dst_cfg;
        steps.push_back(step);
        return *this;
    }

    // 颜色空间转换任务
    RgaChain& addConvert(std::shared_ptr<VBuffer> src, std::shared_ptr<VBuffer> dst,
                         const RgaConfig& src_cfg, const RgaConfig& dst_cfg,
                         int mode = IM_COLOR_SPACE_DEFAULT) {
        RgaStep step;
        step.type = RgaStepType::COLOR_CONVERT;
        step.src = src; step.dst = dst;
        step.src_cfg = src_cfg; step.dst_cfg = dst_cfg;
        step.mode = mode;
        steps.push_back(step);
        return *this;
    }

    // 翻转任务：水平或垂直翻转
    RgaChain& addFlip(std::shared_ptr<VBuffer> src, std::shared_ptr<VBuffer> dst,
                      const RgaConfig& src_cfg, const RgaConfig& dst_cfg, int mode) {
        RgaStep step;
        step.type = RgaStepType::FLIP;
        step.src = src; step.dst = dst;
        step.src_cfg = src_cfg; step.dst_cfg = dst_cfg;
        step.mode = mode;
        steps.push_back(step);
        return *this;
    }

    // 复合图像任务：处理前层 fg 和后层 bg 到 dst
    RgaChain& addComposite(std::shared_ptr<VBuffer> fg, std::shared_ptr<VBuffer> bg, std::shared_ptr<VBuffer> dst,
                           const RgaConfig& fg_cfg, const RgaConfig& bg_cfg, const RgaConfig& dst_cfg,
                           int mode = IM_ALPHA_BLEND_SRC_OVER) {
        RgaStep step;
        step.type = RgaStepType::COMPOSITE;
        step.src = fg; step.pat = bg; step.dst = dst;
        step.src_cfg = fg_cfg; step.pat_cfg = bg_cfg; step.dst_cfg = dst_cfg;
        step.mode = mode;
        steps.push_back(step);
        return *this;
    }

    // 抠图任务（ColorKey）：根据颜色范围去除背景
    RgaChain& addColorKey(std::shared_ptr<VBuffer> fg, std::shared_ptr<VBuffer> bg_dst,
                          const RgaConfig& fg_cfg, const RgaConfig& bg_dst_cfg,
                          im_colorkey_range range, int mode = IM_ALPHA_COLORKEY_NORMAL) {
        RgaStep step;
        step.type = RgaStepType::COLOR_KEY;
        step.src = fg; step.dst = bg_dst;
        step.src_cfg = fg_cfg; step.dst_cfg = bg_dst_cfg;
        step.ck_range = range; step.mode = mode;
        steps.push_back(step);
        return *this;
    }

    // 马赛克任务：在目标区域应用马赛克
    RgaChain& addMosaic(std::shared_ptr<VBuffer> dst, const RgaConfig& dst_cfg, int mosaic_mode) {
        RgaStep step;
        step.type = RgaStepType::MOSAIC;
        step.dst = dst; step.dst_cfg = dst_cfg;
        step.mode = mosaic_mode;
        steps.push_back(step);
        return *this;
    }

    // 矩形绘制任务：绘制矩形框或填充矩形
    RgaChain& addRectangle(std::shared_ptr<VBuffer> dst, const RgaConfig& dst_cfg, 
                           uint32_t color, int thickness = -1) {
        RgaStep step;
        step.type = RgaStepType::RECTANGLE;
        step.dst = dst; step.dst_cfg = dst_cfg;
        step.color = color; step.thickness = thickness;
        steps.push_back(step);
        return *this;
    }

    // 填充数组任务：批量填充多个区域
    RgaChain& addFillArray(std::shared_ptr<VBuffer> dst, int format, int w_stride, int h_stride,
                           const std::vector<im_rect>& rects, uint32_t color) {
        RgaStep step;
        step.type = RgaStepType::FILL_ARRAY;
        step.dst = dst;
        step.dst_cfg = {w_stride, h_stride, format, 0, 0};
        step.rects = rects;
        step.color = color;
        steps.push_back(step);
        return *this;
    }

    // 矩形绘制数组任务：批量绘制多个矩形
    RgaChain& addRectangleArray(std::shared_ptr<VBuffer> dst, int format, int w_stride, int h_stride,
                                const std::vector<im_rect>& rects, uint32_t color, int thickness = -1) {
        RgaStep step;
        step.type = RgaStepType::RECTANGLE_ARRAY;
        step.dst = dst;
        step.dst_cfg = {w_stride, h_stride, format, 0, 0};
        step.rects = rects;
        step.color = color; step.thickness = thickness;
        steps.push_back(step);
        return *this;
    }

    // 马赛克数组任务：批量在多个区域应用马赛克
    RgaChain& addMosaicArray(std::shared_ptr<VBuffer> dst, int format, int w_stride, int h_stride,
                             const std::vector<im_rect>& rects, int mosaic_mode) {
        RgaStep step;
        step.type = RgaStepType::MOSAIC_ARRAY;
        step.dst = dst;
        step.dst_cfg = {w_stride, h_stride, format, 0, 0};
        step.rects = rects;
        step.mode = mosaic_mode;
        steps.push_back(step);
        return *this;
    }

    std::vector<RgaStep> steps;
};

} // namespace SuOS::graphics