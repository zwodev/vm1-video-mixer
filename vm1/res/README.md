# Enable autostart of VM1 and LocalSend

1) Check if directory `~/.config/systemd/user/` exists. If not, create it.
2) Copy `vm1.service` and `localsend.service` to `~/.config/systemd/user/`
3) Enable vm1.service:

```
$ systemctl --user enable localsend.service
$ systemctl --user enable vm1.service
$ systemctl --user daemon-reload
```

4) Reboot Raspberry Pi

# Enable external antenna (on Compute Module only)

1) Add `dtparam=ant2` to `/boot/firmware/config.txt`
2) disable power management for wifi adapter with systemd on system level:

```
sudo nano /etc/systemd/system/wifi-power-save-off.service
```

content:

```
[Unit]
Description=Disable WiFi Power Saving
After=network.target

[Service]
Type=oneshot
ExecStart=/sbin/iwconfig wlan0 power off
RemainAfterExit=true

[Install]
WantedBy=multi-user.target

```

enable it with:

```
sudo systemctl daemon-reexec 
sudo systemctl enable wifi-power-save-off.service
sudo systemctl start wifi-power-save-off.service   # or reboot
```

3) Reboot Raspberry Pi
