
# Test for receiving a MessagePack message

The PiPico is connected to the Raspberry Pi via USB-Serial. [This C++ code](https://github.com/zwodev/vm1-video-mixer/tree/master/vm1/cpp/tests/serial-to-pico) sends a *MessagePack* message to the Pi Pico, telling it how some *NeoPixels* should light up.


## Dependencies

* **MsgPack** library by hideakitai (TODO: check version). Install it via Arduino IDE or clone it directly into your project folder:

```
$ git clone https://github.com/hideakitai/MsgPack.git
$ git clone https://github.com/hideakitai/DebugLog.git
$ git clone https://github.com/hideakitai/ArxTypeTraits.git
$ git clone https://github.com/hideakitai/ArxContainer.git
```

TODO: check why it didn't work with `arduino-cli lib install`

* **Adafruit NeoPixel** library by Adafruit (TODO: check version). Install it also via Arduino IDE or via command line tool:

```
$ arduino-cli lib install "Adafruit NeoPixel"
```

## Optional: Installing the Arduino CLI on your Raspberry Pi

This is only if you want to keep the VM-1 keyboard connected to the Raspberry Pi and work on the code / upload binaries to the Pico from there.

```
$ cd ~
$ curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
$ export PATH=$PATH:$HOME/bin

$ arduino-cli config init
$ arduino-cli config set board_manager.additional_urls https://github.com/earlephilhower/
$ arduino-pico/releases/download/global/package_rp2040_index.json
$ arduino-cli core update-index
$ arduino-cli core install rp2040:rp2040


```

In the arduino project directory (`/home/vm1/Documents/coding/vm1-video-mixer/vm1-controller/arduino/VM1_Keyboard`) you can compile it with

```
$ arduino-cli compile --fqbn rp2040:rp2040:rpipico --verbose --build-path ./build .
```

or you can use the upload-script upload.sh, which compiles and uploads the code to the PiPico:

	1) Hold the BOOTSEL-Button on the PiPico and plug it into the Raspberry Pi
	2) run ./upload.sh

The PiPico shows up as serial device under `/dev/ttyACM0`


## Optional: Serial Monitor 

```
$ sudo apt-get install minicom
$ minicom -b 115200 -o -D /dev/ttyACM0
```





