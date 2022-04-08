#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

PROJECT_DIR=/opt/shortor


# Ports
# ORPorts: 5100s
SHORTOR_BRIDGE_ORPORT=5100
SHORTOR_EXIT_ORPORT=5101
# DirPorts: 7100s
SHORTOR_EXIT_DIRPORT=7101
# SocksPorts: 9100s
SHORTOR_BRIDGE_SOCKS_PORT=9100
SHORTOR_EXIT_SOCKS_PORT=9101
SHORTOR_DEFAULT_SOCKS_PORT=9102


# Tor installs a couple of systemd units by default:
# - tor@default: enabled, uses /etc/tor/torrc
# - tor@.service: uses /etc/tor/instances/%i/torrc
# - tor.service: a noop service used as a target to launch all the Tors!
# We need 3 Tor instances on this machine:
# 1. A client, which ShorTor will connect through (tor@default)
# 2. A bridge, which is the first hop (tor@bridge)
# 3. An exit relay, the final hop (tor@exit)
# 
# Network config:
# - ExitRelay ORPort public
# - *maybe* BridgeRelay ORPort public? or at least reachable within OpenStack
# - 16000-17000 (EchoServeR) public? or at least reachable within OpenStack
install_tor () {
	apt-get install -y apt-transport-https

	cat > /etc/apt/sources.list.d/tor.list <<-EOF
	deb     [signed-by=/usr/share/keyrings/tor-archive-keyring.gpg] https://deb.torproject.org/torproject.org focal main
	deb-src [signed-by=/usr/share/keyrings/tor-archive-keyring.gpg] https://deb.torproject.org/torproject.org focal main
	EOF
	wget -O- https://deb.torproject.org/torproject.org/A3C4F0F979CAA22CDBA8F512EE8CBC9E886DDD89.asc \
		| gpg --dearmor \
		| tee /usr/share/keyrings/tor-archive-keyring.gpg \
		> /dev/null
	apt-get update
	apt-get install -y tor deb.torproject.org-keyring

	for user in "_tor-bridge" "_tor-exit"; do
	  grep -q -E "^$user:" /etc/group || groupadd $user
	  id -u $user || useradd -g $user $user
	  usermod -a -G $user shortor
	done
	usermod -a -G debian-tor debian-tor

	SHORTOR_CONTACT_INFO="shortor [AT] csail.mit [DOT] edu people.csail.mit.edu/zjn/tor.txt"

	RUN_TOR=/run/tor-instances
	SHORTOR_BRIDGE_SOCKET=$RUN_TOR/bridge/socket

	SHORTOR_BRIDGE_NICKNAME=ShorTorBridge

	SHORTOR_EXIT_SOCKET=$RUN_TOR/exit/socket
	SHORTOR_EXIT_NICKNAME=ShorTorExit

	SHORTOR_WRITABLE=/var/lib/tor-instances
	SHORTOR_BRIDGE=$SHORTOR_WRITABLE/bridge
	SHORTOR_EXIT=$SHORTOR_WRITABLE/exit

	SHORTOR_COMMON_TORRC="/etc/tor/common-torrc"
	SHORTOR_DEFAULT_TORRC="/etc/tor/torrc"
	SHORTOR_TOR_INSTANCES="/etc/tor/instances"
	SHORTOR_BRIDGE_TORRC="$SHORTOR_TOR_INSTANCES/bridge/torrc"
	SHORTOR_EXIT_TORRC="$SHORTOR_TOR_INSTANCES/exit/torrc"

	SHORTOR_BRIDGE_PID=$RUN_TOR/bridge/pid
	SHORTOR_EXIT_PID=$RUN_TOR/exit/pid

	for role in bridge exit; do
		mkdir -p $SHORTOR_WRITABLE/${role}/data
		mkdir -p $RUN_TOR/${role}
		mkdir -p $SHORTOR_TOR_INSTANCES/${role}
		chown _tor-${role}:users -R $SHORTOR_WRITABLE/${role} $RUN_TOR/${role}
		find $RUN_TOR/${role} -type d -exec chmod 700 {} \;
		find $RUN_TOR/${role} -type f -exec chmod 600 {} \;
	done

	TOR_RUNTIME_CONF=/etc/tor/torrc.runtime
	touch $TOR_RUNTIME_CONF

	cat > $SHORTOR_DEFAULT_TORRC <<-EOF
	UseBridges 1
	ExitRelay 0
	UpdateBridgesFromAuthority 0
	SocksPort $SHORTOR_DEFAULT_SOCKS_PORT
	FetchUselessDescriptors 1
	MaxClientCircuitsPending 1024
	%include $SHORTOR_COMMON_TORRC
	EOF

	cat > $SHORTOR_COMMON_TORRC <<-EOF
	AvoidDiskWrites 1
	CookieAuthentication 1
	LearnCircuitBuildTimeout 0
	DirReqStatistics 0
	UseMicroDescriptors 0
	DownloadExtraInfo 1
	RunAsDaemon 1
	ContactInfo $SHORTOR_CONTACT_INFO
	Log notice syslog
	%include $TOR_RUNTIME_CONF
	EOF

	cp $SHORTOR_COMMON_TORRC $SHORTOR_BRIDGE_TORRC
	cat >> $SHORTOR_BRIDGE_TORRC <<-EOF
	PublishServerDescriptor 0
	ExitRelay 0
	BridgeRelay 1
	ORPort $SHORTOR_BRIDGE_ORPORT
	DataDirectory $SHORTOR_BRIDGE/data
	SocksPort $SHORTOR_BRIDGE_SOCKS_PORT
	PidFile $SHORTOR_BRIDGE_PID
	ExitPolicy reject *:*
	Nickname $SHORTOR_BRIDGE_NICKNAME
	EOF

	cp $SHORTOR_COMMON_TORRC $SHORTOR_EXIT_TORRC
	cat >> $SHORTOR_EXIT_TORRC <<-EOF
	ExitRelay 1
	RelayBandwidthRate 80 KB  # Cannot be lower than this
	RelayBandwidthBurst 10 MB
	ORPort $SHORTOR_EXIT_ORPORT
	DataDirectory $SHORTOR_EXIT/data
	SocksPort $SHORTOR_EXIT_SOCKS_PORT
	ExitPolicyRejectPrivate 0
	ClientRejectInternalAddresses 0
	PublishServerDescriptor 1
	DirPort $SHORTOR_EXIT_DIRPORT
	PidFile $SHORTOR_EXIT_PID
	Nickname $SHORTOR_EXIT_NICKNAME
	%include /etc/tor/exitpolicy.torrc
	%include /etc/tor/instances/exit/torrc.runtime
	EOF

	mkdir -p /etc/systemd/system/tor@.service.d/
	cat > "/etc/systemd/system/tor@.service.d/restart.conf" <<-EOF
	[Service]
	RestartSec=1min
	EOF

	mkdir -p /etc/systemd/system/tor@default.service.d
	# Regard the "ExecStartPost" command: we've encountered a situation where
	# the Tor client can't find the bridge on startup. It turns out if you just
	# try to *use* Tor, it fixes itself.
	cat > /etc/systemd/system/tor@default.service.d/afterbridge.conf <<-EOF
	[Unit]
	After=tor@bridge.service tor@exit.service

	[Service]
	ExecStartPre=/opt/shortor/venv/bin/python /opt/shortor/latencies/check_exit_in_consensus.py
	ExecStartPost=/bin/sh -c 'sleep 30; curl -x socks5h://localhost:9102 --max-time 30 http://www.example.com &'
	EOF

	mkdir -p /var/lib/tor-instances
	cat > /var/lib/tor-instances/setup_exit.sh <<-EOF
	#!/bin/bash
	set -ex

	SRC=/var/lib/tor-instances

	main () {
	    if [ -f "\$SRC/exit.tar.gz.base64" ]; then
	        if [ -d "\$SRC/exit" ]; then
	            rm -rf "\$SRC/exit"
	        fi
	        mkdir -p "\$SRC/exit"
	        cat "\$SRC/exit.tar.gz.base64" \
	            | base64 -d \
	            | tar xzf - -C "\$SRC/exit"
	        mv "\$SRC/exit.tar.gz.base64" "\$SRC/exit.tar.gz.base64.bak"
	    fi
	    chown -R _tor-exit:users "\$SRC/exit"
	}

	main
	EOF
	chmod +x /var/lib/tor-instances/setup_exit.sh
	mkdir -p /etc/systemd/system/tor@exit.service.d
	cat > /etc/systemd/system/tor@exit.service.d/setup.conf <<-EOF
	[Service]
	ExecStartPre=/var/lib/tor-instances/setup_exit.sh
	ExecStartPre=/usr/bin/tor --defaults-torrc /run/tor-instances/exit.defaults -f /etc/tor/instances/exit/torrc --verify-config
	EOF

	systemctl enable tor@bridge.service
	systemctl enable tor@exit.service
}

install_shortor_service() {
	# Set up directories that shortor owns
	for dir in "$PROJECT_DIR" /run/celery /var/log/celery; do
	  mkdir -p $dir
	  chown shortor:users -R $dir
	done

	CELERY_BIN=$PROJECT_DIR/venv/bin/celery
	CELERY_APP=latencies
	CELERYD_PID_FILE='/run/celery/celery.pid'
	CELERYD_LOG_FILE='/var/log/celery/celery.log'
	CELERYD_LOG_LEVEL=DEBUG
	CELERYD_CONCURRENCY=75
	CELERYD_OPTS=""
	SYSTEMCTL_BIN=$(which systemctl)
	LOGROTATE_BIN=$(which logrotate)
	LOGROTATE_CONF='/etc/shortor-logrotate/logrotate.conf'

	cat > "/etc/systemd/system/shortor-observer.service" <<-EOF
	# https://docs.celeryproject.org/en/stable/userguide/daemonizing.html#daemon-systemd-generic
	[Unit]
	Description=Celery Service
	After=network.target tor.service

	[Service]
	Type=forking
	User=shortor
	Environment=PYTHONPATH=$PROJECT_DIR:$PROJECT_DIR/ting
	EnvironmentFile=$PROJECT_DIR/shortor.conf
	PIDFile=${CELERYD_PID_FILE}
	ExecStartPre=/opt/shortor/venv/bin/python /opt/shortor/latencies/check_exit_in_consensus.py client
	ExecStart=/bin/sh -c '${CELERY_BIN} -A $CELERY_APP worker \
	  --concurrency=${CELERYD_CONCURRENCY} \
	  --detach \
	  --pool=prefork \
	  -Ofair \
	  --pidfile=${CELERYD_PID_FILE} \
	  --logfile=${CELERYD_LOG_FILE} \
	  --loglevel="${CELERYD_LOG_LEVEL}" \
	  $CELERYD_OPTS'
	Restart=always
	RestartSec=10min

	[Install]
	WantedBy=multi-user.target
	EOF

	cat > /etc/tmpfiles.d/shortor-observer.conf <<-EOF
	d /run/celery 0755 shortor users -
	d /var/log/celery 0755 shortor users -
	d /opt/celery 0755 shortor users -
	EOF

	mkdir -p /etc/shortor-logrotate
	# The following will rotate the log if it's larger than 100M, compress old
	# logs, and only keep the last ten.
	cat > $LOGROTATE_CONF <<-EOF
	${CELERYD_LOG_FILE} {
	   compress
		 rotate 5
     size 1G
		 postrotate
  	 		${SYSTEMCTL_BIN} restart shortor-observer.service
		 endscript
	}
	EOF
	chmod 644 $LOGROTATE_CONF
	chown root $LOGROTATE_CONF

	cat > "/etc/systemd/system/shortor-log-rotator.service" <<-EOF
	[Unit]
	Description=Rotate celery logs
	After=shortor-observer.service

	[Service]
	Type=oneshot
	User=root
	ExecStart=/bin/sh -c '${LOGROTATE_BIN} ${LOGROTATE_CONF}'
	EOF

	cat > "/etc/systemd/system/shortor-log-rotator.timer" <<-EOF
	[Unit]
	Description=Rotate celery logs

	[Timer]
	OnCalendar=hourly
	Persistent=true

	[Install]
	WantedBy=timers.target
	EOF

	systemctl daemon-reload
	systemctl enable shortor-log-rotator.service
	systemctl enable shortor-log-rotator.timer
}

install_pgbouncer() {
	apt-get install -y pgbouncer
	cat > "/etc/pgbouncer/pgbouncer.ini" <<-EOF
		[databases]
		%include /etc/pgbouncer/runtime.ini
		[users]
		[pgbouncer]
		logfile = /var/log/postgresql/pgbouncer.log
		pidfile = /var/run/postgresql/pgbouncer.pid
		listen_addr = 127.0.0.1
		listen_port = 6432
		unix_socket_dir = /var/run/postgresql
		server_tls_sslmode = verify-ca
		server_tls_ca_file = /opt/shortor/shortor.ca
		server_tls_key_file = /opt/shortor/shortor.key
		server_tls_cert_file = /opt/shortor/shortor.crt
		auth_type = trust
		auth_file = /etc/pgbouncer/userlist.txt
		pool_mode = transaction
		max_db_connections = 10

	EOF

	cat > "/etc/pgbouncer/userlist.txt" <<-EOF
		"shortor" ""
	EOF
}

main() {
	id -u shortor || useradd shortor --uid 25554

	install_tor

	usermod -a -G debian-tor shortor

	install_shortor_service
	install_pgbouncer
}

main $@
