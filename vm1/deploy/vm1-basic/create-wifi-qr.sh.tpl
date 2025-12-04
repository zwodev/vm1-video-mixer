#!/bin/bash

# Run nmcli and parse output for SSID/PASSWORD/SECURITY
info="$(sudo nmcli dev wifi show-password)"

# Extract fields
ssid=$(echo "$info" | grep '^SSID' | head -n1 | awk -F: '{print $2}' | xargs)
password=$(echo "$info" | grep '^Password' | awk -F: '{print $2}' | xargs)
security=$(echo "$info" | grep '^Security' | awk -F: '{print $2}' | xargs)

# Default to WPA if security is blank
[ -z "$security" ] && security="WPA"

# Warn and exit if password or ssid empty
if [[ -z "$ssid" || -z "$password" ]]; then
    echo "Could not detect SSID or password. Aborting."
    exit 1
fi

# Construct WiFi QR code format
# For WPA/WPA2: WIFI:T:WPA;S:;P:;;
# For open:     WIFI:T:nopass;S:;;
if [[ "$security" =~ WPA|WPA2|WPA3 ]]; then
    qr="WIFI:T:WPA;S:$ssid;P:$password;;"
elif [[ "$security" =~ WEP ]]; then
    qr="WIFI:T:WEP;S:$ssid;P:$password;;"
elif [[ "$security" == "None" || "$security" == "OPEN" || -z "$password" ]]; then
    qr="WIFI:T:nopass;S:$ssid;;"
else
    # Default/fallback
    qr="WIFI:T:WPA;S:$ssid;P:$password;;"
fi

# Output PNG
outfile="/home/vm1/vm1/data/wifi_qr.png"
qrencode -o "$outfile" -l H -m 2 -t PNG "$qr"
chown vm1:vm1 "$outfile"

echo "QR code saved to $outfile"
