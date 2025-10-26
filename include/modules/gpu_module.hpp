#ifndef WAYBAR_CFFI_GPU_MODULE_HPP
#define WAYBAR_CFFI_GPU_MODULE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <module_base.hpp>

namespace waybar::cffi::gpu {

// 配置结构体 - 使用int类型的阈值
struct GpuConfig : public base::ModuleConfigBase<int> {
    using ThresholdType = int;

    std::string gpu_usage_path = "/sys/class/drm/card1/device/gpu_busy_percent";
    std::string vram_used_path = "/sys/class/drm/card1/device/mem_info_vram_used";
    std::string format_tooltip = "GPU: {gpu_usage}%\nVRAM: {vram_used}G";

    GpuConfig() {
        icons["default"] = "󰍹";
        formats["default"] = "{icon}\u2004{gpu_usage:>2}%";
        formats["alt"] = "{icon}\u2004{vram_used}GB";
        states["warning"] = 20;
        states["critical"] = 50;
    }

    // 重写parse_config方法以处理特定配置
    void parse_config(const wbcffi_config_entry *entries, size_t count) override {
        // 调用父类方法
        base::ModuleConfigBase<int>::parse_config(entries, count);

        // 解析特定配置
        gpu_usage_path = common::get_config_value<std::string>(config_map, "gpu-usage-path", gpu_usage_path);
        vram_used_path = common::get_config_value<std::string>(config_map, "vram-used-path", vram_used_path);
        format_tooltip = common::get_config_value<std::string>(config_map, "format-tooltip", format_tooltip);
    }
};

// GPU模块类
class GpuModule : public base::ModuleBase<GpuConfig> {
  public:
    GpuModule(const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len);
    ~GpuModule() = default;

    // 禁止拷贝和移动
    GpuModule(const GpuModule &) = delete;
    GpuModule &operator=(const GpuModule &) = delete;
    GpuModule(GpuModule &&) = delete;
    GpuModule &operator=(GpuModule &&) = delete;

    // 更新函数
    void update() override;

    // 处理点击事件，用于切换显示模式
    gboolean handle_button_press(GdkEventButton *event) override;

  private:
    // 当前使用的格式键，"default"或"alt"
    std::string current_format_key_ = "default";

    // GPU信息获取
    int get_gpu_usage() const;
    double get_vram_used() const; // 返回GB单位
};

} // namespace waybar::cffi::gpu

#endif // WAYBAR_CFFI_GPU_MODULE_HPP
