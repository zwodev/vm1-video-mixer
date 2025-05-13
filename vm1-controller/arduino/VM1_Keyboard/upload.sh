#!/bin/bash
set -e

SKETCH_NAME="VM1_Keyboard.ino.uf2"
BUILD_DIR="build"
DEVICE="/dev/ttyACM0"
MOUNT_POINT="/mnt"

echo "[INFO] Compiling..."
arduino-cli compile --fqbn rp2040:rp2040:rpipico --libraries . --build-path $BUILD_DIR .

# echo "[INFO] Resetting Pico into BOOTSEL mode..."
# stty -F $DEVICE 1200
# echo -n > $DEVICE  # Ensure we are ready to send data
# sleep 2          # Give it a short time to reset

echo "[INFO] Waiting for Pico to appear as a mass storage device..."

# Wait for device with 128M size and label "RPI-RP2"
for i in {1..20}; do
  PICO_DEV=$(lsblk -o NAME,SIZE,LABEL -nr | grep '128M.*RPI-RP2' | awk '{print $1}' | head -n1)
  if [ -n "$PICO_DEV" ]; then
    echo "[INFO] Found Pico on /dev/$PICO_DEV"
    break
  fi
  sleep 0.2
done

if [ -z "$PICO_DEV" ]; then
  echo "[ERROR] Pico mass storage device not found. Is it in BOOTSEL mode?"
  exit 1
fi

# Mount and upload
DEVICE_PATH="/dev/$PICO_DEV"
sudo mount $DEVICE_PATH $MOUNT_POINT

UF2_FILE="$BUILD_DIR/$SKETCH_NAME"
echo "[INFO] Copying $UF2_FILE to Pico..."
sudo cp "$UF2_FILE" $MOUNT_POINT/

echo "[INFO] Unmounting..."
sudo umount $MOUNT_POINT

echo "[DONE] Upload complete!"
echo "starting minicom..."
sleep 2
minicom -b 115200 -o -D /dev/ttyACM0
