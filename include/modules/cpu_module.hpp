#ifndef WAYBAR_CFFI_CPU_MODULE_HPP
#define WAYBAR_CFFI_CPU_MODULE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <module_base.hpp>

namespace waybar::cffi::cpu {

// 配置结构体 - 使用int类型的阈值
struct CpuConfig : public base::ModuleConfigBase<int> {
    using ThresholdType = int;

    CpuConfig() {
        icons["default"] = "󰾆";
        formats["default"] = "{icon}\u2004{usage}%";
        format_tooltip = "CPU Usage: {usage}%\nState: {state}";
    }
};

// CPU模块类
class CpuModule : public base::ModuleBase<CpuConfig> {
  public:
    CpuModule(const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len);
    ~CpuModule() = default;

    // 禁止拷贝和移动
    CpuModule(const CpuModule &) = delete;
    CpuModule &operator=(const CpuModule &) = delete;
    CpuModule(CpuModule &&) = delete;
    CpuModule &operator=(CpuModule &&) = delete;

    // 更新函数
    void update();

  private:
    // CPU信息获取
    struct CpuTimes {
        uint64_t idle;
        uint64_t total;
    } prev_times = {0, 0};
    CpuTimes get_cpu_times() const;
    float calculate_cpu_usage(const CpuTimes &prev, const CpuTimes &curr) const;
};

} // namespace waybar::cffi::cpu

#endif // WAYBAR_CFFI_CPU_MODULE_HPP
