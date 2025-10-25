waybar-cffi-gpu(5)

# NAME

waybar-cffi-gpu - GPU module for Waybar CFFI

# DESCRIPTION

The *cffi-gpu* module is a CFFI (C Foreign Function Interface) implementation for Waybar that displays the current GPU utilization and VRAM usage. It is written in modern C++ and provides a high-performance GPU monitoring solution with customizable display options.

This module is designed to be used with Waybar's CFFI interface, allowing for more complex modules to be created using different programming languages while maintaining integration with Waybar's core functionality.

# CONFIGURATION

Addressed by *cffi/gpu*

*module_path*: ++
	typeof: string ++
	The path to the dynamic library to load to control the widget. ++
	Example: "/path/to/libgpu.so" or "gpu" if installed in a standard library path.

*interval*: ++
	typeof: integer ++
	default: 1 ++
	The interval in seconds in which the information gets polled.

*format*: ++
	typeof: string ++
	default: {icon}\u2004{gpu_usage}% ++
	The format, how information should be displayed. Any of the following placeholders can be used:

	- {icon}: Icon based on GPU usage state
	- {gpu_usage}: Current GPU usage percentage
	- {vram_used}: Current VRAM usage (in GB, if available)

*format-alt*: ++
	typeof: string ++
	default: {icon}\u2004{gpu_usage}%\u2004{vram_used:.1f}GB ++
	The format string for the module when VRAM usage is displayed. This format can include VRAM information.

*format-tooltip*: ++
	typeof: string ++
	default: GPU Usage: {gpu_usage}%\nVRAM Used: {vram_used:.2f}GB ++
	The format for the tooltip displayed on hover.

*gpu-usage-path*: ++
	typeof: string ++
	default: /sys/class/drm/card1/device/gpu_busy_percent ++
	Path to the GPU usage file.

*vram-used-path*: ++
	typeof: string ++
	default: /sys/class/drm/card1/device/mem_info_vram_used ++
	Path to the VRAM usage file.

*show-vram*: ++
	typeof: bool ++
	default: true ++
	Whether to show VRAM usage.

*states*: ++
	typeof: object ++
	Defines the warning and critical thresholds for GPU usage. ++
	*warning*: GPU usage percentage that triggers the warning state (default: 60) ++
	*critical*: GPU usage percentage that triggers the critical state (default: 85)

*icons*: ++
	typeof: object ++
	Defines the icons to use for different GPU states. ++
	*default*: Icon for normal GPU usage (default: "󰢮") ++
	*warning*: Icon for warning GPU usage (default: "󰢮") ++
	*critical*: Icon for critical GPU usage (default: "󰢮")

*tooltip*: ++
	typeof: bool ++
	default: true ++
	Option to disable tooltip on hover.

*click-actions*: ++
	typeof: object ++
	default: {"left": "toggle-mode"} ++
	Actions to perform on click. The "toggle-mode" action switches between showing only GPU usage and showing GPU usage with VRAM.

*expand*: ++
	typeof: bool ++
	default: false ++
	Enables this module to consume all left over space dynamically.

# FORMAT REPLACEMENTS

*{gpu_usage}*: Current GPU usage as a percentage.

*{vram_used}*: Current VRAM usage in gigabytes.

*{icon}*: Icon corresponding to the current GPU state.

*{state}*: Current GPU state (normal, warning, or critical).

# EXAMPLES

Basic configuration:

```
"cffi/gpu": {
	"module_path": "gpu",
	"interval": 2,
	"format": "GPU {gpu_usage}%"
}
```

Advanced configuration with custom thresholds and icons:

```
"cffi/gpu": {
	"module_path": "/usr/local/lib/libgpu.so",
	"interval": 1,
	"format": "{icon} {gpu_usage}%",
	"format-alt": "{icon} {gpu_usage}% {vram_used:.1f}GB",
	"gpu-usage-path": "/sys/class/drm/card0/device/gpu_busy_percent",
	"vram-used-path": "/sys/class/drm/card0/device/mem_info_vram_used",
	"states": {
		"warning": 70,
		"critical": 90
	},
	"icons": {
		"default": "󰢮",
		"warning": "󰢮",
		"critical": "󰢮"
	},
	"tooltip": true,
	"format-tooltip": "GPU Usage: {gpu_usage}%\nVRAM Used: {vram_used:.2f}GB"
}
```

Minimal configuration with default settings:

```
"cffi/gpu": {
	"module_path": "gpu"
}
```

# STYLE

The classes and IDs are managed by the cffi dynamic library. The module applies different CSS classes based on the GPU state:

- *#gpu* - The base identifier for the module
- *.warning* - Applied when GPU usage exceeds the warning threshold
- *.critical* - Applied when GPU usage exceeds the critical threshold

Example CSS styling:

```
#cffi-gpu.warning {
    color: #ffeb3b;
}

#cffi-gpu.critical {
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

This will install the shared library as *libgpu.so* in the system library path, which can then be referenced in the Waybar configuration.

# NOTES

- This module reads GPU usage information from /sys/class/drm/card1/device/gpu_busy_percent
- This module reads VRAM usage information from /sys/class/drm/card1/device/mem_info_vram_used
- The module automatically updates at the specified interval
- The module handles errors gracefully and will display default values if GPU information cannot be retrieved
- Clicking on the module toggles between showing only GPU usage and showing GPU usage with VRAM

# SEE ALSO

waybar(5), waybar-cffi(5), waybar-cffi-cpu(5), waybar-cffi-rapl(5), waybar-cffi-temperature(5)