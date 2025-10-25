#ifndef WAYBAR_CFFI_COMMON_HPP
#define WAYBAR_CFFI_COMMON_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <format>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdarg>
#include <functional>
#include <cmath>
#include <variant>
#include <type_traits>
#include <fmt/format.h>
#include <fmt/args.h>

// 前向声明配置条目结构
struct wbcffi_config_entry;

namespace waybar::cffi::common {

// 日志记录函数
void log_error(const std::string &message);
void log_warning(const std::string &message);
void log_info(const std::string &message);

// 清理字符串值，去除引号和换行符，并处理转义序列
std::string clean_string_value(const std::string &value);

// 使用std::variant支持混合类型的参数值
using format_arg = std::variant<int, double, std::string>;

// 混合类型版本：支持不同类型的参数值
// 例如: format_string("Power: {value:.2f}W, Count: {count:>3}", {{"value", 12.3456}, {"count", 42}})
std::string format_string(const std::string &format_str, const std::vector<std::pair<std::string, format_arg>> &args);

// 泛型版本：支持相同类型的参数值（保留向后兼容性）
// 例如: format_string("Power: {value:.2f}W", {{"value", 12.3456}}) -> "Power: 12.35W"
template <typename T>
std::string format_string(const std::string &format_str, const std::vector<std::pair<std::string, T>> &args) {
    try {
        // 使用fmt库的动态参数存储
        fmt::dynamic_format_arg_store<fmt::format_context> store;

        // 将所有参数添加到存储中
        for (const auto &[name, value] : args) {
            // 直接添加值，让fmt库处理类型
            store.push_back(fmt::arg(name.c_str(), value));
        }

        // 使用vformat进行格式化
        return fmt::vformat(format_str, store);
    } catch (const std::exception &e) {
        log_error("Error in format_string (generic version): " + std::string(e.what()));
        return format_str; // 返回原始格式字符串
    }
}

// 格式化数字，确保总长度为指定字符数（默认4字符）
// 例如: format_number(75.5) -> "75.5", format_number(5.25) -> "5.25", format_number(100.0) -> "100"
std::string format_number(double value, int total_length = 4);

// 安全地获取配置值，如果不存在则返回默认值
template <typename T>
T get_config_value(
    const std::unordered_map<std::string, std::string> &config, const std::string &key, const T &default_value
) {
    auto it = config.find(key);
    if (it == config.end()) {
        return default_value;
    }

    const std::string value = clean_string_value(it->second);

    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, int>) {
        try {
            return std::stoi(value);
        } catch (const std::exception &) {
            log_warning("Invalid integer value for config key: " + key);
            return default_value;
        }
    } else if constexpr (std::is_same_v<T, double>) {
        try {
            return std::stod(value);
        } catch (const std::exception &) {
            log_warning("Invalid double value for config key: " + key);
            return default_value;
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        // 转换为小写以便比较
        std::string lower_value = value;
        std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

        if (lower_value == "true" || lower_value == "1" || lower_value == "yes" || lower_value == "on") {
            return true;
        } else if (lower_value == "false" || lower_value == "0" || lower_value == "no" || lower_value == "off") {
            return false;
        } else {
            log_warning("Invalid boolean value for config key: " + key);
            return default_value;
        }
    } else {
        // 对于不支持的类型，返回默认值
        static_assert(!sizeof(T), "Unsupported type for get_config_value");
        return default_value;
    }
}

// 错误处理辅助函数
template <typename T>
T safe_execute(std::function<T()> func, const T &default_value, const std::string &error_context = "") {
    try {
        return func();
    } catch (const std::exception &e) {
        log_error(error_context + ": " + std::string(e.what()));
        return default_value;
    } catch (...) {
        log_error(error_context + ": Unknown error");
        return default_value;
    }
}

} // namespace waybar::cffi::common

#endif // WAYBAR_CFFI_COMMON_HPP