
#include "common.h"

__kernel void generate_tile(global float4 *ptin,
							global float2 *stats,
							global float *tdoas,
							global uchar *out_)
{
	uint cx = get_global_id(0);
	uint cy = get_global_id(1);
	uint off = (cy * CHUNK_SIZE + cx) * TILE_SIZE * TILE_SIZE;
	global uchar *out = &out_[off];
	uint off2 = 2 * (cy * CHUNK_SIZE + cx);
	float4 trx = ptin[off2 + 0];
	float4 try = ptin[off2 + 1];
	for (uint x = 0; x < TILE_SIZE; x++) {
		for (uint y = 0; y < TILE_SIZE; y++) {
			float2 pt = tile_to_cartesian(
								(float2)((float)x / TILE_SIZE,
										 (float)y / TILE_SIZE),
								trx, try);
			//printf("Transformed %f %f to %f %f\n",
			//		at.x, at.y, pt.x, pt.y);
			float dist = distance(pt, stats[0]);
			float err = 0;
			for (uint i = 1; i < STATION_CNT - 0; i++) {
				float dist2 = distance(pt, stats[i]);
				float ad = (dist - dist2) - tdoas[i];
				ad = ad * ad;
				err += ad;
			}
			err = sqrt(err);
			err /= SCALE_BY;
			int cid = COLORS_LEN - clamp((int)(err * COLORS_LEN), 0, COLORS_LEN - 1) - 1;
			out[y * TILE_SIZE + x] = cid;
		}
	}
}
