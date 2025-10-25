waybar-cffi-cpu(5)

# NAME

waybar-cffi-cpu - CFFI CPU module for Waybar

# DESCRIPTION

The *cffi-cpu* module is a CFFI (C Foreign Function Interface) implementation for Waybar that displays the current CPU utilization. It is written in modern C++ and provides a high-performance CPU monitoring solution with customizable display options.

This module is designed to be used with Waybar's CFFI interface, allowing for more complex modules to be created using different programming languages while maintaining integration with Waybar's core functionality.

# CONFIGURATION

Addressed by *cffi/cpu*

*module_path*: ++
	typeof: string ++
	The path to the dynamic library to load to control the widget. ++
	Example: "/path/to/libcpu.so" or "cpu" if installed in a standard library path.

*interval*: ++
	typeof: integer ++
	default: 1 ++
	The interval in seconds in which the information gets polled.

*format*: ++
	typeof: string ++
	default: {icon}\u2004{usage}% ++
	The format, how information should be displayed.

*format-warning*: ++
	typeof: string ++
	The format to use when CPU usage is in the warning state. If not specified, falls back to *format*.

*format-critical*: ++
	typeof: string ++
	The format to use when CPU usage is in the critical state. If not specified, falls back to *format*.

*format-tooltip*: ++
	typeof: string ++
	default: CPU Usage: {usage}%\nState: {state} ++
	The format for the tooltip displayed on hover.

*tooltip*: ++
	typeof: bool ++
	default: true ++
	Option to disable tooltip on hover.

*states*: ++
	typeof: object ++
	Defines the warning and critical thresholds for CPU usage. ++
	*warning*: CPU usage percentage that triggers the warning state (default: 20) ++
	*critical*: CPU usage percentage that triggers the critical state (default: 50)

*format-icons*: ++
	typeof: object ++
	Defines the icons to use for different CPU states. ++
	*default*: Icon for normal CPU usage (default: "󰾆") ++
	*warning*: Icon for warning CPU usage (default: "󰾅") ++
	*critical*: Icon for critical CPU usage (default: "󰓅")

*expand*: ++
	typeof: bool ++
	default: false ++
	Enables this module to consume all left over space dynamically.

# FORMAT REPLACEMENTS

*{usage}*: Current overall CPU usage as a percentage.

*{icon}*: Icon corresponding to the current CPU state.

*{state}*: Current CPU state (normal, warning, or critical).

# EXAMPLES

Basic configuration:

```
"cffi/cpu": {
	"module_path": "cpu",
	"interval": 2,
	"format": "CPU {usage}%"
}
```

Advanced configuration with custom thresholds and icons:

```
"cffi/cpu": {
	"module_path": "/usr/local/lib/libcpu.so",
	"interval": 1,
	"format": "{icon} {usage}%",
	"format-icons": {
		"default": "",
		"warning": "",
		"critical": ""
	},
	"states": {
		"warning": 30,
		"critical": 70
	},
	"tooltip": true,
	"format-tooltip": "CPU Usage: {usage}%\nState: {state}"
}
```

Minimal configuration with default settings:

```
"cffi/cpu": {
	"module_path": "cpu"
}
```

# STYLE

The classes and IDs are managed by the cffi dynamic library. The module applies different CSS classes based on the CPU state:

- *#cpu* - The base identifier for the module
- *.warning* - Applied when CPU usage exceeds the warning threshold
- *.critical* - Applied when CPU usage exceeds the critical threshold

Example CSS styling:

```
#cffi-cpu.warning {
    color: #ffeb3b;
}

#cffi-cpu.critical {
    color: #f44336;
}
```

# COMPILATION AND INSTALLATION

To compile and install this module:

```
mkdir build && cd build
cmake ..
make
sudo make install
```

This will install the shared library as *libcpu.so* in the system library path, which can then be referenced in the Waybar configuration.

# NOTES

- This module reads CPU usage information from /proc/stat
- The module automatically updates at the specified interval
- CPU usage is calculated as a percentage of non-idle time
- The module handles errors gracefully and will display default values if CPU information cannot be retrieved

# SEE ALSO

waybar(5), waybar-cffi(5), waybar-cpu(5)