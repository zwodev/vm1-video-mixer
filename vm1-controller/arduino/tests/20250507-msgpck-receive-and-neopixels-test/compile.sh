#!/bin/bash
set -e

SKETCH_NAME="msgpck-receive-and-neopixels-test.ino.uf2"
BUILD_DIR="build"
DEVICE="/dev/ttyACM0"
MOUNT_POINT="/mnt"

echo "[INFO] Compiling..."
arduino-cli compile --fqbn rp2040:rp2040:rpipico --libraries . --build-path $BUILD_DIR .

