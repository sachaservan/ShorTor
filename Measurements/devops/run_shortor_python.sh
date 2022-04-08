#!/usr/bin/env bash
# Run Python with the shorter environment loaded
[[ $USER != "shortor" ]] && echo "must be run as shortor (sudo -u shortor $0 ...)" && exit 1
export PYTHONPATH=/opt/shortor:/opt/shortor/ting
while IFS= read -r line; do
    eval "export $line"
done < /opt/shortor/shortor.conf
/opt/shortor/venv/bin/python $@
