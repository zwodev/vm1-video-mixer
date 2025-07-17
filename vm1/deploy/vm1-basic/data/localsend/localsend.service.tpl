[Unit]
Description=Autostart localsend
After=multi-user.target

[Service]
Type=simple
User=<VM1_USER>
Restart=always
ExecStart=xvfb-run -a localsend_app
StandardError=journal

[Install]
WantedBy=default.target
