#! /bin/bash

cd $(dirname $0)

for zoom in $(seq 10 16); do
	cl-heatmap -z $zoom -b50.22,14.23,49.92,14.68 -o ../../web/tiles -i ./data.json -k ../../kernels/tdoa.cl \
		-c "-DSCALE_BY=800"
done
