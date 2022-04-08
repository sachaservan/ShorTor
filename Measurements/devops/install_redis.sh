#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

# Security notes
# - bind to all interfaces; this is dangerous so we:
#   - have no password set at build time, which triggers "protected mode" and redis doesn't accept connections
#   - set a strong password at runtime
#   - openstack security group: tcp/6379 within the group

# put `replicaof 192.168.0.1 6379` in the boot settings
# https://redis.io/topics/sentinel
# if we enable sentinel need fork of Celery for TLS though
# https://github.com/celery/celery/issues/6455

install_redis_configs() {
	cat > /etc/redis/redis.conf <<-EOF
	include /etc/redis/runtime.conf

	## basic config
	protected-mode yes
	timeout 0
	databases 1
	
	## systemd
	daemonize yes
	supervised systemd
	pidfile /var/run/redis/redis-server.pid
	logfile /var/log/redis/redis-server.log
	
	## persistence
	# save after X seconds if at least Y keys changed
	save 900 1
	save 300 10
	save 60 10000
	dir /var/lib/redis
	dbfilename dump.rdb
	appendonly yes
	appendfilename "appendonly.aof"
	EOF

	# /etc/redis/runtime.conf
}

main() {
	if ! command -v redis-server &> /dev/null; then
		# We want the latest stable (6.0) for TLS support
		add-apt-repository -y ppa:redislabs/redis
		apt-get install -y redis
		systemctl enable redis-server
	fi

	install_redis_configs
}

main $@
