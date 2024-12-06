#!/bin/bash

SCRIPTPATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p go/bin
mkdir -p go/pkg
mkdir -p go/src/mator-db

cp src/*.go go/src/mator-db/
cp -r src/asn go/src/mator-db/

cd go/src/mator-db/
go mod init mator-db
go mod tidy
go build -o ../../bin/mator-db
cd -
cp go/bin/mator-db ./
