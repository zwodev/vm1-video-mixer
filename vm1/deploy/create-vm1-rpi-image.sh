#!/bin/bash
cd rpi-image-gen
sudo rm -R ./work/vm1-basic
./build.sh -c vm1-basic  -D ../vm1-basic/ -o ../vm1-basic/vm1-basic.options