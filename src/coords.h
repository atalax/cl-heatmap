
#ifndef COORDS_H
#define COORDS_H

#include <stdbool.h>
#include <proj_api.h>

#include "utils.h"
#include "types.h"


static inline point_t round_point(point_t in, int to, bool up)
{
	point_t ret = {
		.x = round(in.x + (up ? to : 0), to),
		.y = round(in.y + (up ? to : 0), to)
	};
	return ret;
}

static inline point_t wgs84_to_tile(point_t wgs, int z)
{
	point_t ret;
	ret.x = (int)(floor((wgs.lng + 180.0) / 360.0 * pow(2.0, z)));
	ret.y = (int)(floor((1.0 - log( tan(wgs.lat * M_PI/180.0) + 1.0 /
						cos(wgs.lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
	return ret;
}

static inline point_t tile_to_wgs84(point_t tile, int z)
{
	point_t ret;
	double n = M_PI - 2.0 * M_PI * tile.y / pow(2.0, z);
	ret.lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
	ret.lng = tile.x / pow(2.0, z) * 360.0 - 180;
	return ret;
}

int init_projs();
point_t wgs84_to_meters(point_t wgs);


#endif
