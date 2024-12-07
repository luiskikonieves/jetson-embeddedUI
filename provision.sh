#!/bin/bash

if [ "$(id -u)" -ne 0 ]; then
        echo "Please run provisioning script as root." >&2
        exit 1
fi

echo "Removing Ubuntu desktop."
apt-get purge ubuntu-desktop -y
rm -rf ~/Desktop/

echo "Applying OS and library updates."
apt-get update ; apt full-upgrade -y;
apt autoremove -y

echo "Installing tools and dependencies."
apt-get install build-essential cmake libwebsockets-dev -y

echo "Building JSON library."
wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
tar -xvzf v3.11.3.tar.gz
mkdir json-3.11.3/build
cd json-3.11.3/build

cmake ..
make
make install

echo "Cleaning up JSON files."
cd ../..
rm -rf json-3.11.3
rm v3.11.3.tar.gz

echo "Configuring eth0 to 192.168.1.10."
nmcli connection modify Wired\ connection\ 1 ipv4.addresses 192.168.1.10/24 ipv4.method manual

echo "Disabling power saving modes."
systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target

echo "Creating necessary folders."
mkdir ~/configuration
mkdir /var/www/
mkdir /var/www/webFiles

echo "Copying web files and setting permissions."
chmod 755 /var/www/webFiles
chmod 644 /var/www/webFiles/*
chown -R volyjetson:volyjetson /var/www/webFiles

echo ""
echo ""
echo ""

# Check if application exists in the current directory
if [ -f "./jetson-embeddedUI" ]; then
    echo "Application found in current directory. Creating systemd service."

    # Create systemd service file
    cat << EOF > /etc/systemd/system/jetson-embeddedUI.service
[Unit]
Description=EmbeddedUI Application
After=network.target

[Service]
ExecStart=/home/jetson/jetson-embeddedUI
Restart=on-failure
RestartSec=5
StartLimitInterval=500
StartLimitBurst=5
User=root
Group=root
Environment=PATH=/usr/bin:/usr/local/bin
WorkingDirectory=/home/jetson

[Install]
WantedBy=multi-user.target
EOF

    echo "Enabling and starting service."
    systemctl daemon-reload
    systemctl enable jetson-embeddedUI.service
    systemctl start jetson-embeddedUI.service

    echo "Application daemonized; It will run on boot."

else
    echo "Application not found in current directory. Skipping systemd service creation."
fi