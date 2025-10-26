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
#include <chrono>

// 前向声明配置条目结构
struct wbcffi_config_entry;

namespace waybar::cffi::common {

// 日志记录函数 - 使用fmt库风格的格式化
template <typename... Args> void log_error(fmt::format_string<Args...> fmt, Args &&...args) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    // 使用fmt库格式化消息
    std::string message = fmt::vformat(fmt, fmt::make_format_args(args...));

    // 使用ANSI颜色代码：红色表示错误
    fprintf(stderr, "[%s] [\033[0;31merror\033[0m] %s\n", oss.str().c_str(), message.c_str());
}

template <typename... Args> void log_warning(fmt::format_string<Args...> fmt, Args &&...args) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    // 使用fmt库格式化消息
    std::string message = fmt::vformat(fmt, fmt::make_format_args(args...));

    // 使用ANSI颜色代码：黄色表示警告
    fprintf(stderr, "[%s] [\033[0;33mwarning\033[0m] %s\n", oss.str().c_str(), message.c_str());
}

template <typename... Args> void log_info(fmt::format_string<Args...> fmt, Args &&...args) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    // 使用fmt库格式化消息
    std::string message = fmt::vformat(fmt, fmt::make_format_args(args...));

    // 使用ANSI颜色代码：绿色表示信息
    fprintf(stdout, "[%s] [\033[0;32minfo\033[0m] %s\n", oss.str().c_str(), message.c_str());
}

// 清理字符串值，去除引号和换行符，并处理转义序列
std::string clean_string_value(const std::string &value);

// 使用std::variant支持混合类型的参数值
using format_arg = std::variant<int, double, std::string>;

// 混合类型版本：支持不同类型的参数值
// 例如: format_string("Power: {value:.2f}W, Count: {count:>3}", {{"value", 12.3456}, {"count", 42}})
std::string format_string(const std::string &format_str, const std::vector<std::pair<std::string, format_arg>> &args);

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
            log_warning("Invalid integer value for config key: {}", key);
            return default_value;
        }
    } else if constexpr (std::is_same_v<T, double>) {
        try {
            return std::stod(value);
        } catch (const std::exception &) {
            log_warning("Invalid double value for config key: {}", key);
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
            log_warning("Invalid boolean value for config key: {}", key);
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
        log_error("{}: {}", error_context, e.what());
        return default_value;
    } catch (...) {
        log_error("{}: Unknown error", error_context);
        return default_value;
    }
}

} // namespace waybar::cffi::common

#endif // WAYBAR_CFFI_COMMON_HPP
