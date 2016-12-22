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

#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <proj_api.h>

#include "coords.h"
#include "utils.h"

#define N_PTS		1000
#define RECT_SIDE	50
#define TILE_SIZE	256


struct arguments {
	cl_float2 center;
	int zmin;
	int zmax;
	projPJ proj_meters;
};

const char *argp_program_version = "precision_bench 0.1";
const char *argp_program_bug_address = "<atx@atx.name>";
static const char argp_doc[] = "TODO";

static struct argp_option argp_opts[] = {
	{ "zoom-min",	'i',	"ZOOM_MIN",		0,	"Minimal Z", 0 },
	{ "zoom-max",	'x',	"ZOOM_MAX",		0,	"Maximal Z", 0 },
	{ "point",		'c',	"POINT",		0,	"Center point", 0 },
	{ "projection",	'p',	"PROJECTION",	0,	"Proj4 specification of the cartesian projection (default=\"+init=epsg:3045\")", 0 },
	{ NULL,			0,		NULL,			0,	NULL,		0 }
};

static void parse_opt_boundaries(char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	float flts[2];
	char *save;
	char *end;

	for (size_t i = 0; i < ARRAY_SIZE(flts); i++, arg = NULL) {
		char *tok = strtok_r(arg, ",", &save);
		if (tok == NULL) {
			argp_error(state, "Error while parsing boundary specification!");
			return;
		}
		flts[i] = strtof(tok, &end);
		if (*end != '\0') {
			argp_error(state, "Error while parsing boundary specification!");
			return;
		}
	}

	arguments->center.x = flts[0];
	arguments->center.y = flts[1];
}

static long safe_parse_long(struct argp_state *state, char *name, char *arg)
{
	char *end = NULL;
	long ret = strtol(arg, &end, 10);
	if (*end != '\0') {
		argp_error(state, "%s has to be an integer!", name);
		return 0; // We shouldn't get here
	}
	return ret;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	switch (key) {
		case 'i':
			arguments->zmin = safe_parse_long(state, "ZOOM_MIN", arg);
			break;
		case 'x':
			arguments->zmax = safe_parse_long(state, "ZOOM_MAX", arg);
			break;
		case 'b':
			parse_opt_boundaries(arg, state);
			break;
		case 'p':
			arguments->proj_meters = pj_init_plus(arg);
			if (arguments->proj_meters == NULL) {
				argp_error(state, "Failed to initialize projection: %s", pj_strerrno(pj_errno));
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { argp_opts, parse_opt, NULL, argp_doc, NULL, NULL, NULL };

static cl_float2 transform_proj(cl_float2 in, int zoom, projPJ proj)
{
	return wgs84_to_meters(tile_to_wgs84(in, zoom), proj);
}

static cl_float2 transform_linear(cl_float2 in, cl_float4 *tr)
{
	cl_float2 out;

	out.x = in.x * tr[0].x + in.y * tr[0].y + tr[0].z;
	out.y = in.x * tr[1].x + in.y * tr[1].y + tr[1].z;

	return out;
}

float randf()
{
	return (float)rand() / RAND_MAX;
}

static cl_float2 calculate_avg_and_dev(float *ft, size_t len)
{
	float avg = 0.0;
	for (size_t i = 0; i < len; i++) {
		avg += ft[i];
	}
	avg /= len;

	float var = 0.0;
	for (size_t i = 0; i < len; i++) {
		var += (ft[i] - avg) * (ft[i] - avg);
	}
	var = sqrt(var / len);

	return (cl_float2){ .x = avg, .y = var };
}

int main(int argc, char *argv[])
{
	struct arguments args = {
		// General area around prague
		.center = { .x = 50.076091, .y = 14.447708 },
		.zmin = 10,
		.zmax = 18,
		.proj_meters = NULL,
	};

	argp_parse(&argp, argc, argv, 0, 0, &args);

	init_projs();

	// We want the benchmark to be deterministic
	srand(2);

	if (args.proj_meters == NULL) {
		args.proj_meters = pj_init_plus("+init=epsg:3045");
	}

	size_t total_ntiles = RECT_SIDE * RECT_SIDE;
	size_t total_npts = total_ntiles * N_PTS;
	float xlens[total_ntiles];
	float ylens[total_ntiles];
	float *xerrs = calloc(total_npts, sizeof(float));
	float *yerrs = calloc(total_npts, sizeof(float));

	for (int zoom = args.zmin; zoom <= args.zmax; zoom++) {
		printf("zoom = %d:\n", zoom);
		// Border tiles, we want at least 10x10 chunk or more
		cl_float2 tilecenter = wgs84_to_tile(args.center, zoom);
		cl_float2 ltcorner = {
			.x = floor(tilecenter.x - RECT_SIDE / 2),
			.y = floor(tilecenter.y - RECT_SIDE / 2),
		};
		size_t ntiles = 0;
		size_t npts = 0;
		for (unsigned int x = 0; x < RECT_SIDE; x++) {
			for (unsigned int y = 0; y < RECT_SIDE; y++, ntiles++) {
				// Calculate side lengths
				unsigned int tx = ltcorner.x + x;
				unsigned int ty = ltcorner.y + y;
				cl_float2 metlt = transform_proj(
						(cl_float2){ .x = tx, .y = ty},
						zoom, args.proj_meters);
				cl_float2 metrb = transform_proj(
						(cl_float2){ .x = tx + 1.0, .y = ty + 1.0 },
						zoom, args.proj_meters);
				xlens[ntiles] = fabs(metlt.x - metrb.x);
				ylens[ntiles] = fabs(metlt.y - metrb.y);

				// Calculate transformation matrix
				cl_float4 trmat[2];
				generate_translation_tile(tx, ty, zoom, trmat, args.proj_meters);

				// Sample a bunch of random points
				for (int i = 0; i < N_PTS; i++, npts++) {
					cl_float2 pt = { .x = randf(), .y = randf() };
					cl_float2 ptf = { .x = tx + pt.x, .y = ty + pt.y };
					cl_float2 ptp = transform_proj(ptf, zoom, args.proj_meters);
					cl_float2 ptl = transform_linear(pt, trmat);

					xerrs[npts] = fabs(ptp.x - ptl.x);
					yerrs[npts] = fabs(ptp.y - ptl.y);
				}
			}
		}
		if (ntiles != total_ntiles || npts != total_npts) {
			printf("WTF?!");
		}

		printf(" avg pixel size:\n");
		cl_float2 res = calculate_avg_and_dev(xlens, ntiles);
		printf("  x = %.2fm (+- %.3f) \n", res.x / TILE_SIZE, res.y / TILE_SIZE);
		res = calculate_avg_and_dev(ylens, ntiles);
		printf("  y = %.2fm (+- %.3f) \n", res.x / TILE_SIZE, res.y / TILE_SIZE);
		printf(" avg errors:\n");
		res = calculate_avg_and_dev(xerrs, npts);
		printf("  x = %.2fm (+- %.3f)\n", res.x, res.y);
		res = calculate_avg_and_dev(yerrs, npts);
		printf("  y = %.2fm (+- %.3f)\n", res.x, res.y);
	}

	pj_free(args.proj_meters);

	free(xerrs);
	free(yerrs);
}
