#include <modules/cpu_module.hpp>
#include <fstream>
#include <common.hpp>

namespace waybar::cffi::cpu {

// CpuModule实现
CpuModule::CpuModule(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : base::ModuleBase<CpuConfig>(init_info, config_entries, config_entries_len) {
    // 初始更新
    update();
}

void CpuModule::update() {
    // 获取当前CPU时间
    CpuTimes current_times = get_cpu_times();

    // 计算CPU使用率
    float usage = calculate_cpu_usage(prev_times, current_times);

    // 使用get_state方法设置CSS类并获取状态名称
    std::string state_name = get_state(usage);

    // 获取对应的图标和格式
    const std::string &icon = get_icon_for_state_name(state_name);
    const std::string &format = get_format_for_state_name(state_name);

    // 使用公共工具格式化输出
    std::string usage_str = common::format_number(usage);
    std::string display_text = common::safe_execute<std::string>(
        [&]() { return common::format_string<std::string>(format, {{"icon", icon}, {"usage", usage_str}}); },
        format + " " + icon + " " + usage_str, "Error formatting output"
    );

    // 更新标签
    gtk_label_set_text(GTK_LABEL(label_), display_text.c_str());

    // 设置tooltip
    if (config().tooltip) {
        std::string tooltip = common::format_string<std::string>(
            config().format_tooltip, {{"usage", common::format_number(usage)}, {"state", state_name}}
        );

        gtk_widget_set_tooltip_text(event_box_, tooltip.c_str());
    } else {
        gtk_widget_set_has_tooltip(event_box_, FALSE);
    }

    prev_times = current_times;
}

CpuModule::CpuTimes CpuModule::get_cpu_times() const {
    return common::safe_execute<CpuTimes>(
        []() {
            std::ifstream file("/proc/stat");
            if (!file) {
                throw std::runtime_error("Failed to open /proc/stat");
            }

            std::string line;
            if (!std::getline(file, line)) {
                throw std::runtime_error("Failed to read from /proc/stat");
            }

            // 解析CPU时间，格式: cpu user nice system idle iowait irq softirq steal guest
            // guest_nice
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
            if (sscanf(
                    line.c_str(), "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait,
                    &irq, &softirq, &steal, &guest, &guest_nice
                ) != 10) {
                throw std::runtime_error("Failed to parse /proc/stat");
            }

            // 计算总时间和空闲时间
            uint64_t idle_time = idle + iowait;
            uint64_t total_time = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;

            return CpuTimes{idle_time, total_time};
        },
        CpuTimes{0, 0}, "Error reading CPU times"
    );
}

float CpuModule::calculate_cpu_usage(const CpuTimes &prev, const CpuTimes &curr) const {
    if (curr.total <= prev.total) {
        return 0.0f;
    }

    uint64_t total_diff = curr.total - prev.total;
    uint64_t idle_diff = curr.idle - prev.idle;

    if (total_diff == 0) {
        return 0.0f;
    }

    return 100.0f * (1.0f - static_cast<float>(idle_diff) / static_cast<float>(total_diff));
}

#define MODULENAME CpuModule
#include <wbcffi.txt>
#undef MODULENAME

} // namespace waybar::cffi::cpu
