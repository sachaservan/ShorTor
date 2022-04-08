#!/bin/bash
# Test the e2e flow in the Vagrant environment.
# 
# Assumes: venv in /venv
# Assumes: this directory is mounted in /app
#
# Args: $1 = "sqlite" (default) or "pg"
set -eo pipefail

sudo mkdir -p /var/run/celery /var/log/celery
sudo chown vagrant /var/run/celery /var/log/celery

if [ ! -d /venv/bin ]; then
  python -m venv /venv
fi
source /venv/bin/activate
pip install -r /app/requirements.txt

eval $(cat /opt/shortor/shortor.conf)
export PYTHONPATH="/app:/app/ting:${PYTHONPATH}"
export CELERY_BROKER_URL="redis://localhost:6379/0"
export RUN_TIME=5

if [ -z $1 ] || [ $1 == "sqlite" ]; then
  echo "Using SQLite..."
  export SHORTOR_DB_URL="sqlite:////tmp/data.db"
elif [ $1 == "pg" ]; then
  echo "Using PostgreSQL..."
  export SHORTOR_DB_URL="postgresql+psycopg2://vagrant:password@localhost/vagrant"
else
  echo "Bad DB $1; expected [pg] or [sqlite]."
  exit 1
fi

/app/run.sh
