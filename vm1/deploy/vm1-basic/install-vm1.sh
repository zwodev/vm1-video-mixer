#!/bin/bash
# Generate a random 4-character suffix with uppercase letters and digits
RAND_SUFFIX=$(tr -dc '0-9' </dev/urandom | head -c4)

# Construct the SSID with the suffix
SSID="VM1_$RAND_SUFFIX"

if [ ! -f /boot/firstboot_done ]; then
    sleep 5
    raspi-config nonint do_i2c 0
    raspi-config nonint do_wifi_country DE
    nmcli device wifi hotspot ssid "$SSID" password vmone12345
    nmcli connection modify Hotspot connection.autoconnect yes connection.autoconnect-priority 100 802-11-wireless.band a
    raspi-config nonint do_expand_rootfs
    touch /boot/firstboot_done
    sync
    echo b > /proc/sysrq-trigger
fi