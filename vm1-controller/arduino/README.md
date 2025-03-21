# VM-1 Controller

## Installation of existing binaries

On the Raspberry Pi Pico (V1), press and hold the `bootsel` button while plugging in into your USB-Port. It will appear as a removable disk. Drag and Drop the file `bin/VM1_Keyboard.ino.uf2` onto it. The Pi Pico will reboot and show up as a keyboard on your computer. You can then use it with your VM-1.

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
* You can change them with *Serial Commands*:
	* `0` - start the running animation (default)
	* `1` - turn on LED 1
	* `2` - turn on LED 2
	* `3` - turn on LED 3
	* `4` - turn on LED 4

	

## Working on the project

If you want to extend the VM-1 keyboard, you can use that with the source-code found in the `VM1_Keyboard`-folder. We use the PiPico board library by [Earle Philhower](https://github.com/earlephilhower/arduino-pico?tab=readme-ov-file#installation) because the implementation of the HID-Keyboard worked better for us. Please follow the installation instructions in the link (adding the link to the board-library in the Arduino-Settings). 

#### Used Libraries

In Arduino IDE, choose `Raspberry Pi Pico` (*not the Arduino Mbed, but the one mentioned above!*) as your Board. The following libraries are used:

* [https://github.com/gbr1/rp2040-encoder-library](https://github.com/gbr1/rp2040-encoder-library)
