#ifndef WAYBAR_CFFI_NETWORK_MODULE_HPP
#define WAYBAR_CFFI_NETWORK_MODULE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <module_base.hpp>
#include <vector>
#include <map>

namespace waybar::cffi::network {

// 网络接口信息结构体
struct NetworkInterface {
    std::string name;  // 接口名称
    std::string ip;    // IP地址
    std::string ipv6;  // IPv6地址
    bool is_up;        // 接口是否启用
    bool is_wireless;  // 是否为无线接口
    std::string ssid;  // WiFi网络名称（如果适用）
    int quality_link;  // 信号质量百分比（0-100）
    int quality_level; // 信号强度(dBm，通常为负值)
    int quality_noise; // 噪声水平(dBm，通常为负值)
    uint64_t rx_bytes; // 接收字节数
    uint64_t tx_bytes; // 发送字节数
};

// 配置结构体 - 使用int类型的阈值
struct NetworkConfig : public base::ModuleConfigBase<int> {
    using ThresholdType = int;

    // 网络特定配置
    std::string interface;             // 指定监控的网络接口，空字符串表示自动选择
    bool accumulate_bandwidth = false; // 是否累积带宽统计
    int max_bandwidth = 1000;          // 最大带宽（Mbps），用于计算百分比

    NetworkConfig() {
        icons["default"] = "󰈀";
        icons["disconnected"] = "󱞐";
        icons["wired"] = "󰈀";
        icons["wireless"] = "󰖩";

        // 无线连接质量图标（从差到好）
        icons["wireless-1"] = "󰤯";
        icons["wireless-2"] = "󰤟";
        icons["wireless-3"] = "󰤢";
        icons["wireless-4"] = "󰤥";
        icons["wireless-5"] = "󰤨";

        formats["default"] = "{icon}\u2004{bandwidthRx:>5}\u2004{bandwidthTx:>5}";
        formats["disconnected"] = "{icon}";
        formats["wired"] = "{icon}\u2004{bandwidthRx:>5}\u2004{bandwidthTx:>5}";
        formats["wireless"] = "{icon}\u2004{bandwidthRx:>5}\u2004{bandwidthTx:>5}";

        // 无线信号强度阈值（从差到好）
        // 信号强度范围0-100，分为5个级别
        states["wireless-1"] = 20;  // 0-20%
        states["wireless-2"] = 40;  // 21-40%
        states["wireless-3"] = 60;  // 41-60%
        states["wireless-4"] = 80;  // 61-80%
        states["wireless-5"] = 100; // 81-100%

        format_tooltip = "Interface: {ifname}\nIP: {ipaddr}\nIPv6: {ipv6}\n"
                         "RX Total: {bandwidthRxTot}\nTX Total: {bandwidthTxTot}\n"
                         "RX Rate: {bandwidthRx}\nTX Rate: {bandwidthTx}\n"
                         "Net Speed: {netspeed}";

        // 默认鼠标事件动作
        actions["on-middle-click"] = "LANG=en_US.UTF-8 iwmenu -l rofi";
    }

    // 重写解析配置方法，添加网络特定配置
    void parse_config(const wbcffi_config_entry *entries, size_t count) override;
};

// 网络模块类
class NetworkModule : public base::ModuleBase<NetworkConfig> {
  public:
    NetworkModule(
        const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
    );
    ~NetworkModule() = default;

    // 禁止拷贝和移动
    NetworkModule(const NetworkModule &) = delete;
    NetworkModule &operator=(const NetworkModule &) = delete;
    NetworkModule(NetworkModule &&) = delete;
    NetworkModule &operator=(NetworkModule &&) = delete;

    // 更新函数
    void update();

  private:
    // 网络接口信息
    std::map<std::string, NetworkInterface> interfaces_;
    std::string selected_interface_;

    // 带宽计算
    uint64_t last_rx_bytes_ = 0;
    uint64_t last_tx_bytes_ = 0;
    uint64_t last_update_time_ = 0;

    // 网络信息获取方法
    void scan_network_interfaces();
    std::string get_ip_address(const std::string &interface, bool ipv6 = false);
    std::string get_wifi_ssid(const std::string &interface);
    void get_wifi_info(NetworkInterface &interface);
    void select_best_interface();
    bool is_wireless_interface(const std::string &ifname);
    void determine_interface_type(NetworkInterface &iface);

    // 流量数据格式化方法（参考原始Waybar的pow_format5w）
    std::string pow_format5w(uint64_t bytes) const;
};

} // namespace waybar::cffi::network

#endif // WAYBAR_CFFI_NETWORK_MODULE_HPP