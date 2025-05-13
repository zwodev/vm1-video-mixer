# VM-1 Controller

## Installation of existing binaries

~~On the Raspberry Pi Pico (V1), press and hold the `bootsel` button while plugging in into your USB-Port. It will appear as a removable disk. Drag and Drop the file `bin/VM1_Keyboard.ino.uf2` onto it. The Pi Pico will reboot and show up as a keyboard on your computer. You can then use it with your VM-1.~~

--> t.b.a.

## Assignments

### Buttons
* The controller acts simply as a HID Keyboard
* left-most button is `KEY_LEFT_SHIFT`
* two buttons at the top-right are `KEY_ARROW_LEFT` and `KEY_ARROW_RIGHT`
* the 8x3 button matrix is:
	* `q` `w` `e` `r` `t` `y` `u` `i`
	* `a` `s` `d` `f` `g` `h` `j` `k`
	* `z` `x` `c` `v` `b` `n` `m` `,`

### Rotary Encoder
* Acts as `ARROW_DOWN` (turn CW) and `ARROW_UP` (turn CCW)

### LEDs

* The four LEDs blink in a row. 
* ~~ You can change them with *Serial Commands*:~~
	* `0` - start the running animation (default)
	* `1` - turn on LED 1
	* `2` - turn on LED 2
	* `3` - turn on LED 3
	* `4` - turn on LED 4
	

## Working on the project

If you want to extend the VM-1 keyboard, you can use that with the source-code found in the `VM1_Keyboard`-folder. We use the PiPico board library by [Earle Philhower](https://github.com/earlephilhower/arduino-pico?tab=readme-ov-file#installation) because the implementation of the HID-Keyboard worked better for us. Please follow the installation instructions in the link (adding the link to the board-library in the Arduino-Settings). 

#### Used Libraries

In Arduino IDE, choose `Raspberry Pi Pico` (*not the Arduino Mbed, but the one mentioned above!*) as your Board. The following libraries are used:

* ~~[https://github.com/gbr1/rp2040-encoder-library](https://github.com/gbr1/rp2040-encoder-library)~~
	* this library gets in conflict with the Adafruit NeoPixels library. 


# Installing Arduino CLI on your Raspberry Pi

I attached the PiPico (aka the VM-1 Keyboard) to the Raspberry Pi (aka the VM-1). It shall be programmed directly from there.

The PiPico shows up as serial device under `/dev/ttyACM0`

In the home directory:

```
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
export PATH=$PATH:$HOME/bin

arduino-cli config init
arduino-cli config set board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core update-index
arduino-cli core install rp2040:rp2040

```

In the arduino project directory (`/home/vm1/Documents/coding/vm1-video-mixer/vm1-controller/arduino/VM1_Keyboard`) you can compile it with

arduino-cli compile --fqbn rp2040:rp2040:rpipico --verbose --build-path ./build .


### ~~not needed (hopefully) BEGIN~~
~~upload the file to pi pico~~
~~press bootsel button and plug the pico in~~
~~(mount if needed)~~
~~mkdir -p ~/pico_mount~~
~~lsblk~~
~~sudo mount /dev/sda1 ~/pico_mount~~


~~sudo cp your_program.uf2 ~/pico_mount/~~
### ~~not needed END~~




# get a serial monitor

sudo apt-get install minicom

minicom -b 115200 -o -D /dev/ttyACM0

# Install Neopixel library

arduino-cli lib install "Adafruit NeoPixel"

# Install MessagePack

git clone https://github.com/hideakitai/MsgPack.git
git clone https://github.com/hideakitai/DebugLog.git
git clone https://github.com/hideakitai/ArxTypeTraits.git
git clone https://github.com/hideakitai/ArxContainer.git






