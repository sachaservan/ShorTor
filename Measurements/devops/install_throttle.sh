#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

install_throttle_configs() {
	id -u throttle || useradd throttle
	cat > /etc/systemd/system/throttle.service <<-EOF
	[Unit]
	Description=Throttle: semaphore for networked systems
	After=network.target

	[Service]
	Type=exec
	User=throttle
	ExecStart=throttle --address 0.0.0.0 --configuration /etc/throttle.toml --port 8888
	Restart=always

	[Install]
	WantedBy=multi-user.target
	EOF

	systemctl daemon-reload
	systemctl enable throttle

	# /etc/throttle.toml
	# litter_collection_interval = "5min"
	# [semaphores]
	# TOR_CIRCUITS = 10
}

main() {
	apt-get install -y python3-pip
	pip3 install throttle-server
	install_throttle_configs
}

main $@
