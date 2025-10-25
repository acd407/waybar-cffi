waybar-cffi-rapl(5)

# NAME

waybar-cffi-rapl - waybar的CFFI RAPL功耗监控模块

# DESCRIPTION

**waybar-cffi-rapl**是一个CFFI模块，用于监控Intel RAPL (Running Average Power Limit)功耗信息。该模块通过读取/sys/class/powercap/intel-rapl接口获取CPU功耗数据，并以可视化方式显示在Waybar状态栏中。

该模块显示以下功耗信息：
- Package功耗：整个CPU封装的功耗
- Core功耗：CPU核心的功耗
- Other功耗：非核心部分的功耗（Package - Core）

# CONFIGURATION

该模块通过waybar配置文件中的"cffi/rapl"部分进行配置。

地址：

```
"waybar": {
    "cffi/rapl": {
        "module_path": "/path/to/librapl.so",
        // 其他配置选项
    }
}
```

配置选项：

*module_path*: ++
    类型: string ++
    默认值: N/A ++
    指向librapl.so共享库的路径，必须提供

*interval*: ++
    类型: integer ++
    默认值: 10 ++
    更新间隔，单位为秒

*sysfs-dir*: ++
    类型: string ++
    默认值: "/sys/class/powercap/intel-rapl:0" ++
    RAPL sysfs目录路径

*warning-threshold*: ++
    类型: double ++
    默认值: 50.0 ++
    警告状态阈值，单位为瓦特

*critical-threshold*: ++
    类型: double ++
    默认值: 100.0 ++
    严重状态阈值，单位为瓦特

*format*: ++
    类型: string ++
    默认值: "{icon}\u2004{power}W" ++
    格式字符串

*format-warning*: ++
    类型: string ++
    默认值: 与format相同 ++
    警告状态的格式字符串

*format-critical*: ++
    类型: string ++
    默认值: 与format相同 ++
    严重状态的格式字符串

*format-tooltip*: ++
    类型: string ++
    默认值: "Package: {package_power:.2f}W\nCore: {core_power:.2f}W\nOther: {other_power:.2f}W" ++
    工具提示格式字符串

*tooltip*: ++
    类型: bool ++
    默认值: true ++
    是否启用工具提示

*format-icons*: ++
    类型: json ++
    默认值: {"default": "⚡", "warning": "⚡", "critical": "⚡"} ++
    不同状态对应的图标

# FORMAT REPLACEMENTS

{icon}: 当前状态对应的图标

{power}: Package功耗（瓦特）

{package_power}: Package功耗（瓦特）

{core_power}: Core功耗（瓦特）

{other_power}: Other功耗（瓦特）

# EXAMPLES

基本配置：

```
"cffi/rapl": {
    "module_path": "/usr/local/lib/librapl.so",
    "interval": 5,
    "format": "⚡ {power}W"
}
```

高级配置：

```
"cffi/rapl": {
    "module_path": "/usr/local/lib/librapl.so",
    "interval": 2,
    "warning-threshold": 30.0,
    "critical-threshold": 65.0,
    "format": "{icon} {power}W",
    "format-icons": {
        "default": "🔋",
        "warning": "⚡",
        "critical": "🔥"
    },
    "format-tooltip": "Package: {package_power:.2f}W\nCore: {core_power:.2f}W\nOther: {other_power:.2f}W"
}
```

最小化配置：

```
"cffi/rapl": {
    "module_path": "/usr/local/lib/librapl.so",
    "format": "{power}W"
}
```

# STYLE

该模块使用#cffi-rapl CSS选择器。

示例：

```
#cffi-rapl {
    background-color: #1a1a1a;
    color: #ffffff;
    padding: 0 10px;
    margin: 0 5px;
    border-radius: 5px;
}

#cffi-rapl.warning {
    background-color: #ff9800;
    color: #000000;
}

#cffi-rapl.critical {
    background-color: #f44336;
    color: #ffffff;
    animation: blink 2s step-end infinite;
}

@keyframes blink {
    50% {
        opacity: 0.5;
    }
}
```

# COMPILING AND INSTALLING

1. 确保系统安装了必要的依赖：
- GTK3开发包
- CMake (>= 3.16)
- C++20兼容编译器
- nlohmann/json库

2. 编译项目：

```
mkdir build
cd build
cmake ..
make
```

3. 安装模块：

```
sudo make install
```

4. 在Waybar配置中添加模块：

```
"cffi/rapl": {
    "module_path": "/usr/local/lib/librapl.so"
}
```

# NOTES

- 该模块需要Intel RAPL支持，大多数现代Intel处理器都支持此功能
- 如果/sys/class/powercap/intel-rapl接口不可用，模块初始化将失败
- 功耗计算基于能量差和时间差，因此第一次更新不会显示有效功耗值
- 模块会自动处理RAPL计数器的回绕情况

# SEE ALSO

waybar(1), waybar-cffi(5), waybar-cffi-cpu(5)
