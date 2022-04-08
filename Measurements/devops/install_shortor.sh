#!/usr/bin/env bash
set -eufx -o pipefail
shopt -s failglob

PROJECT_DIR=/opt/shortor

main() {
	# Install system dependencies
	apt-get install -y \
		python3-venv python-is-python3 python3-dev \
		build-essential libpq-dev \
		nfs-common

	# Set up directories that shortor owns
	mkdir -p "$PROJECT_DIR"

	VENV_DIR="$PROJECT_DIR/venv"
	python -m venv "$VENV_DIR"
	"$PROJECT_DIR/venv/bin/pip" install --no-cache-dir wheel
	"$PROJECT_DIR/venv/bin/pip" install --no-cache-dir -r /tmp/requirements.txt
	cp -r /tmp/latencies $PROJECT_DIR/
	cp -r /tmp/ting $PROJECT_DIR/
	if [ -z "$(ls -A ${PROJECT_DIR}/ting)" ]; then
		echo "Ting empty. Did you init+update your submodules?"
		exit 1
	fi
}

main $@
