#!/bin/bash
if [ -f /boot/firstboot_done ]; then
    cd "$HOME/vm1"
    ./vm1
fi