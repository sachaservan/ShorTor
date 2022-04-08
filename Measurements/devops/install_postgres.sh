#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

# Environment variables to specify on launch (in $PROJECT_DIR/shortor.conf):
# - PG_BACKUP_DIR: this is where dumps will be read from/written to
CONFIG_DIR=/etc/postgresql/12/main

# Postgres DB name
DB_NAME=shortor
DB_USER=shortor

PROJECT_DIR=/opt/shortor  # install project-related files here

install_postgres_config() {
	sudo -u postgres mkdir -p /var/lib/postgresql/12/main/data
	cat > "$CONFIG_DIR/postgresql.conf" <<-EOF
	include_dir = 'conf.d'
	listen_addresses = '*'  # bind all interfaces
	password_encryption = 'scram-sha-256'
	hba_file = '${CONFIG_DIR}/pg_hba.conf'
	ident_file = '${CONFIG_DIR}/pg_ident.conf'
	ssl = on
	ssl_cert_file = '${CONFIG_DIR}/server.crt'
	ssl_key_file= '${CONFIG_DIR}/server.key'
	ssl_ca_file= '${CONFIG_DIR}/server.ca'
	restart_after_crash = on
	autovacuum = on
	EOF

	cat > "$CONFIG_DIR/pg_hba.conf" <<-EOF
	# TYPE    DATABASE    USER   ADDRESS    METHOD
	  local   all         all    peer
	  hostssl all         all    0.0.0.0/0  cert map=ssl
	  hostssl all         all    ::1/128    cert map=ssl
	EOF

	cat > "$CONFIG_DIR/pg_ident.conf" <<-EOF
	# MAPNAME  SYSTEM-USERNAME       PG-USERNAME
	  ssl      /^(.*).example\.com$  \1
	EOF
}

##########################################################################################
## Backups
##
## Must set $PG_BACKUP_DIR on the instance for backups to work; should be
## writable by the `postgres` user.
##
## We have a "backup" cron job that dumps periodically to the backup dir, and a
## "restore" systemd oneshot job that restores on boot.
##########################################################################################

install_backup_services() {
	# RESTORE
	cat > "$PROJECT_DIR/restore.sh" <<-EOF
	#!/usr/bin/env bash
	main() {
	    BACKUP_FILE="\$PG_BACKUP_DIR/latest.gz"
	    if ! psql --command '\l' | grep $DB_NAME; then
	        echo "Creating DB $DB_NAME"
	        createdb -T template0 $DB_NAME
	    fi
	    if ! psql --command '\du' | grep $DB_USER; then
	        echo "Creating DB user $DB_USER"
	        createuser $DB_USER
	        psql --command "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER"
	    fi
	    if psql --command '\dt' $DB_NAME | grep relays; then
	        echo "DB already exists; delete tables for restore to do anything"
	    elif [ -f "\$BACKUP_FILE" ]; then
	        echo "Restoring from \$BACKUP_FILE"
	        gunzip -c "\$BACKUP_FILE" | psql $DB_NAME
	        vacuumdb -d $DB_NAME -z
	        psql --command "GRANT ALL ON ALL TABLES IN SCHEMA public TO $DB_USER" $DB_NAME
	        psql --command "GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO $DB_USER" $DB_NAME
	    else
	        echo "No existing database and no backup; creating tables."
	        export SHORTOR_DB_URL="postgresql://postgres@/${DB_NAME}?host=/var/run/postgresql/"
	        export PYTHONPATH="/opt/shortor/:/opt/shortor/ting:\$PYTHONPATH"
	        /opt/shortor/venv/bin/python -m latencies.db create
	        psql --command "GRANT ALL ON ALL TABLES IN SCHEMA public TO $DB_USER" $DB_NAME
	        psql --command "GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO $DB_USER" $DB_NAME
	    fi
	}
	main
	EOF
	chmod +x "$PROJECT_DIR/restore.sh"
	cat > /etc/systemd/system/shortor-restore.service <<-EOF
	[Unit]
	Description=Restore ShorTor DB from backup
	After=postgresql@12-main.service
	Requires=postgresql@12-main.service

	[Service]
	Type=oneshot
	ExecStart=$PROJECT_DIR/restore.sh
	User=postgres
	RemainAfterExit=true
	EnvironmentFile=$PROJECT_DIR/shortor.conf
	StandardOutput=journal

	[Install]
	WantedBy=multi-user.target
	EOF

	# BACKUP
	cat > "$PROJECT_DIR/backup.sh" <<-EOF
	#!/usr/bin/env bash
	main() {
	    if ! psql --command '\l' | grep $DB_NAME; then
	        echo "No DB to back up!"
	        exit 1
	    fi
	    BACKUP_FILE="\$PG_BACKUP_DIR/latest.gz"
	    if [ -d "\$PG_BACKUP_DIR" ]; then
	        pg_dump $DB_NAME | gzip > /tmp/backup.gz
	        mv /tmp/backup.gz "\$PG_BACKUP_DIR/latest.gz"
	    fi
	}
	main
	EOF
	chmod +x "$PROJECT_DIR/backup.sh"
	cat > /etc/systemd/system/shortor-backup.service <<-EOF
	[Unit]
	Description=Back up ShorTor DB
	After=postgresql@12-main.service
	Requires=postgresql@12-main.service

	[Service]
	Type=oneshot
	ExecStart=$PROJECT_DIR/backup.sh
	User=postgres
	RemainAfterExit=false
	EnvironmentFile=$PROJECT_DIR/shortor.conf
	StandardOutput=journal

	[Install]
	WantedBy=shortor-backup.timer.target
	EOF

	cat > /etc/systemd/system/shortor-backup.timer <<-EOF
	[Unit]
	Description=Run ShorTor backup hourly, starting 15 minutes after boot

	[Timer]
	OnBootSec=15min
	OnUnitActiveSec=1d

	[Install]
	WantedBy=timers.target
	EOF
	systemctl daemon-reload
	systemctl enable shortor-backup.timer
	systemctl enable shortor-restore.service
}

main() {
	id -u postgres || useradd postgres --uid 117
	if ! command -v psql &>/dev/null; then
		apt-get install -y postgresql
	fi
	mkdir -p "$PROJECT_DIR"

	install_postgres_config
	install_backup_services
}

main $@
