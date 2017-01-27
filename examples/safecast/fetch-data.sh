#! /bin/sh

cd $(dirname $0)

wget https://api.safecast.org/system/measurements.csv -O measurements.csv
