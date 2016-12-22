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
