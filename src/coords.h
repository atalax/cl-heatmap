
#ifndef COORDS_H
#define COORDS_H

#include <stdbool.h>
#include <proj_api.h>
#include <CL/cl.h>

#include "utils.h"


static inline cl_float2 round_point(cl_float2 in, int to, bool up)
{
	cl_float2 ret = {
		.x = round(in.x + (up ? to : 0), to),
		.y = round(in.y + (up ? to : 0), to)
	};
	return ret;
}

static inline cl_float2 wgs84_to_tile(cl_float2 wgs, int z)
{
	cl_float2 ret;
	ret.x = (int)(floor((wgs.y + 180.0) / 360.0 * pow(2.0, z)));
	ret.y = (int)(floor((1.0 - log( tan(wgs.x * M_PI/180.0) + 1.0 /
						cos(wgs.x * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
	return ret;
}

static inline cl_float2 tile_to_wgs84(cl_float2 tile, int z)
{
	cl_float2 ret;
	double n = M_PI - 2.0 * M_PI * tile.y / pow(2.0, z);
	ret.x = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
	ret.y = tile.x / pow(2.0, z) * 360.0 - 180;
	return ret;
}

void init_projs();
cl_float2 wgs84_to_meters(cl_float2 wgs, projPJ proj_meters);


#endif
