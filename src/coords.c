
#include <stdio.h>

#include "coords.h"

projPJ proj_wgs;
projPJ proj_meters;

int init_projs()
{
	proj_wgs = pj_init_plus("+init=epsg:4326");
	proj_meters = pj_init_plus("+init=epsg:3045");
	if (proj_wgs == NULL || proj_meters == NULL) {
		fprintf(stderr, "Failed to initialize the projections: %s\n",
				pj_strerrno(pj_errno));
		return -1;
	}
	return 0;
}

point_t wgs84_to_meters(point_t wgs)
{
	point_t ret;
	double ry = wgs.lat * DEG_TO_RAD;
	double rx = wgs.lng * DEG_TO_RAD;
	int err = pj_transform(proj_wgs, proj_meters, 1, 1, &ry, &rx, NULL);
	if (err) {
		fprintf(stderr, "Coordinate conversion failed: %s\n", pj_strerrno(err));
	}
	ret.x = rx;
	ret.y = ry;
	return ret;
}
