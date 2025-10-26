#ifndef WAYBAR_CFFI_MODULE_BASE_HPP
#define WAYBAR_CFFI_MODULE_BASE_HPP

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <cstdlib>
#include <common.hpp>
#include <concepts>

// 如果系统安装了nlohmann/json，使用系统版本
#ifdef __has_include
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#define HAS_NLOHMANN_JSON 1
#endif
#endif

// 如果没有系统版本，使用内置版本
#ifndef HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

extern "C" {
// 前向声明
typedef struct wbcffi_module wbcffi_module;

// 初始化信息结构体
typedef struct {
    wbcffi_module *obj;
    const char *waybar_version;
    GtkContainer *(*get_root_widget)(wbcffi_module *);
    void (*queue_update)(wbcffi_module *);
} wbcffi_init_info;

// 配置项结构体
struct wbcffi_config_entry {
    const char *key;
    const char *value;
};

extern const size_t wbcffi_version;

// 必须导出的函数
void *wbcffi_init(
    const wbcffi_init_info *init_info, const struct wbcffi_config_entry *config_entries, size_t config_entries_len
);

void wbcffi_deinit(void *instance);

// 可选导出的函数
void wbcffi_update(void *instance);
void wbcffi_refresh(void *instance, int signal);

// 获取GTK组件
GtkWidget *wbcffi_get_widget(void *instance);
}

namespace waybar::cffi::base {

// 通用状态枚举
enum class ModuleState { DEFAULT, WARNING, CRITICAL };

// 通用配置基类 - 使用模板参数支持不同类型的阈值
template <typename ThresholdType = int>
    requires std::integral<ThresholdType> || std::floating_point<ThresholdType>
struct ModuleConfigBase {
    std::unordered_map<std::string, std::string> config_map;

    int interval = 1;
    bool tooltip = true; // 默认启用tooltip
    std::string format_tooltip;
    std::unordered_map<std::string, std::string> icons;
    std::unordered_map<std::string, std::string> formats;
    std::unordered_map<std::string, ThresholdType> states; // 存储状态名称和阈值

    // 鼠标事件动作配置
    std::unordered_map<std::string, std::string> actions; // 存储鼠标事件对应的动作

    // 构造函数，初始化默认状态和格式
    ModuleConfigBase() {
        states["warning"] = static_cast<ThresholdType>(20);
        states["critical"] = static_cast<ThresholdType>(50);
    }

    // 从配置条目解析配置
    virtual void parse_config(const wbcffi_config_entry *entries, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            config_map[entries[i].key] = common::clean_string_value(entries[i].value);
        }

        // 使用公共工具加载配置
        tooltip = common::get_config_value<bool>(config_map, "tooltip", tooltip);
        interval = common::get_config_value<int>(config_map, "interval", interval);
        format_tooltip = common::get_config_value<std::string>(config_map, "format-tooltip", format_tooltip);

        // 解析格式配置
        auto formats_value = config_map.find("formats");
        if (formats_value != config_map.end()) {
            try {
                nlohmann::json formats = nlohmann::json::parse(formats_value->second);
                for (auto it = formats.begin(); it != formats.end(); ++it) {
                    if (it.value().is_string()) {
                        this->formats[it.key()] = it.value();
                    }
                }
            } catch (const nlohmann::json::exception &e) {
                common::log_error("Failed to parse formats JSON: {}", e.what());
            }
        }

        // 解析图标配置
        auto icons_value = config_map.find("icons");
        if (icons_value != config_map.end()) {
            try {
                nlohmann::json icons = nlohmann::json::parse(icons_value->second);
                for (auto it = icons.begin(); it != icons.end(); ++it) {
                    if (it.value().is_string()) {
                        this->icons[it.key()] = it.value();
                    }
                }
            } catch (const nlohmann::json::exception &e) {
                common::log_error("Failed to parse icons JSON: {}", e.what());
            }
        }

        auto states_value = config_map.find("states");
        if (states_value != config_map.end()) {
            try {
                nlohmann::json states = nlohmann::json::parse(states_value->second);
                for (auto it = states.begin(); it != states.end(); ++it) {
                    if (it.value().is_number()) {
                        this->states[it.key()] = it.value();
                    }
                }
            } catch (const nlohmann::json::exception &e) {
                common::log_error("Failed to parse states JSON: {}", e.what());
            }
        }

        // 解析鼠标事件动作配置
        auto actions_value = config_map.find("actions");
        if (actions_value != config_map.end()) {
            try {
                nlohmann::json actions = nlohmann::json::parse(actions_value->second);
                for (auto it = actions.begin(); it != actions.end(); ++it) {
                    if (it.value().is_string()) {
                        this->actions[it.key()] = it.value();
                    }
                }
            } catch (const nlohmann::json::exception &e) {
                common::log_error("Failed to parse actions JSON: {}", e.what());
            }
        }
    }
};

// 模块基类
template <typename ConfigType> class ModuleBase {
  public:
    ModuleBase(const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len);
    virtual ~ModuleBase();

    // 禁止拷贝和移动
    ModuleBase(const ModuleBase &) = delete;
    ModuleBase &operator=(const ModuleBase &) = delete;
    ModuleBase(ModuleBase &&) = delete;
    ModuleBase &operator=(ModuleBase &&) = delete;

    // 更新函数
    virtual void update() = 0;
    virtual void refresh(int signal);

    // 获取GTK组件（用于C接口）
    GtkWidget *get_widget() const {
        return event_box_;
    }

    // 获取配置
    const ConfigType &config() const {
        return *config_;
    }

  protected:
    // 配置和状态
    std::unique_ptr<ConfigType> config_;
    std::string state_name_ = "default"; // 使用字符串代替ModuleState枚举
    bool first_update_ = true;

    // GTK组件
    GtkWidget *label_ = nullptr;
    GtkWidget *event_box_ = nullptr;

    // Waybar回调
    wbcffi_module *obj_ = nullptr;
    void (*queue_update_)(wbcffi_module *) = nullptr;

    // 定时器ID
    guint timer_id_ = 0;
    bool handles_button_press_ = true; // 标记子类是否重载了handle_button_press
    bool handles_scroll_ = true;       // 标记子类是否重载了handle_scroll

    // 内部方法
    void init_ui(const wbcffi_init_info *init_info);
    void setup_timer();

    // 获取状态对应的图标和格式
    virtual const std::string &get_icon_for_state_name(const std::string &state_name) const;
    virtual const std::string &get_format_for_state_name(const std::string &state_name) const;

    // 获取tooltip格式，如果format-tooltip为空则回退到默认格式
    const std::string &get_tooltip_format() const;

    // 根据值获取状态字符串并设置对应的CSS类 - 模板方法支持不同类型
    template <typename ValueType> std::string get_state(ValueType value, bool lesser = false);

    // 定时器回调
    static gboolean timer_callback(gpointer user_data);

    // 按钮点击回调
    static gboolean button_press_callback(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

    // 虚函数，子类可以重载来实现自定义按钮点击处理
    virtual gboolean handle_button_press(GdkEventButton *event);

    // 执行动作的通用方法
    void execute_action(const std::string &action);

    // 滚轮事件回调
    static gboolean scroll_event_callback(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);

    // 虚函数，子类可以重载来实现自定义滚轮事件处理
    virtual gboolean handle_scroll(GdkEventScroll *event);

    // 窗口创建回调
    static void on_widget_realized(GtkWidget *widget, gpointer user_data);
};

// ModuleBase模板实现
template <typename ConfigType>
ModuleBase<ConfigType>::ModuleBase(
    const wbcffi_init_info *init_info, const wbcffi_config_entry *config_entries, size_t config_entries_len
)
    : handles_button_press_(false), handles_scroll_(false) {
    // 保存Waybar回调
    obj_ = init_info->obj;
    queue_update_ = init_info->queue_update;

    // 初始化配置（子类应该重写此方法来创建特定类型的配置）
    config_ = std::make_unique<ConfigType>();
    config_->parse_config(config_entries, config_entries_len);

    // 初始化UI
    init_ui(init_info);

    // 设置定时器实现自动刷新
    setup_timer();
}

template <typename ConfigType> ModuleBase<ConfigType>::~ModuleBase() {
    // 移除定时器
    if (timer_id_ > 0) {
        g_source_remove(timer_id_);
        timer_id_ = 0;
    }

    // 销毁GTK组件
    if (label_) {
        gtk_widget_destroy(label_);
    }
    if (event_box_) {
        gtk_widget_destroy(event_box_);
    }
}

template <typename ConfigType> void ModuleBase<ConfigType>::init_ui(const wbcffi_init_info *init_info) {
    // 获取根容器
    GtkContainer *root = init_info->get_root_widget(init_info->obj);

    // 创建事件盒
    event_box_ = gtk_event_box_new();
    // 设置事件盒可以接收焦点和事件
    gtk_widget_set_can_focus(event_box_, TRUE);
    gtk_widget_add_events(event_box_, GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_container_add(GTK_CONTAINER(root), event_box_);

    // 创建标签
    label_ = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(event_box_), label_);

    // 设置tooltip查询属性，确保tooltip可以显示
    gtk_widget_set_has_tooltip(event_box_, TRUE);

    // 设置鼠标指针
    GdkWindow *window = gtk_widget_get_window(event_box_);
    if (window) {
        GdkDisplay *display = gdk_window_get_display(window);
        // 根据handles_button_press_标记使用手形或默认指针
        GdkCursorType cursor_type = handles_button_press_ ? GDK_HAND2 : GDK_ARROW;
        GdkCursor *cursor = gdk_cursor_new_for_display(display, cursor_type);
        gdk_window_set_cursor(window, cursor);
        g_object_unref(cursor);
    } else {
        // 如果窗口还未创建，使用信号在窗口创建后设置光标
        g_signal_connect(event_box_, "realize", G_CALLBACK(on_widget_realized), this);
    }

    // 添加按钮点击事件处理器
    g_signal_connect(event_box_, "button-press-event", G_CALLBACK(button_press_callback), this);

    // 添加滚轮事件处理器
    g_signal_connect(event_box_, "scroll-event", G_CALLBACK(scroll_event_callback), this);

    // 显示所有组件
    gtk_widget_show_all(event_box_);
}

template <typename ConfigType> void ModuleBase<ConfigType>::setup_timer() {
    timer_id_ = g_timeout_add_seconds(static_cast<guint>(config_->interval), timer_callback, this);
}

template <typename ConfigType> void ModuleBase<ConfigType>::refresh(int signal) {
    (void)signal;
    // 可以根据信号执行特定操作
    update();
}

template <typename ConfigType>
const std::string &ModuleBase<ConfigType>::get_icon_for_state_name(const std::string &state_name) const {
    // 从icons映射中获取图标，如果不存在则使用默认图标
    auto it = config_->icons.find(state_name);
    if (it != config_->icons.end()) {
        return it->second;
    }

    // 如果找不到对应状态的图标，尝试使用默认图标
    auto default_it = config_->icons.find("default");
    if (default_it != config_->icons.end()) {
        return default_it->second;
    }

    // 最后的备用方案：返回空字符串
    static const std::string empty_icon = "";
    return empty_icon;
}

template <typename ConfigType>
const std::string &ModuleBase<ConfigType>::get_format_for_state_name(const std::string &state_name) const {
    // 从formats映射中获取格式，如果不存在则使用默认格式
    auto it = config_->formats.find(state_name);
    if (it != config_->formats.end()) {
        return it->second;
    }

    // 如果找不到对应状态的格式，尝试使用默认格式
    auto default_it = config_->formats.find("default");
    if (default_it != config_->formats.end()) {
        return default_it->second;
    }

    // 最后的备用方案：返回空字符串
    static const std::string empty_format = "{}";
    return empty_format;
}

template <typename ConfigType> const std::string &ModuleBase<ConfigType>::get_tooltip_format() const {
    // 如果format-tooltip不为空，使用它
    if (!config_->format_tooltip.empty()) {
        return config_->format_tooltip;
    }

    // 否则尝试使用默认格式
    auto default_it = config_->formats.find("default");
    if (default_it != config_->formats.end()) {
        return default_it->second;
    }

    // 最后的备用方案：返回空字符串
    static const std::string empty_format = "{}";
    return empty_format;
}

// get_state模板方法实现
template <typename ConfigType>
template <typename ValueType>
std::string ModuleBase<ConfigType>::get_state(ValueType value, bool lesser) {
    if (config_->states.empty()) {
        return "";
    }

    // 获取GTK样式上下文
    GtkStyleContext *context = gtk_widget_get_style_context(event_box_);
    if (!context) {
        return "";
    }

    // 获取当前状态并排序
    using ThresholdType = typename ConfigType::ThresholdType;
    std::vector<std::pair<std::string, ThresholdType>> sorted_states;
    for (const auto &state : config_->states) {
        sorted_states.emplace_back(state.first, state.second);
    }

    // 根据lesser参数排序状态
    std::sort(sorted_states.begin(), sorted_states.end(), [lesser](const auto &a, const auto &b) {
        return lesser ? a.second < b.second : a.second > b.second;
    });

    // 找到匹配的状态并设置CSS类
    std::string valid_state;
    for (const auto &state : sorted_states) {
        bool condition = lesser ? value <= state.second : value >= state.second;
        if (condition && valid_state.empty()) {
            gtk_style_context_add_class(context, state.first.c_str());
            valid_state = state.first;
        } else {
            gtk_style_context_remove_class(context, state.first.c_str());
        }
    }

    return valid_state;
}

// 定时器回调
template <typename ConfigType> gboolean ModuleBase<ConfigType>::timer_callback(gpointer user_data) {
    ModuleBase<ConfigType> *module = static_cast<ModuleBase<ConfigType> *>(user_data);
    if (module) {
        module->update();
    }
    return G_SOURCE_CONTINUE;
}

// 按钮点击回调
template <typename ConfigType>
gboolean ModuleBase<ConfigType>::button_press_callback(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    ModuleBase<ConfigType> *module = static_cast<ModuleBase<ConfigType> *>(user_data);
    if (module) {
        common::log_info("Button press event received in module");
        // 调用虚函数，允许子类重载行为
        module->handle_button_press(event);
        // 总是返回TRUE，阻止事件继续传递给Waybar
        return TRUE;
    }
    // 返回TRUE阻止事件继续传递
    return TRUE;
}

// 默认的虚函数实现
template <typename ConfigType> gboolean ModuleBase<ConfigType>::handle_button_press(GdkEventButton *event) {
    // 根据按钮类型确定动作键
    std::string action_key;
    switch (event->button) {
    case 1: // 左键
        action_key = "on-left-click";
        break;
    case 2: // 中键
        action_key = "on-middle-click";
        break;
    case 3: // 右键
        action_key = "on-right-click";
        break;
    default:
        return TRUE; // 不处理其他按钮
    }

    // 查找配置的动作
    auto it = config_->actions.find(action_key);
    if (it != config_->actions.end()) {
        execute_action(it->second);
    }

    return TRUE;
}

// 执行动作的通用方法
template <typename ConfigType> void ModuleBase<ConfigType>::execute_action(const std::string &action) {
    if (action.empty()) {
        return;
    }

    // 使用system执行命令
    int result = std::system(action.c_str());
    if (result != 0) {
        common::log_error("Failed to execute action '{}', exit code: {}", action, result);
    }
}

// 滚轮事件回调
template <typename ConfigType>
gboolean ModuleBase<ConfigType>::scroll_event_callback(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)widget;
    ModuleBase<ConfigType> *module = static_cast<ModuleBase<ConfigType> *>(user_data);
    if (module) {
        // 记录滚轮方向
        std::string direction;
        switch (event->direction) {
        case GDK_SCROLL_UP:
            direction = "UP";
            break;
        case GDK_SCROLL_DOWN:
            direction = "DOWN";
            break;
        case GDK_SCROLL_LEFT:
            direction = "LEFT";
            break;
        case GDK_SCROLL_RIGHT:
            direction = "RIGHT";
            break;
        case GDK_SCROLL_SMOOTH:
            direction = "SMOOTH";
            break;
        default:
            direction = "UNKNOWN";
            break;
        }
        common::log_info("Scroll event received in module, direction: {}", direction);

        // 调用虚函数，允许子类重载行为
        module->handle_scroll(event);
        // 总是返回TRUE，阻止事件继续传递给Waybar
        return TRUE;
    }
    // 返回TRUE阻止事件继续传递
    return TRUE;
}

// 默认的滚轮事件虚函数实现
template <typename ConfigType> gboolean ModuleBase<ConfigType>::handle_scroll(GdkEventScroll *event) {
    // 根据滚动方向确定动作键
    std::string action_key;
    switch (event->direction) {
    case GDK_SCROLL_UP:
        action_key = "on-scroll-up";
        break;
    case GDK_SCROLL_DOWN:
        action_key = "on-scroll-down";
        break;
    case GDK_SCROLL_LEFT:
        action_key = "on-scroll-left";
        break;
    case GDK_SCROLL_RIGHT:
        action_key = "on-scroll-right";
        break;
    default:
        return TRUE; // 不处理其他滚动方向
    }

    // 查找配置的动作
    auto it = config_->actions.find(action_key);
    if (it != config_->actions.end()) {
        execute_action(it->second);
    }

    return TRUE;
}

// 窗口创建回调
template <typename ConfigType> void ModuleBase<ConfigType>::on_widget_realized(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ModuleBase<ConfigType> *module = static_cast<ModuleBase<ConfigType> *>(user_data);
    if (module && module->event_box_) {
        GdkWindow *window = gtk_widget_get_window(module->event_box_);
        if (window) {
            GdkDisplay *display = gdk_window_get_display(window);
            // 根据handles_button_press_标记使用手形或默认指针
            GdkCursorType cursor_type = module->handles_button_press_ ? GDK_HAND2 : GDK_ARROW;
            GdkCursor *cursor = gdk_cursor_new_for_display(display, cursor_type);
            gdk_window_set_cursor(window, cursor);
            g_object_unref(cursor);
        }
    }
}

} // namespace waybar::cffi::base

#endif // WAYBAR_CFFI_MODULE_BASE_HPP