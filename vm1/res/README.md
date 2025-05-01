# Enable autostart of VM1 and LocalSend

1) Check if directory `~/.config/systemd/user/` exists. If not, create it.
2) Copy `vm1.service` and `localsend.service` to `~/.config/systemd/user/`
3) Enable vm1.service:

    systemctl --user enable localsend.service
    systemctl --user enable vm1.service
    systemctl --user daemon-reload

4) Reboot Raspberry Pi

# Enable external antenna (on Compute Module only)

1) Add `dtparam=ant2` to `/boot/firmware/config.txt`
2) Reboot Raspberry Pi