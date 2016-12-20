
#include "common.h"

// See QGIS/src/plugins/heatmap/heatmap.cpp
float quartic_kernel(float dist, float bw)
{
	dist = clamp((float)(dist / bw), -0.9999f, 0.9999f);
	return 15.0 / 16.0 * pow(1 - dist, 2);
}

__kernel void generate_pixel(
		float4 trx,
		float4 try,
		uint npts,
		read_only global float2 *pts,
		read_only global float *vals,
		write_only image2d_t out)
{
	uint x = get_global_id(0);
	uint y = get_global_id(1);

	float2 self = tile_to_cartesian((float2)((float)x / TILE_SIZE, (float)y / TILE_SIZE),
								  trx, try);

	float val = 0.0;
	float sw = 0.0;
	float best = FLT_MAX;
	for (uint i = 0; i < npts; i++) {
		float dist = pow(self.x - pts[i].x, 2) + pow(self.y - pts[i].y, 2);
		float w = quartic_kernel(dist, RANGE * RANGE);
		if (dist < best) {
			best = dist;
		}
		sw += w;
		val += vals[i] * w;
	}
	int cid = 0;
	if (best < RANGE * RANGE && sw > 0.0) {
		val /= sw;
		float rval = (val - MIN) / MAX;
		cid = clamp((int)(rval * COLORS_LEN), 1, COLORS_LEN - 1);
	}

#ifdef HIGHLIGHT_BORDERS
	if (x == 0 || y == 0 || x == TILE_SIZE - 1 || y == TILE_SIZE - 1) {
		cid = 150;
	}
#endif

	write_imageui(out, (int2)(x, y), (uint4)(cid, 0, 0, 0));
}
