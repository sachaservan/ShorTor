#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

CHUTNEY_DIR=/opt/chutney
CHUTNEY_REPO=https://git.torproject.org/chutney.git
CHUTNEY_NETWORK="${CHUTNEY_DIR}/networks/basic"
CHUTNEY_BIN="${CHUTNEY_DIR}/chutney"

main() {
	id -u chutney || useradd chutney
	if ! command -v tor &> /dev/null; then
		apt-get install -y tor
	fi
	mkdir -p "$CHUTNEY_DIR"
	chown chutney:users "$CHUTNEY_DIR"

	if [ ! -d "${CHUTNEY_DIR}/.git" ]; then
		sudo -u chutney git clone "$CHUTNEY_REPO" "$CHUTNEY_DIR"
	fi

	cat > "$CHUTNEY_DIR/setup.sh" <<-EOF
	#!/bin/bash
	set -e

	CHUTNEY_DIR="$CHUTNEY_DIR"
	SRC=/opt/shortor-tor

	main () {
	    if [ -f "\$SRC/chutney.tar.gz.base64" ]; then
	        cat "\$SRC/chutney.tar.gz.base64" \
	            | base64 -d \
	            | tar xzf - -C "\$SRC"
	    fi

	    # remove old DirAuthority lines from all torrcs
	    for tor_dir in \$CHUTNEY_DIR/net/nodes/*/; do
	        sed -i '/DirAuthority/d' "\$tor_dir/torrc"
	    done
	    for auth in 00{0,1,2}a; do
	        AUTH_DIR="\$CHUTNEY_DIR/net/nodes/\$auth"
	        rsync -r "\$SRC/\$auth/" "\$AUTH_DIR/"
	        NICKNAME="\$(cat \$AUTH_DIR/fingerprint | cut -d' ' -f 1)"
	        FINGERPRINT="\$(cat \$AUTH_DIR/fingerprint | cut -d' ' -f 2)"
	        AUTH_FINGERPRINT="\$(cat \$AUTH_DIR/keys/authority_certificate | grep fingerprint | cut -d' ' -f 2)"
	        ORPORT="\$(cat "\$AUTH_DIR/torrc" | grep OrPort | cut -d' ' -f 2)"
	        DIRPORT="\$(cat "\$AUTH_DIR/torrc" | grep DirPort | cut -d' ' -f 2)"
	        DIRADDRESS="\$(hostname -I | tr -d ' '):\$DIRPORT"
	        for tor_dir in \$CHUTNEY_DIR/net/nodes/*/; do
	            cp \$tor_dir/torrc{,.bak}
	            echo "DirAuthority \$NICKNAME orport=\$ORPORT no-v2 v3ident=\$AUTH_FINGERPRINT \$DIRADDRESS \$FINGERPRINT" \
	                >> \$tor_dir/torrc
	        done
	    done
	}

	main
	EOF
	chmod +x /opt/chutney/setup.sh

	cat > "$CHUTNEY_DIR/publish.sh" <<-EOF
	#!/bin/bash
	set -e

	CHUTNEY_DIR="$CHUTNEY_DIR"
	TORRC_FILE="${CHUTNEY_DIR}/dirauth.torrc"

	main () {
	    # Empty torrc file
	    echo "ExitPolicy accept \$(hostname -I | tr -d ' '):16000-17000, reject *:*" > \$TORRC_FILE
	    echo "Address \$(hostname -I)" >> \$TORRC_FILE
	    for auth in 00{0,1,2}a; do
	        AUTH_DIR="\$CHUTNEY_DIR/net/nodes/\$auth"
	        NICKNAME="\$(cat \$AUTH_DIR/fingerprint | cut -d' ' -f 1)"
	        FINGERPRINT="\$(cat \$AUTH_DIR/fingerprint | cut -d' ' -f 2)"
	        AUTH_FINGERPRINT="\$(cat \$AUTH_DIR/keys/authority_certificate | grep fingerprint | cut -d' ' -f 2)"
	        ORPORT="\$(cat "\$AUTH_DIR/torrc" | grep OrPort | cut -d' ' -f 2)"
	        DIRPORT="\$(cat "\$AUTH_DIR/torrc" | grep DirPort | cut -d' ' -f 2)"
	        DIRADDRESS="\$(hostname -I | tr -d ' '):\$DIRPORT"
	        echo "DirAuthority \$NICKNAME orport=\$ORPORT no-v2 v3ident=\$AUTH_FINGERPRINT \$DIRADDRESS \$FINGERPRINT" \
	            >> \$TORRC_FILE
	    done
	}
	main
	EOF
	chmod +x /opt/chutney/publish.sh

	cat > "/etc/systemd/system/chutney.service" <<-EOF
	[Unit]
	Description=Chutney: run a bunch of Tor nodes
	Requires=systemd-networkd-wait-online.service
	After=systemd-networkd-wait-online.service
	Before=tor@default.service tor@bridge.service tor@exit.service

	[Service]
	Type=forking
	User=chutney
	WorkingDirectory=$CHUTNEY_DIR
	ExecStartPre=/bin/sh -c "CHUTNEY_LISTEN_ADDRESS=\$(hostname -I | cut -d' ' -f 1) $CHUTNEY_BIN configure $CHUTNEY_NETWORK"
	ExecStartPre=/opt/chutney/setup.sh
	ExecStart=$CHUTNEY_BIN start $CHUTNEY_NETWORK
	ExecStartPost=/opt/chutney/publish.sh
	ExecStartPost=$CHUTNEY_BIN wait_for_bootstrap $CHUTNEY_NETWORK
	ExecStop=$CHUTNEY_BIN stop $CHUTNEY_NETWORK
	Restart=always
	TimeoutSec=200s

	[Install]
	WantedBy=multi-user.target
	EOF

	mkdir -p /etc/systemd/system/tor@default.service.d
	cat > /etc/systemd/system/tor@default.service.d/deletefiles.conf <<-EOF
	[Service]
	# this needs to happen every time Chutney restarts (incl. on reboots)
	ExecStartPre=-find /var/lib/tor -type f -delete
	EOF

	mkdir -p /etc/systemd/system/tor@.service.d
	cat > /etc/systemd/system/tor@.service.d/deletefiles.conf <<-EOF
	[Service]
	# this needs to happen every time Chutney restarts (incl. on reboots)
	ExecStartPre=-find /var/lib/tor-instances/%i -type f -delete
	EOF

	systemctl daemon-reload
	systemctl enable chutney.service
}

main $@
