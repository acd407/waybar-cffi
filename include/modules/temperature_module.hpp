#ifndef WAYBAR_CFFI_TEMPERATURE_MODULE_HPP
#define WAYBAR_CFFI_TEMPERATURE_MODULE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <module_base.hpp>
#include <filesystem>

namespace waybar::cffi::temperature {

// 配置结构体 - 使用int类型的阈值
struct TemperatureConfig : public base::ModuleConfigBase<int> {
    using ThresholdType = int;

    std::string hwmon_path;

    TemperatureConfig() {
        icons["default"] = "";
        formats["default"] = "{icon}\u2004{temperature_c}°C";
        format_tooltip = "Temperature: {temperature_c}°C\nFahrenheit: {temperature_f}°F\nKelvin: {temperature_k}K";
        states["warning"] = 60;
        states["critical"] = 80;
    }

    // 重写parse_config方法以处理特定配置
    void parse_config(const wbcffi_config_entry *entries, size_t count) override {
        base::ModuleConfigBase<int>::parse_config(entries, count);
        hwmon_path = common::get_config_value<std::string>(config_map, "hwmon-path", hwmon_path);
    }
};

// 温度模块类
class TemperatureModule : public base::ModuleBase<TemperatureConfig> {
  public:
    TemperatureModule(
        const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
    );
    ~TemperatureModule() = default;

    // 禁止拷贝和移动
    TemperatureModule(const TemperatureModule &) = delete;
    TemperatureModule &operator=(const TemperatureModule &) = delete;
    TemperatureModule(TemperatureModule &&) = delete;
    TemperatureModule &operator=(TemperatureModule &&) = delete;

    // 温度模块的更新方法
    void update() override;

  private:
    // 获取温度值
    float get_temperature() const;
};

} // namespace waybar::cffi::temperature

#endif // WAYBAR_CFFI_TEMPERATURE_MODULE_HPP
