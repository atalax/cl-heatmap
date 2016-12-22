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

struct rect {
	cl_float2 lt; // Left top
	cl_float2 rb; // Right bottom
};

static inline struct rect rect_make(cl_float2 a, cl_float2 b)
{
	struct rect ret = {
		.lt = a,
		.rb = b
	};

	if (ret.lt.x > ret.rb.x) {
		float tmp = ret.lt.x;
		ret.lt.x = ret.rb.x;
		ret.rb.x = tmp;
	}
	if (ret.lt.y > ret.rb.y) {
		float tmp = ret.lt.y;
		ret.lt.y = ret.rb.y;
		ret.rb.y = tmp;
	}

	return ret;
}

static inline float rect_left(struct rect rect)
{
	return rect.lt.x;
}

static inline float rect_right(struct rect rect)
{
	return rect.rb.x;
}

static inline float rect_top(struct rect rect)
{
	return rect.lt.y;
}

static inline float rect_bot(struct rect rect)
{
	return rect.rb.y;
}

static inline bool rect_is_inside(struct rect rect, cl_float2 pt)
{
	return pt.x <= rect_right(rect) && pt.x >= rect_left(rect) &&
			pt.y <= rect_bot(rect) && pt.y >= rect_top(rect);
}

static inline struct rect rect_inflate(struct rect rect, float by)
{
	return (struct rect){
		.lt = (cl_float2){ .x = rect.lt.x - by, .y = rect.lt.y - by },
		.rb = (cl_float2){ .x = rect.rb.x + by, .y = rect.rb.y + by }
	};
}

void init_projs();
cl_float2 wgs84_to_meters(cl_float2 wgs, projPJ proj_meters);
void generate_translation_tile(int xtile, int ytile, int zoom, cl_float4 *out, projPJ proj_meters);

static inline cl_float2 tile_to_meters(cl_float2 tile, int zoom, projPJ proj_meters)
{
	return wgs84_to_meters(tile_to_wgs84(tile, zoom), proj_meters);
}


#endif
