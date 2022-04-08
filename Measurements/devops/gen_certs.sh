#!/usr/bin/env bash
set -e
set -x

CERT_DIR="certs"

make_tls_config() {
	TLS_DIR="$CERT_DIR/tls"
	mkdir -p "$TLS_DIR"
	# Generate CA key/crt
	openssl req -nodes \
		-x509 \
		-days 3650 \
		-newkey rsa:4096 \
		-keyout "$TLS_DIR/ca.key" \
		-out "$TLS_DIR/ca.crt" \
		-sha256 \
		-batch \
		-subj "/CN=ShorTor RSA CA"
	# Generate server key/crt
	for server in postgres redis observer taker; do
		# generate key and signing request
		openssl req -nodes \
			-newkey rsa:2048 \
			-keyout "$TLS_DIR/${server}.key" \
			-out "$TLS_DIR/${server}.req" \
			-sha256 \
			-batch \
			-subj "/CN=shortor.example.com"
		# sign with CA key
		openssl x509 -req \
			-in "$TLS_DIR/${server}.req" \
			-out "$TLS_DIR/${server}.crt" \
			-CA "$TLS_DIR/ca.crt" \
			-CAkey "$TLS_DIR/ca.key" \
			-sha256 \
			-days 2000 \
			-set_serial 456
		rm "$TLS_DIR/${server}.req"
	done
}

make_tor_config() {
	TOR_DIR="${CERT_DIR}/chutney/$1"
	mkdir -p "$TOR_DIR/keys"
	echo "" |
		tor --keygen \
			--DataDirectory "$PWD/$TOR_DIR" \
			--passphrase-fd 0
	echo "PASSWORD" | \
		tor-gencert \
			-i "$TOR_DIR/keys/authority_identity_key" \
			-s "$TOR_DIR/keys/authority_signing_key" \
			-c "$TOR_DIR/keys/authority_certificate" \
			--passphrase-fd 0 \
			--create-identity-key
	tor --list-fingerprint \
		--DataDirectory "$PWD/$TOR_DIR" \
		--ORPort 9000 \
		--Nickname "test${1}"
}

main() {
	mkdir -p "$CERT_DIR"

	make_tor_config 000a
	make_tor_config 001a
	make_tor_config 002a

	# TODO: make exit config

	make_tls_config

	tar czf "$CERT_DIR/chutney.tar.gz" -C "$CERT_DIR/chutney" .
	tar czf "$CERT_DIR/exit.tar.gz" -C "$CERT_DIR/exit" .
}

main $@
