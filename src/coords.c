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

#include <stdio.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>

#include "log.h"

#include "coords.h"

static projPJ proj_wgs;

void init_projs()
{
	proj_wgs = pj_init_plus("+init=epsg:4326");
}

cl_float2 wgs84_to_meters(cl_float2 wgs, projPJ proj_meters)
{
	cl_float2 ret;
	// The X and Ys are switched intentionally
	double ry = wgs.x * DEG_TO_RAD;
	double rx = wgs.y * DEG_TO_RAD;
	int err = pj_transform(proj_wgs, proj_meters, 1, 1, &ry, &rx, NULL);
	if (err) {
		log_error("Coordinate conversion failed: %s", pj_strerrno(err));
	}
	ret.x = rx;
	ret.y = ry;
	return ret;
}

void generate_translation_tile(int xtile, int ytile, int zoom, cl_float4 *out, projPJ proj_meters)
{
	int side = 20;
	int npoints = side * side;
	gsl_multifit_linear_workspace *gwsp = gsl_multifit_linear_alloc(npoints, 3);
	gsl_vector *yx = gsl_vector_alloc(npoints);
	gsl_vector *yy = gsl_vector_alloc(npoints);
	gsl_matrix *X = gsl_matrix_alloc(npoints, 3);
	gsl_vector *cx = gsl_vector_alloc(3);
	gsl_vector *cy = gsl_vector_alloc(3);
	gsl_matrix *cov = gsl_matrix_alloc(npoints, npoints);
	double chisq;

	// Generate side x side training points
	for (int x = 0; x < side; x++) {
		for (int y = 0; y < side; y++) {
			cl_float2 tile = {.x = xtile + (double)x / side,
							  .y = ytile + (double)y / side};
			cl_float2 wgs = tile_to_wgs84(tile, zoom);
			cl_float2 mets = wgs84_to_meters(wgs, proj_meters);

			int pt = x * side + y;
			// Inputs
			gsl_matrix_set(X, pt, 0, tile.x - xtile);
			gsl_matrix_set(X, pt, 1, tile.y - ytile);
			gsl_matrix_set(X, pt, 2,	 1);

			// Outputs
			gsl_vector_set(yx, pt, mets.x);
			gsl_vector_set(yy, pt, mets.y);

		}
	}

	// Fit
	gsl_multifit_linear(X, yx, cx, cov, &chisq, gwsp);
	gsl_multifit_linear(X, yy, cy, cov, &chisq, gwsp);

	gsl_matrix *mtrans = gsl_matrix_alloc(3, 2);
	gsl_matrix_set_col(mtrans, 0, cx);
	gsl_matrix_set_col(mtrans, 1, cy);

	// Output

	out[0].x = gsl_matrix_get(mtrans, 0, 0);
	out[0].y = gsl_matrix_get(mtrans, 1, 0);
	out[0].z = gsl_matrix_get(mtrans, 2, 0);
	out[1].x = gsl_matrix_get(mtrans, 0, 1);
	out[1].y = gsl_matrix_get(mtrans, 1, 1);
	out[1].z = gsl_matrix_get(mtrans, 2, 1);

	gsl_matrix_free(mtrans);
	gsl_matrix_free(cov);
	gsl_vector_free(cy);
	gsl_vector_free(cx);
	gsl_matrix_free(X);
	gsl_vector_free(yy);
	gsl_vector_free(yx);
	gsl_multifit_linear_free(gwsp);
}
