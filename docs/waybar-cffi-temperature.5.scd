waybar-cffi-temperature(5)

# NAME

waybar-cffi-temperature - Temperature module for Waybar CFFI

# DESCRIPTION

The *temperature* module displays the current system temperature in Celsius, Fahrenheit, or Kelvin.

# CONFIGURATION

Addressed by *temperature*

*format*: ++
    typeof: string ++
    default: "{icon}\u2004{temperature_c}" ++
    The format string.

*format-tooltip*: ++
    typeof: string ++
    default: "Temperature: {temperature_c}°C\nFahrenheit: {temperature_f}°F\nKelvin: {temperature_k}K" ++
    The format string for the tooltip.

*hwmon-path*: ++
    typeof: string or array ++
    Path to a hwmon temperature sensor. If an array is provided, the first existing path will be used.

*hwmon-path-abs*: ++
    typeof: string or array ++
    Absolute path to a hwmon directory. If an array is provided, the first existing directory will be used.

*input-filename*: ++
    typeof: string ++
    default: "temp1_input" ++
    Name of the input file inside the hwmon directory.

*thermal-zone*: ++
    typeof: integer ++
    default: 0 ++
    The thermal zone to read from.

*interval*: ++
    typeof: integer ++
    default: 10 ++
    The interval in seconds to update the temperature.

*tooltip*: ++
    typeof: bool ++
    default: true ++
    Whether to show the tooltip.

*icons*: ++
    typeof: object ++
    The icons to use for different states.

*formats*: ++
    typeof: object ++
    The format strings to use for different states.

*states*: ++
    typeof: object ++
    The temperature thresholds for different states.

# FORMAT REPLACEMENTS

*{temperature_c}*: The temperature in Celsius.

*{temperature_f}*: The temperature in Fahrenheit.

*{temperature_k}*: The temperature in Kelvin.

*{icon}*: The icon corresponding to the current state.

# EXAMPLES

```
"temperature": {
    "hwmon-path": "/sys/class/hwmon/hwmon0/temp1_input",
    "format": "🌡️ {temperature_c}°C",
    "format-tooltip": "Temperature: {temperature_c}°C\nFahrenheit: {temperature_f}°F\nKelvin: {temperature_k}K",
    "interval": 5,
    "states": {
        "warning": 70,
        "critical": 85
    }
}
```

# SEE ALSO

waybar(5), waybar-cffi(5)

# AUTHORS

Written by the Waybar CFFI contributors.