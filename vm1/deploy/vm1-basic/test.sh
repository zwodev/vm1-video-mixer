#!/bin/bash
# Generate a random 4-character suffix with uppercase letters and digits
RAND_SUFFIX=$(tr -dc '0-9' </dev/urandom | head -c4)

# Construct the SSID with the suffix
SSID="VM1_$RAND_SUFFIX"


nmcli device wifi hotspot ssid "$SSID" password vmone12345
