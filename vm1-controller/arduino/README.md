# VM-1 Controller

## Installation

On the Raspberry Pi Pico (V1), press and hold the `bootsel` button while plugging in into your USB-Port. It will appear as a removable disk. Drag and Drop the file `VM1_Keyboard.uf2` onto it. The Pi Pico will reboot and show up as a keyboard.

## Assignments

### Buttons
* The controller acts simply as a HID Keyboard
* left-most button is the `Arrow Right` key
* two buttons at the top-right are `1` and `2`
* the 8x3 button matrix is:
	* `q` `w` `e` `r` `t` `z` `u` `i`
	* `a` `s` `d` `f` `g` `h` `j` `k`
	* 	`y` `x` `c` `v` `b` `n` `m` `,`

### Rotary Encoder
* Acts as `Button Up` (turn CW) and `Button Down` (turn CCW)

### LEDs
* The four LEDs blink in a row. 
* You can change them with *Serial Commands*
	* `0` - start the running animation (default)
	* `1` - turn on LED 1
	* `2` - turn on LED 2
	* `3` - turn on LED 3
	* `4` - turn on LED 4

	
#### Used Libraries

In Arduino IDE, choose `Raspberry Pi Pico` as your Board. The following libraries are

* [https://github.com/victorsvi/MatrixKeypad](https://github.com/victorsvi/MatrixKeypad)
* [https://github.com/gbr1/rp2040-encoder-library](https://github.com/gbr1/rp2040-encoder-library)