#include <modules/rapl_module.hpp>
#include <fstream>
#include <common.hpp>

namespace waybar::cffi::rapl {

// RaplModule实现
RaplModule::RaplModule(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : base::ModuleBase<RaplConfig>(init_info, config_entries, config_entries_len) {
    // 内联路径获取函数
    auto package_path = config().sysfs_dir + "/energy_uj";
    auto core_path = config().sysfs_dir + ":0/energy_uj";
    auto package_max_energy_range_path = config().sysfs_dir + "/max_energy_range_uj";
    auto core_max_energy_range_path = config().sysfs_dir + ":0/max_energy_range_uj";

    // 检查RAPL文件是否存在
    if (!std::filesystem::exists(package_path) || !std::filesystem::exists(core_path) ||
        !std::filesystem::exists(package_max_energy_range_path) ||
        !std::filesystem::exists(core_max_energy_range_path)) {
        throw std::runtime_error("RAPL sysfs files not found");
    }

    // 在初始化时读取并缓存max_energy_range值
    package_max_energy_range_ = get_energy_uj(package_max_energy_range_path);
    core_max_energy_range_ = get_energy_uj(core_max_energy_range_path);

    // 初始更新
    update();
}

void RaplModule::update() {
    // 获取当前RAPL数据
    RaplData current_data = get_rapl_data();

    // 计算功耗
    double package_power = 0.0;
    double core_power = 0.0;

    if (!first_update_) {
        // 计算时间差（秒）
        std::chrono::duration<double> time_diff = current_data.timestamp - prev_data_.timestamp;
        double seconds = time_diff.count();

        if (seconds > 0) {
            // 计算能量差（微焦耳）
            uint64_t package_energy_diff = current_data.package_energy - prev_data_.package_energy;
            uint64_t core_energy_diff = current_data.core_energy - prev_data_.core_energy;

            // 处理计数器回绕 - 使用缓存的max_energy_range值
            if (package_max_energy_range_ > 0 && package_energy_diff > package_max_energy_range_ / 2) {
                package_energy_diff += package_max_energy_range_;
            }

            if (core_max_energy_range_ > 0 && core_energy_diff > core_max_energy_range_ / 2) {
                core_energy_diff += core_max_energy_range_;
            }

            // 计算功耗（瓦特 = 焦耳/秒）
            package_power = calculate_power(package_energy_diff, seconds);
            core_power = calculate_power(core_energy_diff, seconds);
        }
    }

    // 更新上一次的值
    prev_data_ = current_data;
    first_update_ = false;

    // 计算其他功耗（非核心部分）
    double other_power = package_power - core_power;

    // 使用get_state方法设置CSS类并获取状态名称
    std::string state_name = get_state(package_power);

    // 获取对应的图标和格式
    const std::string &icon = get_icon_for_state_name(state_name);
    const std::string &format = get_format_for_state_name(state_name);

    // 定义format_args，供format和tooltip共同使用
    std::vector<std::pair<std::string, waybar::cffi::common::format_arg>> args = {
        {"icon", icon},
        {"power", common::format_number(package_power)},
        {"package_power", common::format_number(package_power)},
        {"core_power", common::format_number(core_power)},
        {"other_power", common::format_number(other_power)}
    };

    std::string display_text = common::safe_execute<std::string>(
        [&]() { return common::format_string(format, args); },
        format + " " + icon + " " + common::format_number(package_power), "Error formatting output"
    );

    // 更新标签
    gtk_label_set_text(GTK_LABEL(label_), display_text.c_str());

    // 设置tooltip
    if (config().tooltip) {
        auto tooltip_format = get_tooltip_format();
        std::string tooltip = common::safe_execute<std::string>(
            [&]() { return common::format_string(tooltip_format, args); },
            icon + " " + common::format_number(package_power), "Error formatting tooltip"
        );

        gtk_widget_set_tooltip_text(event_box_, tooltip.c_str());
    } else {
        gtk_widget_set_has_tooltip(event_box_, FALSE);
    }
}

RaplData RaplModule::get_rapl_data() const {
    // 内联路径获取函数
    auto package_path = config().sysfs_dir + "/energy_uj";
    auto core_path = config().sysfs_dir + ":0/energy_uj";

    // 读取当前能量值
    uint64_t package_energy = get_energy_uj(package_path);
    uint64_t core_energy = get_energy_uj(core_path);

    // 获取当前时间
    auto current_time = std::chrono::steady_clock::now();

    return RaplData(package_energy, core_energy, current_time);
}

uint64_t RaplModule::get_energy_uj(const std::string &path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return 0;
    }

    uint64_t energy = 0;
    file >> energy;
    return energy;
}

double RaplModule::calculate_power(uint64_t energy_diff, double time_diff_seconds) const {
    // 转换为焦耳，然后除以时间得到瓦特
    return (double(energy_diff) / 1000000.0) / time_diff_seconds;
}

#define MODULENAME RaplModule
#include <wbcffi.txt>
#undef MODULENAME

} // namespace waybar::cffi::rapl