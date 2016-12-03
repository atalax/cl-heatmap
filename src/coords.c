
#include <stdio.h>

#include "coords.h"

static projPJ proj_wgs;

void init_projs()
{
	proj_wgs = pj_init_plus("+init=epsg:4326");
}

cl_float2 wgs84_to_meters(cl_float2 wgs, projPJ proj_meters)
{
	cl_float2 ret;
	// The X and Ys are switched intentionally
	double ry = wgs.x * DEG_TO_RAD;
	double rx = wgs.y * DEG_TO_RAD;
	int err = pj_transform(proj_wgs, proj_meters, 1, 1, &ry, &rx, NULL);
	if (err) {
		fprintf(stderr, "Coordinate conversion failed: %s\n", pj_strerrno(err));
	}
	ret.x = rx;
	ret.y = ry;
	return ret;
}
