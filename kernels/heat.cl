
#include "common.h"

// See QGIS/src/plugins/heatmap/heatmap.cpp
float quartic_kernel(float dist, float bw)
{
	dist = clamp((float)(dist / bw), -0.9999f, 0.9999f);
	return 15.0 / 16.0 * pow(1 - pow(dist, 2), 2);
}

__kernel void generate_tile(global float4 *ptin,
							global float2 *pts,
							global float *vals,
							global uchar *out_)
{
	uint cx = get_global_id(0);
	uint cy = get_global_id(1);
	uint off = (cy * CHUNK_SIZE + cx) * TILE_SIZE * TILE_SIZE;
	global uchar *out = &out_[off];
	uint off2 = 2 * (cy * CHUNK_SIZE + cx);
	float4 trx = ptin[off2 + 0];
	float4 try = ptin[off2 + 1];

	// This should be far enough
	float2 cornera = tile_to_cartesian((float2)(-1.0, -1.0), trx, try);
	float2 cornerb = tile_to_cartesian((float2)( 2.0,  2.0), trx, try);

	// The transformations axis might not actually point in the same direction as the tile axis
	float4 bbox = (float4) (
		min(cornera.x, cornerb.x),
		min(cornera.y, cornerb.y),
		max(cornera.x, cornerb.x),
		max(cornera.y, cornerb.y)
	);

	for (uint x = 0; x < TILE_SIZE; x++) {
		for (uint y = 0; y < TILE_SIZE; y++) {
			float2 pt = tile_to_cartesian((float2)((float)x / TILE_SIZE, (float)y / TILE_SIZE),
								   trx, try);
			//printf("Transformed %f %f to %f %f\n",
			//		at.x, at.y, pt.x, pt.y);
			float val = 0.0;
			float sw = 0.0;
			float best = FLT_MAX;
			for (uint i = 0; i < STATION_CNT; i++) {
				//if (!in_bounding_box(pts[i], bbox)) {
				//	continue;
				//}
				float dist = distance(pt, pts[i]);
				float w = quartic_kernel(dist, RANGE);
				if (dist < best) {
					best = dist;
				}
				sw += w;
				val += vals[i] * w;
			}
			int cid = 0;
			if (best < RANGE && sw > 0.0) {
				val /= sw;
				//printf("%f\n", val);
				float rval = (val - MIN) / MAX;
				cid = clamp((int)(rval * COLORS_LEN), 1, COLORS_LEN - 1);
			}
			out[y * TILE_SIZE + x] = cid;
		}
	}
}
