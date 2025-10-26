waybar-cffi-network(5)

# NAME

waybar-cffi-network - Network module for Waybar CFFI

# DESCRIPTION

The *network* module displays network interface information in Waybar.

# CONFIGURATION

Addressed by *network*

*format*: ++
    typeof: string ++
    The format string for displaying network information. ++
    Default: "{icon}\u2004{ifname}"

*format-disconnected*: ++
    typeof: string ++
    The format string for displaying disconnected network status. ++
    Default: "{icon}\u2004{ifname} Disconnected"

*format-linked*: ++
    typeof: string ++
    The format string for displaying connected network status. ++
    Default: "{icon}\u2004{ifname}"

*format-wireless*: ++
    typeof: string ++
    The format string for displaying wireless network status. ++
    Default: "{icon}\u2004{essid} {signal_strength}%"

*format-wired*: ++
    typeof: string ++
    The format string for displaying wired network status. ++
    Default: "{icon}\u2004{ifname}"

*interface*: ++
    typeof: string ++
    The network interface to monitor. If empty, the module will automatically select the best interface. ++
    Default: "" (auto-select)

*accumulate-bandwidth*: ++
    typeof: bool ++
    Whether to accumulate bandwidth statistics. ++
    Default: false

*cycle-time*: ++
    typeof: int ++
    Bandwidth calculation cycle time in seconds. ++
    Default: 5

*max-bandwidth*: ++
    typeof: int ++
    Maximum bandwidth in Mbps, used for calculating percentage. ++
    Default: 1000

*interval*: ++
    typeof: int ++
    The interval in seconds for updating the module. ++
    Default: 1

*tooltip*: ++
    typeof: bool ++
    Whether to show a tooltip when hovering over the module. ++
    Default: false

*format-tooltip*: ++
    typeof: string ++
    The format string for the tooltip. ++
    Default: "Interface: {ifname}\nIP: {ipaddr}\nIPv6: {ipv6}\nRX Total: {bandwidthRxTot}\nTX Total: {bandwidthTxTot}\nRX Rate: {bandwidthDownBits}\nTX Rate: {bandwidthUpBits}\nDown: {bandwidthDownBytes}\nUp: {bandwidthUpBytes}"

*icons*: ++
    typeof: json ++
    Icons for different states. ++
    Default: {"default": "󰈀", "disconnected": "󰈂", "linked": "󰈁", "wired": "󰈀", "wireless": "󰖩"}

*formats*: ++
    typeof: json ++
    Format strings for different states. ++
    Default: {"default": "{icon}\u2004{usage}%"}

*states*: ++
    typeof: json ++
    Threshold values for different states. ++
    Default: {"warning": 20, "critical": 50}

# FORMAT REPLACEMENTS

*{icon}*: Icon representing the network state

*{ifname}*: Network interface name

*{ipaddr}*: IPv4 address

*{ipv6}*: IPv6 address

*{essid}*: WiFi network name (ESSID)

*{signal_strength}*: WiFi signal strength as a percentage

*{bandwidthRxTot}*: Total received data (formatted with 5-character width)

*{bandwidthTxTot}*: Total transmitted data (formatted with 5-character width)

*{bandwidthRx}*: Current receive rate (in bits per second)

*{bandwidthTx}*: Current transmit rate (in bits per second)

*{bandwidthDownBits}*: Current download rate in bits per second

*{bandwidthUpBits}*: Current upload rate in bits per second

*{bandwidthDownBytes}*: Current download rate in bytes per second

*{bandwidthUpBytes}*: Current upload rate in bytes per second

*{netcidr}*: Network interface with CIDR notation

*{netspeed}*: Total network speed (sum of upload and download)

# EXAMPLES

```
"network": {
    "interface": "wlan0",
    "format": "{icon}\u2004{ifname}",
    "format-wireless": "{icon}\u2004{essid} {signal_strength}%",
    "format-wired": "{icon}\u2004{ifname}",
    "format-disconnected": "{icon}\u2004Disconnected",
    "tooltip": true,
    "interval": 5
}
```

# SEE ALSO

waybar(5), waybar-cffi(1)

# BUGS

For bug reports and feature requests, please visit:
<https://github.com/Alexays/Waybar/issues>