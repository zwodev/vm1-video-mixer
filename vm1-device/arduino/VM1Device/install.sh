curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
./bin/arduino-cli config init
./bin/arduino-cli config set board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
./bin/arduino-cli core update-index
./bin/arduino-cli core install rp2040:rp2040
./bin/arduino-cli lib install "Adafruit NeoPixel"
