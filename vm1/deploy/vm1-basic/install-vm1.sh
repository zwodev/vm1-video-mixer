#!/bin/bash
if [ ! -f /boot/firstboot_done ]; then
    sleep 5
    raspi-config nonint do_wifi_country DE
    nmcli device wifi hotspot ssid VM1 password testtesttest
    nmcli connection modify Hotspot connection.autoconnect yes connection.autoconnect-priority 100
    raspi-config nonint do_expand_rootfs
    touch /boot/firstboot_done
    sync
    echo b > /proc/sysrq-trigger
fi