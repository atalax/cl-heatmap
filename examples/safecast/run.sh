#! /bin/bash

cd $(dirname $0)

zooms=(		10		11		12		13		14		15		16)
ranges=(	250		200		200		180		150		120		100)
filters=(	750		600		600		540		450		360		300)

for x in $(seq 0 6); do
	cl-heatmap -z ${zooms[$x]} -b50.22,14.23,49.92,14.68 -o ../../web/tiles -i ./data.json \
		-k heat -f ${filters[$x]} -c "-DRANGE=${ranges[$x]} -DMIN=20 -DMAX=80"
done
