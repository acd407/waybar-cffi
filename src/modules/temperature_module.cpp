#include <modules/temperature_module.hpp>
#include <fstream>
#include <common.hpp>
#include <filesystem>

namespace waybar::cffi::temperature {

// TemperatureModule实现
TemperatureModule::TemperatureModule(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : base::ModuleBase<TemperatureConfig>(init_info, config_entries, config_entries_len) {
    update();
}

void TemperatureModule::update() {
    // 获取当前温度
    float temperature_c = get_temperature();

    // 转换为其他温度单位
    int temperature_c_int = static_cast<int>(std::round(temperature_c));
    int temperature_f_int = static_cast<int>(std::round((temperature_c * 1.8) + 32));
    int temperature_k_int = static_cast<int>(std::round(temperature_c + 273.15));

    // 使用get_state方法设置CSS类并获取状态名称
    std::string state_name = get_state(temperature_c_int);

    // 获取对应的图标和格式
    const std::string &icon = get_icon_for_state_name(state_name);
    const std::string &format = get_format_for_state_name(state_name);

    // 使用公共工具格式化输出
    std::string temperature_c_str = std::to_string(temperature_c_int);
    std::string temperature_f_str = std::to_string(temperature_f_int);
    std::string temperature_k_str = std::to_string(temperature_k_int);

    std::string display_text = common::safe_execute<std::string>(
        [&]() {
            return common::format_string<std::string>(
                format, {{"icon", icon},
                         {"temperature_c", temperature_c_str},
                         {"temperature_f", temperature_f_str},
                         {"temperature_k", temperature_k_str}}
            );
        },
        format + " " + icon + " " + temperature_c_str, "Error formatting output"
    );

    // 更新标签
    gtk_label_set_text(GTK_LABEL(label_), display_text.c_str());

    // 设置tooltip
    if (config().tooltip) {
        std::string tooltip = common::safe_execute<std::string>(
            [&]() {
                // 使用泛型版本的format_string，直接传递int值
                return common::format_string<int>(
                    config().format_tooltip, {{"temperature_c", temperature_c_int},
                                              {"temperature_f", temperature_f_int},
                                              {"temperature_k", temperature_k_int}}
                );
            },
            config().format_tooltip, "Error formatting tooltip"
        );

        gtk_widget_set_tooltip_text(event_box_, tooltip.c_str());
    } else {
        gtk_widget_set_has_tooltip(event_box_, FALSE);
    }
}

float TemperatureModule::get_temperature() const {
    std::ifstream file(config_->hwmon_path);
    if (!file.is_open()) {
        common::log_error("Failed to open temperature file: " + config_->hwmon_path);
        return 0.0f;
    }

    std::string line;
    if (!std::getline(file, line)) {
        common::log_error("Failed to read temperature from: " + config_->hwmon_path);
        return 0.0f;
    }

    // 温度值通常以毫摄氏度为单位存储
    auto temperature_c = double(std::strtol(line.c_str(), nullptr, 10)) / 1000.0;
    return static_cast<float>(temperature_c);
}

#define MODULENAME TemperatureModule
#include <wbcffi.txt>
#undef MODULENAME

} // namespace waybar::cffi::temperature