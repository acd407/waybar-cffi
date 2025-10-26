#include <common.hpp>
#include <modules/cpu_module.hpp>
#include <functional>
#include <cctype>
#include <fmt/format.h>
#include <fmt/args.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace waybar::cffi::common {

// 混合类型版本的format_string实现
std::string format_string(const std::string &format_str, const std::vector<std::pair<std::string, format_arg>> &args) {
    try {
        // 使用fmt库的动态参数存储
        fmt::dynamic_format_arg_store<fmt::format_context> store;

        // 将所有参数添加到存储中
        for (const auto &[key, value] : args) {
            std::visit([&](auto &&arg) { store.push_back(fmt::arg(key.c_str(), arg)); }, value);
        }

        // 使用vformat进行格式化
        return fmt::vformat(format_str, store);
    } catch (const std::exception &e) {
        // 发生错误时，解析并输出args参数以便调试
        log_info("=== format_string 错误调试信息 ===");
        log_info("错误信息: {}", e.what());
        log_info("格式字符串: {}", format_str);
        log_info("参数数量: {}", args.size());

        // 遍历所有参数
        for (size_t i = 0; i < args.size(); ++i) {
            const auto &[key, value] = args[i];
            log_info("参数[{}]: 键 = '{}'", i, key);

            // 使用std::visit访问variant中的值
            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, int>) {
                        log_info("  值类型: int, 值 = {}", arg);
                    } else if constexpr (std::is_same_v<T, double>) {
                        log_info("  值类型: double, 值 = {}", arg);
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        log_info("  值类型: string, 值 = '{}'", arg);
                    }
                },
                value
            );
        }
        log_info("=== 错误调试信息结束 ===");

        log_error("Error in format_string (mixed type version): {}", e.what());
        return format_str; // 返回原始格式字符串
    }
}

// 格式化数字，确保总长度为指定字符数
std::string format_number(double value, int total_length) {
    std::ostringstream oss;

    // 根据值的大小和所需长度动态选择精度
    if (value >= 100.0) {
        oss << std::fixed << std::setprecision(0) << std::round(value);
    } else if (value >= 10.0) {
        oss << std::fixed << std::setprecision(1) << value;
    } else {
        oss << std::fixed << std::setprecision(2) << value;
    }

    std::string result = oss.str();

    // 强制截断或填充到指定长度（在前面填充空格，保持右对齐）
    if (result.length() > static_cast<size_t>(total_length)) {
        result = result.substr(0, static_cast<size_t>(total_length));
    } else if (result.length() < static_cast<size_t>(total_length)) {
        size_t padding = static_cast<size_t>(total_length) - result.length();
        result = std::string(padding, ' ') + result;
    }

    return result;
}

// 辅助函数：将Unicode码点写入输出流
static void write_unicode_code_point(std::stringstream &output, unsigned long code_point) {
    if (code_point <= 0x7F) {
        output << static_cast<char>(code_point);
    } else if (code_point <= 0x7FF) {
        output << static_cast<char>(0xC0 | ((code_point >> 6) & 0x1F));
        output << static_cast<char>(0x80 | (code_point & 0x3F));
    } else if (code_point <= 0xFFFF) {
        output << static_cast<char>(0xE0 | ((code_point >> 12) & 0x0F));
        output << static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        output << static_cast<char>(0x80 | (code_point & 0x3F));
    } else if (code_point <= 0x10FFFF) {
        output << static_cast<char>(0xF0 | ((code_point >> 18) & 0x07));
        output << static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
        output << static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        output << static_cast<char>(0x80 | (code_point & 0x3F));
    } else {
        throw std::invalid_argument("Invalid Unicode code point");
    }
}

static std::string waybar_parse_escape_sequences(const std::string_view &input) {
    std::stringstream output;
    bool in_escape = false;

    // 安全地处理引号包围的字符串
    size_t start = 0, end = input.size();
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        start = 1;
        end = input.size() - 1;
    }

    for (size_t i = start; i < end; ++i) {
        char c = input[i];

        if (!in_escape) {
            if (c == '\\') {
                in_escape = true;
            } else {
                output << c;
            }
            continue;
        }

        // We're in an escape sequence
        switch (c) {
        case '\\':
        case '\'':
        case '\"':
            output << c;
            break;
        case 'a':
        case 'b':
        case 'f':
            output << c - 90;
            break;
        case 'n':
            output << '\n';
            break; // New line
        case 'r':
            output << '\r';
            break; // Carriage return
        case 't':
            output << '\t';
            break; // Horizontal tab
        case 'v':
            output << '\v';
            break; // Vertical tab
        case '0':
            output << '\0';
            break; // Null character

        case 'u':
        case 'U': { // Unicode escape (4 or 8 hex digits)
            size_t hex_len = (c == 'u') ? 4 : 8;
            if (i + hex_len >= end) {
                throw std::invalid_argument("Incomplete Unicode escape sequence");
            }

            std::string_view hex_str = input.substr(i + 1, hex_len);
            i += hex_len;

            try {
                // 手动解析十六进制，避免string_view到string的转换
                unsigned long code_point = 0;
                for (char c : hex_str) {
                    code_point <<= 4;
                    if (c >= '0' && c <= '9') {
                        code_point += static_cast<unsigned long>(c - '0');
                    } else if (c >= 'a' && c <= 'f') {
                        code_point += static_cast<unsigned long>(10 + (c - 'a'));
                    } else if (c >= 'A' && c <= 'F') {
                        code_point += static_cast<unsigned long>(10 + (c - 'A'));
                    } else {
                        throw std::invalid_argument("Invalid hex digit");
                    }
                }
                write_unicode_code_point(output, code_point);
            } catch (const std::exception &e) {
                throw std::invalid_argument("Invalid Unicode escape sequence");
            }
            break;
        }

        case 'x': { // Hexadecimal escape (1-2 hex digits)
            if (i + 1 >= end) {
                throw std::invalid_argument("Incomplete hexadecimal escape sequence");
            }

            size_t hex_digits = 0;
            std::string hex_str;

            while (i + 1 + hex_digits < end && hex_digits < 2 && isxdigit(input[i + 1 + hex_digits])) {
                hex_digits++;
            }

            if (hex_digits == 0) {
                throw std::invalid_argument("Invalid hexadecimal escape sequence");
            }

            hex_str = input.substr(i + 1, hex_digits);
            i += hex_digits;

            try {
                unsigned long value = std::stoul(hex_str, nullptr, 16);
                output << static_cast<char>(value);
            } catch (const std::exception &e) {
                throw std::invalid_argument("Invalid hexadecimal escape sequence");
            }
            break;
        }

        default: {
            // Unknown escape sequence - just output as literal
            output << '\\' << c;
            break;
        }
        }

        in_escape = false;
    }

    return output.str();
}

// 清理字符串值，去除换行符并处理转义序列
std::string clean_string_value(const std::string &value) {
    if (value.empty()) {
        return value;
    }

    std::string result = value;

    // 去除末尾的换行符
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    // 使用waybar_parse_escape_sequences处理引号和转义序列
    try {
        result = waybar_parse_escape_sequences(result);
    } catch (const std::exception &e) {
        log_warning("Error parsing escape sequences: {}", e.what());
    }

    return result;
}

} // namespace waybar::cffi::common
