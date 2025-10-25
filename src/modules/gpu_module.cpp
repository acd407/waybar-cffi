#include <modules/gpu_module.hpp>
#include <fstream>
#include <common.hpp>
#include <filesystem>

namespace waybar::cffi::gpu {

// GpuModule实现
GpuModule::GpuModule(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : base::ModuleBase<GpuConfig>(init_info, config_entries, config_entries_len) {

    // 标记此模块处理按钮点击事件
    handles_button_press_ = true;

    update();
}

void GpuModule::update() {
    try {
        // 获取GPU使用率和VRAM使用量
        int gpu_usage = get_gpu_usage();
        double vram_used = get_vram_used();

        // 确定当前使用的格式
        const std::string &format_key = current_format_key_.empty() ? "default" : current_format_key_;
        std::string format_str = get_format_for_state_name(format_key);

        // 获取状态对应的图标
        std::string state_name = get_state(gpu_usage);
        const std::string &icon = get_icon_for_state_name(state_name);

        // 使用混合类型的format_string函数，支持格式说明符
        std::vector<std::pair<std::string, waybar::cffi::common::format_arg>> args = {
            {"icon", icon}, {"gpu_usage", gpu_usage}, {"vram_used", vram_used}
        };

        std::string text = common::format_string(format_str, args);
        gtk_label_set_text(GTK_LABEL(label_), text.c_str());

        // 设置tooltip
        std::string tooltip = common::format_string(config().format_tooltip, args);
        gtk_widget_set_tooltip_text(event_box_, tooltip.c_str());

    } catch (const std::exception &e) {
        common::log_error("Error updating GPU module: " + std::string(e.what()));
        gtk_label_set_text(GTK_LABEL(label_), "Error");
    }
}

int GpuModule::get_gpu_usage() const {
    return common::safe_execute<int>(
        [&]() {
            std::ifstream file(config().gpu_usage_path);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open GPU usage file: " + config().gpu_usage_path);
            }

            std::string line;
            if (!std::getline(file, line)) {
                throw std::runtime_error("Failed to read GPU usage from: " + config().gpu_usage_path);
            }

            // 去除空白字符
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

            try {
                return std::stoi(line);
            } catch (const std::exception &e) {
                throw std::runtime_error("Failed to parse GPU usage value: " + line);
            }
        },
        0, "Error reading GPU usage"
    );
}

double GpuModule::get_vram_used() const {
    return common::safe_execute<double>(
        [&]() {
            std::ifstream file(config().vram_used_path);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open VRAM usage file: " + config().vram_used_path);
            }

            std::string line;
            if (!std::getline(file, line)) {
                throw std::runtime_error("Failed to read VRAM usage from: " + config().vram_used_path);
            }

            try {
                // VRAM值通常以字节为单位，转换为GB
                unsigned long vram_bytes = std::stoul(line);
                return double(vram_bytes) / (1024.0 * 1024.0 * 1024.0);
            } catch (const std::exception &e) {
                throw std::runtime_error("Failed to parse VRAM usage value: " + line);
            }
        },
        0.0, "Error reading VRAM usage"
    );
}

gboolean GpuModule::handle_button_press(GdkEventButton *event) {
    // 只处理左键点击事件
    if (event->button == GDK_BUTTON_PRIMARY) {
        // 切换格式键
        if (current_format_key_ == "default") {
            current_format_key_ = "alt";
        } else {
            current_format_key_ = "default";
        }

        // 立即更新显示
        update();

        common::log_info("GPU module format switched to: " + current_format_key_);

        // 返回TRUE表示我们已经处理了点击事件
        return TRUE;
    }

    // 对于非左键点击，返回FALSE表示未处理，允许事件继续传播
    return FALSE;
}

#define MODULENAME GpuModule
#include <wbcffi.txt>
#undef MODULENAME

} // namespace waybar::cffi::gpu