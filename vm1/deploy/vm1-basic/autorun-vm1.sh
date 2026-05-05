#!/bin/bash
if [ -f /boot/firstboot_done ]; then
    cd "$HOME/vm1"
    ./create-wifi-qr.sh
    ./vm1
fi