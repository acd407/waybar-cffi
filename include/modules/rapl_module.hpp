#ifndef WAYBAR_CFFI_RAPL_MODULE_HPP
#define WAYBAR_CFFI_RAPL_MODULE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <chrono>
#include <filesystem>
#include <module_base.hpp>

namespace waybar::cffi::rapl {

// 配置结构体 - 使用double类型的阈值
struct RaplConfig : public base::ModuleConfigBase<double> {
    using ThresholdType = double;

    std::string sysfs_dir = "/sys/class/powercap/intel-rapl:0";
    std::string format_tooltip = "Package: {package_power}W\nCore: {core_power}W\nOther: {other_power}W";

    RaplConfig() {
        icons["default"] = "󰟩";
        formats["default"] = "{icon}\u2004{power}W";
        states["warning"] = 15.0;
        states["critical"] = 30.0;
    }

    // 重写parse_config方法以处理特定配置
    void parse_config(const wbcffi_config_entry *entries, size_t count) override {
        // 调用父类方法
        base::ModuleConfigBase<double>::parse_config(entries, count);

        // 解析特定配置
        sysfs_dir = common::get_config_value<std::string>(config_map, "sysfs-dir", sysfs_dir);
        format_tooltip = common::get_config_value<std::string>(config_map, "format-tooltip", format_tooltip);
    }
};

// RAPL数据结构体
struct RaplData {
    uint64_t package_energy = 0;
    uint64_t core_energy = 0;
    std::chrono::steady_clock::time_point timestamp;

    // 默认构造函数
    RaplData() = default;

    // 便利构造函数
    RaplData(uint64_t pkg_energy, uint64_t core_energy, std::chrono::steady_clock::time_point time)
        : package_energy(pkg_energy), core_energy(core_energy), timestamp(time) {}
};

// RAPL模块类
class RaplModule : public base::ModuleBase<RaplConfig> {
  public:
    RaplModule(const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len);
    ~RaplModule() = default;

    // 禁止拷贝和移动
    RaplModule(const RaplModule &) = delete;
    RaplModule &operator=(const RaplModule &) = delete;
    RaplModule(RaplModule &&) = delete;
    RaplModule &operator=(RaplModule &&) = delete;

    // 更新函数
    void update();

  private:
    // RAPL数据
    RaplData prev_data_;
    bool first_update_ = true;

    // 缓存的max_energy_range值
    uint64_t package_max_energy_range_ = 0;
    uint64_t core_max_energy_range_ = 0;

    // RAPL信息获取
    RaplData get_rapl_data() const;
    uint64_t get_energy_uj(const std::string &path) const;
    double calculate_power(uint64_t energy_diff, double time_diff_seconds) const;
};

} // namespace waybar::cffi::rapl

#endif // WAYBAR_CFFI_RAPL_MODULE_HPP
