#! /usr/bin/env python3

import argparse
import json
import dateutil.parser

parser = argparse.ArgumentParser(description="Convert bgeigie log to JSON")
parser.add_argument("files", nargs="+")

args = parser.parse_args()

NANO_CPM_FACTOR = 334

def convert_coord(val, hem):
    ln = 3 if hem in ['W', 'E'] else 2
    dgr = int(val[:ln])
    mins = float(val[ln:])
    return (dgr + mins / 60) * (-1 if hem in ["S", "W"] else 1)

output = {"points": []}
for fil in args.files:
    with open(fil) as f:
        for line in f:
            pt = {}
            split = line.split(",")
            if split[0] != "$BNRDD":
                continue
            try:
                dt = dateutil.parser.parse(split[2])
            except:
                continue
            lat = convert_coord(split[7], split[8])
            lon = convert_coord(split[9], split[10])

            pt["loc"] = [lat, lon]
            pt["val"] = int(split[3]) / NANO_CPM_FACTOR * 1000
            output["points"].append(pt)

print(json.dumps(output))
