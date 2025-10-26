#include <modules/network_module.hpp>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <chrono>
#include <regex>
#include <common.hpp>

namespace waybar::cffi::network {

// NetworkConfig实现
void NetworkConfig::parse_config(const wbcffi_config_entry *entries, size_t count) {
    // 先调用基类方法
    base::ModuleConfigBase<int>::parse_config(entries, count);

    // 解析网络特定配置
    interface = common::get_config_value<std::string>(config_map, "interface", interface);
    accumulate_bandwidth = common::get_config_value<bool>(config_map, "accumulate-bandwidth", accumulate_bandwidth);
    max_bandwidth = common::get_config_value<int>(config_map, "max-bandwidth", max_bandwidth);
}

// NetworkModule实现
NetworkModule::NetworkModule(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : base::ModuleBase<NetworkConfig>(init_info, config_entries, config_entries_len) {

    // 初始更新
    update();
}

void NetworkModule::update() {
    // 扫描所有网络接口
    scan_network_interfaces();

    // 选择最佳接口
    select_best_interface();

    // 如果没有找到接口，显示断开连接状态
    if (selected_interface_.empty() || interfaces_.find(selected_interface_) == interfaces_.end()) {
        // 使用断开连接的格式
        const std::string &icon = get_icon_for_state_name("disconnected");
        const std::string &format = get_format_for_state_name("disconnected");

        std::string display_text = common::safe_execute<std::string>(
            [&]() {
                std::vector<std::pair<std::string, common::format_arg>> args = {{"icon", icon}, {"ifname", "None"}};
                return common::format_string(format, args);
            },
            icon + " None", "Error formatting disconnected output"
        );

        gtk_label_set_text(GTK_LABEL(label_), display_text.c_str());

        // 设置tooltip
        if (config().tooltip) {
            gtk_widget_set_tooltip_text(event_box_, "No network interface available");
        } else {
            gtk_widget_set_has_tooltip(event_box_, FALSE);
        }

        return;
    }

    // 获取选定的接口信息
    const NetworkInterface &iface = interfaces_.at(selected_interface_);

    // 临时计算速率
    uint64_t rx_rate = 0;
    uint64_t tx_rate = 0;

    // 获取当前时间
    auto now = std::chrono::steady_clock::now();
    uint64_t current_time = uint64_t(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

    // 如果不是第一次更新，计算速率
    if (last_update_time_ > 0 && current_time > last_update_time_) {
        uint64_t time_diff = current_time - last_update_time_;
        if (time_diff > 0) {
            uint64_t rx_diff = iface.rx_bytes - last_rx_bytes_;
            uint64_t tx_diff = iface.tx_bytes - last_tx_bytes_;
            rx_rate = rx_diff / time_diff;
            tx_rate = tx_diff / time_diff;
        }
    }

    // 更新上次记录的值
    last_rx_bytes_ = iface.rx_bytes;
    last_tx_bytes_ = iface.tx_bytes;
    last_update_time_ = current_time;

    // 根据接口状态确定状态名称、图标和显示格式
    std::string state_name;
    std::string icon;
    std::string format;

    if (!iface.is_up || iface.ip.empty()) {
        state_name = "disconnected";
        icon = get_icon_for_state_name("disconnected");
        format = get_format_for_state_name("disconnected");
    } else if (iface.is_wireless) {
        state_name = get_state(iface.quality_link, true);
        icon = get_icon_for_state_name(state_name);
        format = get_format_for_state_name("wireless");
    } else {
        // 有线连接
        state_name = "wired";
        icon = get_icon_for_state_name("wired");
        format = get_format_for_state_name("wired");
    }

    // 准备格式化参数
    std::vector<std::pair<std::string, common::format_arg>> format_args = {
        {"icon", icon},
        {"ifname", iface.name},
        {"ipaddr", iface.ip},
        {"ipv6", iface.ipv6},
        {"essid", iface.ssid},
        {"quality_level", iface.quality_level},
        {"quality_link", iface.quality_link},
        {"quality_noise", iface.quality_noise},
        {"bandwidthRxTot", pow_format5w(iface.rx_bytes)},
        {"bandwidthTxTot", pow_format5w(iface.tx_bytes)},
        {"bandwidthRx", pow_format5w(rx_rate)},
        {"bandwidthTx", pow_format5w(tx_rate)},
        {"netcidr", iface.ip.empty() ? "" : iface.ip + "/24"}, // 简化实现
        {"netspeed", pow_format5w(rx_rate + tx_rate)}
    };

    // 使用公共工具格式化输出
    std::string display_text = common::safe_execute<std::string>(
        [&]() { return common::format_string(format, format_args); }, icon + " " + iface.name, "Error formatting output"
    );

    // 更新标签
    gtk_label_set_text(GTK_LABEL(label_), display_text.c_str());

    // 设置tooltip
    if (config().tooltip) {
        auto tooltip_format = get_tooltip_format();
        std::string tooltip = common::safe_execute<std::string>(
            [&]() { return common::format_string(tooltip_format, format_args); }, "Network: " + iface.name,
            "Error formatting tooltip"
        );

        gtk_widget_set_tooltip_text(event_box_, tooltip.c_str());
    } else {
        gtk_widget_set_has_tooltip(event_box_, FALSE);
    }
}

void NetworkModule::scan_network_interfaces() {
    interfaces_.clear();

    // 使用ifaddrs获取网络接口列表
    struct ifaddrs *ifaddrs_ptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        common::log_error("Failed to get network interfaces");
        return;
    }

    // 遍历所有接口
    for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == nullptr) {
            continue;
        }

        std::string ifname = ifa->ifa_name;

        // 跳过回环接口和非活动接口
        if (ifname == "lo" || !(ifa->ifa_flags & IFF_UP)) {
            continue;
        }

        // 如果接口不存在，创建它
        if (interfaces_.find(ifname) == interfaces_.end()) {
            NetworkInterface iface;
            iface.name = ifname;
            iface.is_up = (ifa->ifa_flags & IFF_UP) != 0;
            iface.is_wireless = false; // 默认为有线，后面会检查
            iface.quality_level = 0;
            iface.rx_bytes = 0;
            iface.tx_bytes = 0;
            interfaces_[ifname] = iface;
        }

        // 获取IP地址
        if (ifa->ifa_addr != nullptr) {
            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
                char addr_str[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(addr_in->sin_addr), addr_str, INET_ADDRSTRLEN) != nullptr) {
                    interfaces_[ifname].ip = addr_str;
                }
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                char addr_str[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, &(addr_in6->sin6_addr), addr_str, INET6_ADDRSTRLEN) != nullptr) {
                    // 跳过本地链路地址
                    if (std::string(addr_str).find("fe80") != 0) {
                        interfaces_[ifname].ipv6 = addr_str;
                    }
                }
            }
        }
    }

    freeifaddrs(ifaddrs_ptr);

    // 检查哪些接口是无线接口并获取WiFi信息
    for (auto &pair : interfaces_) {
        std::string ifname = pair.first;
        NetworkInterface &iface = pair.second;

        // 使用ioctl更准确地判断接口类型
        if (ifname.empty()) {
            iface.is_wireless = false;
            continue;
        }

        // 使用新的函数来确定接口类型
        determine_interface_type(iface);
        if (!iface.is_up) {
            continue;
        }

        // 获取网络统计信息 - 使用lambda表达式
        auto get_interface_stat = [](const std::string &interface, const std::string &stat) -> uint64_t {
            std::string path = "/sys/class/net/" + interface + "/statistics/" + stat;
            std::ifstream file(path);
            if (!file.is_open()) {
                return 0;
            }

            uint64_t value;
            file >> value;
            return value;
        };

        iface.rx_bytes = get_interface_stat(ifname, "rx_bytes");
        iface.tx_bytes = get_interface_stat(ifname, "tx_bytes");
    }
}

std::string NetworkModule::get_ip_address(const std::string &interface, bool ipv6) {
    struct ifaddrs *ifaddrs_ptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        return "";
    }

    std::string result;

    for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == nullptr || std::string(ifa->ifa_name) != interface) {
            continue;
        }

        if (ifa->ifa_addr != nullptr) {
            if (!ipv6 && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
                char addr_str[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(addr_in->sin_addr), addr_str, INET_ADDRSTRLEN) != nullptr) {
                    result = addr_str;
                    break;
                }
            } else if (ipv6 && ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                char addr_str[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, &(addr_in6->sin6_addr), addr_str, INET6_ADDRSTRLEN) != nullptr) {
                    // 跳过本地链路地址
                    if (std::string(addr_str).find("fe80") != 0) {
                        result = addr_str;
                        break;
                    }
                }
            }
        }
    }

    freeifaddrs(ifaddrs_ptr);
    return result;
}

std::string NetworkModule::get_wifi_ssid(const std::string &interface) {
    // 使用ioctl获取SSID，避免依赖外部命令
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return "";
    }

    // 准备ioctl请求
    struct iwreq wreq;
    memset(&wreq, 0, sizeof(wreq));
    strncpy(wreq.ifr_name, interface.c_str(), IFNAMSIZ - 1);

    // 准备缓冲区来存储ESSID
    char essid[IW_ESSID_MAX_SIZE + 1] = {0};
    wreq.u.essid.pointer = essid;
    wreq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    wreq.u.essid.flags = 0;

    // 执行ioctl获取ESSID
    if (ioctl(sockfd, SIOCGIWESSID, &wreq) < 0) {
        close(sockfd);
        return "";
    }

    close(sockfd);

    // 确保字符串以null结尾
    essid[wreq.u.essid.length] = '\0';

    return std::string(essid);
}

void NetworkModule::get_wifi_info(NetworkInterface &iface) {
    // 初始化为0
    iface.quality_level = 0;
    iface.quality_link = 0;
    iface.quality_noise = 0;

    // 使用ioctl获取无线统计信息，避免解析/proc/net/wireless
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return;
    }

    // 准备ioctl请求
    struct iwreq wreq;
    memset(&wreq, 0, sizeof(wreq));
    strncpy(wreq.ifr_name, iface.name.c_str(), IFNAMSIZ - 1);

    // 获取无线统计信息
    struct iw_statistics stats;
    wreq.u.data.pointer = (caddr_t)&stats;
    wreq.u.data.length = sizeof(stats);
    wreq.u.data.flags = 1; // 更新统计信息

    if (ioctl(sockfd, SIOCGIWSTATS, &wreq) < 0) {
        close(sockfd);
        return;
    }

    close(sockfd);

    // 解析统计信息
    // 注意：不同的驱动可能使用不同的字段，这里使用常见的映射
    if (stats.qual.updated & IW_QUAL_QUAL_UPDATED) {
        // quality_link是信号质量百分比(0-100)
        iface.quality_link = stats.qual.qual;
    }

    if (stats.qual.updated & IW_QUAL_LEVEL_UPDATED) {
        // quality_level是信号强度(dBm)，通常是负值
        if (stats.qual.level > 0) {
            // 一些驱动可能直接提供dBm值
            iface.quality_level = stats.qual.level;
        } else {
            // 无线扩展中负值存储为256-值，转换为真实的dBm值
            iface.quality_level = stats.qual.level - 256;
        }
    }

    if (stats.qual.updated & IW_QUAL_NOISE_UPDATED) {
        // quality_noise是噪声水平(dBm)，通常是负值
        if (stats.qual.noise > 0) {
            // 一些驱动可能直接提供dBm值
            iface.quality_noise = stats.qual.noise;
        } else {
            // 无线扩展中负值存储为256-值，转换为真实的dBm值
            iface.quality_noise = stats.qual.noise - 256;
        }
    }
}

void NetworkModule::select_best_interface() {
    // 如果用户指定了接口，尝试使用它
    if (!config().interface.empty()) {
        if (interfaces_.find(config().interface) != interfaces_.end()) {
            selected_interface_ = config().interface;
            return;
        }
        common::log_warning("Configured interface '{}' not found, auto-selecting", config().interface);
    }

    // 否则，自动选择最佳接口
    // 优先级：有线连接 > 无线连接 > 其他
    std::string wired_interface;
    std::string wireless_interface;

    for (const auto &pair : interfaces_) {
        const NetworkInterface &iface = pair.second;
        if (!iface.is_up || iface.ip.empty()) {
            continue;
        }

        // 只考虑以e或w开头的有效接口
        if (iface.name.empty()) {
            continue;
        }

        char first_char = iface.name[0];
        if (first_char != 'e' && first_char != 'w') {
            continue;
        }

        if (iface.is_wireless && wireless_interface.empty()) {
            wireless_interface = iface.name;
        } else if (!iface.is_wireless && wired_interface.empty()) {
            wired_interface = iface.name;
        }
    }

    // 选择最佳接口
    if (!wired_interface.empty()) {
        selected_interface_ = wired_interface;
    } else if (!wireless_interface.empty()) {
        selected_interface_ = wireless_interface;
    } else {
        selected_interface_ = "";
    }
}

bool NetworkModule::is_wireless_interface(const std::string &ifname) {
    // 使用ioctl更准确地判断是否为无线接口，而不是仅依赖名称前缀
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        // 回退到简单的名称判断
        return !ifname.empty() && ifname[0] == 'w';
    }

    struct iwreq wreq;
    memset(&wreq, 0, sizeof(wreq));
    strncpy(wreq.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

    bool is_wireless = (ioctl(sockfd, SIOCGIWNAME, &wreq) >= 0);
    close(sockfd);

    return is_wireless;
}

void NetworkModule::determine_interface_type(NetworkInterface &iface) {
    if (iface.name.empty()) {
        iface.is_wireless = false;
        return;
    }

    if (is_wireless_interface(iface.name)) {
        // 无线接口
        iface.is_wireless = true;
        iface.ssid = get_wifi_ssid(iface.name);
        get_wifi_info(iface);
    } else {
        // 检查是否为以太网接口（以'e'开头）
        char first_char = iface.name[0];
        if (first_char == 'e') {
            // 以太网接口
            iface.is_wireless = false;
        } else {
            // 其他类型的接口，标记为无效，将在后续处理中被剔除
            iface.is_up = false;
        }
    }
}

// 参考原始Waybar的pow_format5w实现 - 5字符宽度的格式化
std::string NetworkModule::pow_format5w(uint64_t bytes) const {
    const char *units = "KMGTPE";
    int unit_idx = -1;
    auto size = static_cast<double>(bytes);
    auto base = 1000.0; // 使用1000作为基数，与原始实现一致

    if (size < 10.0) {
        return "0.00K";
    }

    while (size >= base && unit_idx < 5) {
        size /= base;
        unit_idx++;
    }

    if (unit_idx < 0) {
        unit_idx = 0;
        size /= base;
    }

    // 使用common命名空间的format_number函数格式化数值部分，固定4字符宽度
    std::string number_part = common::format_number(size, 4);
    std::string unit_part = std::string(1, units[unit_idx]);
    std::string formatted = number_part + unit_part;

    return formatted;
}

#define MODULENAME NetworkModule
#include <wbcffi.txt>
#undef MODULENAME

} // namespace waybar::cffi::network
