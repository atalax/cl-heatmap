#! /bin/bash

BINDTO="127.0.0.1:9900"
cd $(dirname $0)

echo "Running busybox httpd on $BINDTO"
busybox httpd -fp "$BINDTO" -c httpd.conf -v
