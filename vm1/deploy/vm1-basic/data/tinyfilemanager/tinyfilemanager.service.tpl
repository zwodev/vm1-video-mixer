[Unit]
Description=Autostart TinyFileManager
After=multi-user.target

[Service]
Type=simple
User=<VM1_USER>
Restart=always
ExecStart=/usr/bin/php -S 0.0.0.0:8080 -t /home/<VM1_USER>/
StandardError=journal

[Install]
WantedBy=default.target
