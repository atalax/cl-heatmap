/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Josef Gajdusek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * */

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
