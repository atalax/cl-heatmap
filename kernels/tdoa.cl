
#include "common.h"

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

	float2 self = tile_to_cartesian((float2)((float)x / TILE_SIZE,
											 (float)y / TILE_SIZE),
									trx, try);

	float dist = distance(self, pts[0]);
	float err = 0;
	for (uint i = 1; i < npts; i++) {
		float dist2 = distance(self, pts[i]);
		float ad = (dist - dist2) - vals[i];
		ad = pow(ad, 2);
		err += ad;
	}
	err = sqrt(err);
	err /= SCALE_BY;
	int cid = COLORS_LEN - clamp((int)(err * COLORS_LEN), 0, COLORS_LEN - 1) - 1;
	write_imageui(out, (int2)(x, y), (uint4)(cid, 0, 0, 0));
}
