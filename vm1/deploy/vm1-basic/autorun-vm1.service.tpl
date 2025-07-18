[Unit]
Description=Autostart VM-1 application
After=multi-user.target

[Service]
User=<VM1_USER>
TTYPath=/dev/tty1
Environment="XDG_RUNTIME_DIR=<VM1_RUNDIR>"
Restart=always
ExecStart=<VM1_APP>
StandardError=journal

[Install]
WantedBy=default.target
