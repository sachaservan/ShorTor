#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

PROJECT_DIR=/opt/shortor
GEO_DIR=/opt/geo

main() {
    id -u shortor || useradd shortor --uid 25554

    # Set up directories that shortor owns
    for dir in "$PROJECT_DIR"; do
        mkdir -p $dir
        chown shortor:users -R $dir
    done

    if [ -f /tmp/consensus.txt ]; then
       mv /tmp/consensus.txt "${PROJECT_DIR}/"
    fi

    if [ -f /tmp/GeoLite2-City.mmdb ]; then
       mkdir -p "${GEO_DIR}"
       mv /tmp/GeoLite2-City.mmdb "${GEO_DIR}/"
       mv /tmp/GeoLite2-ASN.mmdb "${GEO_DIR}/"
       chown shortor:users -R "${GEO_DIR}"
    fi

	cat > /etc/systemd/system/shortor-consensus-taker.service <<-EOF
	[Unit]
	Description=Check Tor consensus and queue jobs.

	[Service]
	Type=oneshot
	Environment=PYTHONPATH=$PROJECT_DIR:$PROJECT_DIR/ting
	ExecStart=$PROJECT_DIR/venv/bin/python -m latencies.consensus_taker
	User=shortor
	RemainAfterExit=false
	EnvironmentFile=$PROJECT_DIR/shortor.conf
	StandardOutput=journal

	[Install]
	WantedBy=shortor-consensus-taker.timer.target
	EOF

	cat > /etc/systemd/system/shortor-consensus-taker.timer <<-EOF
	[Unit]
	Description=Run ShorTor consensus taker every 15 minutes, starting 15 minutes after boot

	[Timer]
	OnBootSec=15min
	OnUnitActiveSec=15min

	[Install]
	WantedBy=timers.target
	EOF

	systemctl daemon-reload
}

main $@
