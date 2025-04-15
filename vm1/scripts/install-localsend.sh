#!/bin/bash

SCRIPT_DIR=$(dirname "$0")
VIDEO_DIR=$(realpath $SCRIPT_DIR/../videos)
LOCALSEND_PACKET_NAME=LocalSend-1.17.0-linux-arm-64.deb
LOCALSEND_SETTINGS_FILE=~/.local/share/org.localsend.localsend_app/shared_preferences.json

read -p "Do you want to install LocalSend? Press Enter to start."
sudo apt remove xdg-desktop-portal
sudo apt install xvfb
wget https://github.com/localsend/localsend/releases/download/v1.17.0/$LOCALSEND_PACKET_NAME
sudo dpkg -i $LOCALSEND_PACKET_NAME
sudo apt-get install -f
rm $LOCALSEND_PACKET_NAME
wget https://github.com/zwodev/vm1-video-mixer/tree/master/vm1/res/localsend/shared_preferences.json -O LOCALSEND_SETTINGS_FILE
sed -i "s|###VIDEO_DST###|$VIDEO_DIR|g" $LOCALSEND_SETTINGS_FILE



