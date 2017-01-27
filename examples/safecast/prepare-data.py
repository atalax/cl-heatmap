#! /usr/bin/env python3

import csv
import json

data = []
with open("measurements.csv") as f:
    for line in csv.reader(f):
        if line[4] != "cpm":
            continue
        try:
            lat, lng = map(float, line[1:3])
        except ValueError:
            continue
        if not (49.0 <= lat <= 51.0) or not (14.0 <= lng <= 16.0):
            continue
        data.append({"val": line[3], "loc": [lat, lng]})

with open("data.json", "w") as f:
    json.dump({"points": data}, f)

print("Collected %d points to data.json" % len(data))
