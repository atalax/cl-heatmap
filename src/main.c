
#include <argp.h>
#include <limits.h>
#include <math.h>
#include <libgen.h>
#include <proj_api.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <CL/cl.h>
#include <libpng16/png.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include <json-c/json.h>

#include "types.h"
#include "colormaps.h"
#include "coords.h"
#include "utils.h"

#define MAX_SOURCE_SIZE 100000
#define OCCHECK(x) if ((x) != CL_SUCCESS) { fprintf(stderr, "OCL Error! %d @%d\n", x, __LINE__); exit(EXIT_FAILURE); }
#define TILE_SIZE 256
#define CHUNK_SIZE 4

void write_png(char *fname, int width, int height, uint8_t *img, rgba_t *colormap)
{
	FILE *fout = fopen(fname, "w");
	if (fout == NULL) {
		perror("Failed to write a file\n");
		return;
	}

	// TODO: Error handling!!!
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop png_info = png_create_info_struct(png_ptr);

	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		return;
	}

	png_init_io(png_ptr, fout);

	png_colorp colors = calloc(COLORMAP_LEN, sizeof(png_color));
	png_bytep trns = calloc(COLORMAP_LEN, sizeof(png_color));
	for (unsigned i = 0; i < COLORMAP_LEN; i++) {
		colors[i].red = colormap[i].r;
		colors[i].green = colormap[i].g;
		colors[i].blue = colormap[i].b;
		trns[i] = colormap[i].a;
	}

	png_set_PLTE(png_ptr, png_info, colors, COLORMAP_LEN);
	png_set_tRNS(png_ptr, png_info, trns, COLORMAP_LEN, NULL);
	png_set_IHDR(png_ptr, png_info, width, height, 8, PNG_COLOR_TYPE_PALETTE,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, png_info);

	free(colors);
	free(trns);

	// TODO: We actually want pallete colors
	png_bytep rowp = malloc(1 * width * sizeof(png_byte));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			rowp[x] = img[y * height + x];
		}
		png_write_row(png_ptr, rowp);
	}
	free(rowp);

	png_write_end(png_ptr, NULL);

	fclose(fout);
	png_free_data(png_ptr, png_info, PNG_FREE_ALL, -1);
	free(png_info);
	png_destroy_write_struct(&png_ptr, NULL);
}

void print_gsl_matrix(gsl_matrix *mat)
{
	printf("[\n");
	for (unsigned int i = 0; i < mat->size1; i++) {
		printf("  [ ");
		for (unsigned int j = 0; j < mat->size2; j++) {
			printf("%.5f\t", gsl_matrix_get(mat, i, j));
		}
		printf("]\n");
	}
	printf("]\n");
}

void generate_translation_tile(int xtile, int ytile, int zoom, float4_t *out)
{
	int npoints = 9;
	gsl_multifit_linear_workspace *gwsp = gsl_multifit_linear_alloc(npoints, 3);
	gsl_vector *yx = gsl_vector_alloc(npoints);
	gsl_vector *yy = gsl_vector_alloc(npoints);
	gsl_matrix *X = gsl_matrix_alloc(npoints, 3);
	gsl_vector *cx = gsl_vector_alloc(3);
	gsl_vector *cy = gsl_vector_alloc(3);
	gsl_matrix *cov = gsl_matrix_alloc(npoints, npoints);
	double chisq;

	// Generate 3x3 training points
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			point_t tile = {.x = xtile + (double)x / 2,
							.y = ytile + (double)y / 2};
			point_t wgs = tile_to_wgs84(tile, zoom);
			point_t mets = wgs84_to_meters(wgs);

			int pt = x * 3 + y;
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

struct arguments {
	int zoomlevel;
	char *kernel;
	char *jspath;
	char *outdir;
	char *clargs;
	point_t northwest;
	point_t southeast;
	rgba_t *colormap;
};

const char *argp_program_version = "cl-heatmap 1.0";
const char *argp_program_bug_address = "<atx@atx.name>";
static const char argp_doc[] = "TODO";

static struct argp_option argp_opts[] = {
	{ "-z",		'z',	"ZOOM",			0,	"Zoomlevel", 0 },
	{ "-k",		'k',	"KERNEL",		0,	"Kernel to use", 0 },
	{ "-o",		'o',	"OUTDIR",		0,	"Output directory", 0 },
	{ "-i",		'i',	"INPUT",		0,	"Input JSON", 0 },
	{ "-c",		'c',	"CLARGS",		0,	"OpenCL compiler arguments", 0 },
	{ "-m",		'm',	"COLORMAP",		0,	"Colormap to use, available: [\"heat\"]", 0 },
	{ "-b",		'b',	"BOUNDARIES",	0,	"Boundaries in WGS84 '50.12,14.23,51.23,15.33'", 0 },
	{ NULL,		0,		NULL,			0,	NULL, 0 }
};

static void parse_opt_boundaries(char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	float flts[4];
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

	arguments->northwest.lat = flts[0];
	arguments->northwest.lng = flts[1];
	arguments->southeast.lat = flts[2];
	arguments->southeast.lng = flts[3];
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	char *end = NULL;
	switch (key) {
		case 'z':
			arguments->zoomlevel = strtol(arg, &end, 10);
			if (*end != '\0') {
				argp_error(state, "ZOOM has to be an integer!");
			}
			break;
		case 'k':
			arguments->kernel = arg;
			break;
		case 'o':
			arguments->outdir = arg;
			break;
		case 'i':
			arguments->jspath = arg;
			break;
		case 'c':
			arguments->clargs = arg;
			break;
		case 'm':
			if (!strcmp(arg, "heat")) {
				arguments->colormap = colormap_heat;
			} else if (!strcmp(arg, "grayscale")) {
				arguments->colormap = colormap_grayscale;
			} else {
				argp_error(state, "Unknown colormap specified!");
			}
			break;
		case 'b':
			parse_opt_boundaries(arg, state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { argp_opts, parse_opt, NULL, argp_doc, NULL, NULL, NULL };


int main(int argc, char *argv[])
{
	struct arguments args = {
		.zoomlevel = 12,
		.kernel = NULL,
		.jspath = "./input.json",
		.outdir = "./cache",
		.clargs = "",
		.northwest = { .lat = 50.17, .lng = 14.24 },
		.southeast = { .lat = 49.95, .lng = 14.70 },
		.colormap = colormap_heat,
	};

	argp_parse(&argp, argc, argv, 0, 0, &args);

	if (args.kernel == NULL) {
		fprintf(stderr, "No kernel specified. Select on from the kernels/ directory!\n");
		return EXIT_FAILURE;
	}

	init_projs();

	// Parse the input JSON
	char *jsonstr;
	if (file_read_whole(args.jspath, &jsonstr, NULL)) {
		perror("failed to read the input JSON file");
		return EXIT_FAILURE;
	}
	struct json_object *jroot = json_tokener_parse(jsonstr);
	struct json_object *jpts;
	if (!json_object_object_get_ex(jroot, "points", &jpts)) {
		fprintf(stderr, "key \"points\" not found!\n");
		return EXIT_FAILURE;
	}
	size_t datalen = json_object_array_length(jpts);
	point_t *datapts = calloc(datalen, sizeof(point_t));
	float *datavals = calloc(datalen, sizeof(float));
	for (unsigned int i = 0; i < datalen; i++) {
		json_object *jpt = json_object_array_get_idx(jpts, i);
		json_object *jloc;
		json_object_object_get_ex(jpt, "loc", &jloc);
		json_object *jval;
		json_object_object_get_ex(jpt, "val", &jval);
		json_object *jlat = json_object_array_get_idx(jloc, 0);
		json_object *jlng = json_object_array_get_idx(jloc, 1);
		datapts[i].lat = json_object_get_double(jlat);
		datapts[i].lng = json_object_get_double(jlng);
		datavals[i] = json_object_get_double(jval);
	}

	for (unsigned int i = 0; i < datalen; i++) {
		datapts[i] = wgs84_to_meters(datapts[i]);
	}

	// Render tiles
	point_t tilelt = round_point(wgs84_to_tile(args.northwest, args.zoomlevel),
								CHUNK_SIZE, false);
	point_t tilerb = round_point(wgs84_to_tile(args.southeast, args.zoomlevel),
								CHUNK_SIZE, true);
	printf("Rendering tiles from %dx%d to %dx%d on zoomlevel %d\n",
			(int)tilelt.x, (int)tilelt.y,
			(int)tilerb.x, (int)tilerb.y,
			args.zoomlevel);


	char zpath[PATH_MAX];
	snprintf(zpath, sizeof(zpath), "%s/%d", args.outdir, args.zoomlevel);
	mkdir(zpath, 0755);

	printf("Generating coordinate translation cache...\n");
	// TODO: Multithread this (or ProjCL this)
	for (unsigned int x = tilelt.x; x <= tilerb.x; x++) {
		char xpath[PATH_MAX];
		snprintf(xpath, sizeof(xpath), "%s/%d", zpath, x);
		mkdir(xpath, 0755);
		for (unsigned int y = tilelt.y; y <= tilerb.y; y++) {
			char tpath[PATH_MAX];
			snprintf(tpath, sizeof(tpath), "%s/%d.map", xpath, y);
			if (!access(tpath, F_OK)) {
				continue;
			}
			float4_t out[2];
			generate_translation_tile(x, y, args.zoomlevel, out);
			FILE *fout = fopen(tpath, "w");
			if (fout == NULL) {
				perror("failed to write cache file\n");
				return EXIT_FAILURE;
			}
			printf("generated %s\n", tpath);
			fwrite(out, sizeof(out[0]), ARRAY_SIZE(out), fout);
			fclose(fout);
		}
	}

	printf("OCL time!\n");

	/* TODO: Add proper platform/device selection */
	cl_platform_id pids[10];
	cl_uint cnt;
	clGetPlatformIDs(ARRAY_SIZE(pids), pids, &cnt);
	if (cnt < 1) {
		exit(EXIT_FAILURE);
	}
	cl_device_id dids[10];
	clGetDeviceIDs(pids[0], CL_DEVICE_TYPE_ALL, ARRAY_SIZE(dids), dids, &cnt);
	if (cnt < 1) {
		exit(EXIT_FAILURE);
	}
	cl_device_id *devid = &dids[0];

	cl_int ret;
	cl_context clctx = clCreateContext(NULL, 1, devid, NULL, NULL, &ret);
	OCCHECK(ret);
	cl_command_queue clque = clCreateCommandQueue(clctx, *devid, 0, &ret);
	OCCHECK(ret);

	// Build the kernel
	FILE *fsrc = fopen(args.kernel, "r");
	char *clsrc = malloc(MAX_SOURCE_SIZE);
	size_t len = fread(clsrc, 1, MAX_SOURCE_SIZE, fsrc);
	clsrc[len] = '\0';

	char kpath[PATH_MAX];
	strncpy(kpath, args.kernel, sizeof(kpath));
	char *kdir = dirname(kpath);
	char compargs[1000];
	bzero(compargs, sizeof(compargs));
	snprintf(compargs, ARRAY_SIZE(compargs),
			"-I%s -DCOLORS_LEN=%d -DTILE_SIZE=%d -DCHUNK_SIZE=%d -DSTATION_CNT=%lu %s",
			kdir, COLORMAP_LEN, TILE_SIZE, CHUNK_SIZE, datalen, args.clargs);

	cl_program clprg = clCreateProgramWithSource(clctx, 1, (const char **)&clsrc, &len, &ret);
	OCCHECK(ret);
	ret = clBuildProgram(clprg, 1, devid, compargs, NULL, NULL);
	if (ret != CL_SUCCESS) {
		printf("Kernel build failed:\n");
		size_t len;
		clGetProgramBuildInfo(clprg, *devid, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
		char msg[len];
		clGetProgramBuildInfo(clprg, *devid, CL_PROGRAM_BUILD_LOG, len, msg, NULL);
		printf("%s\n", msg);
		return EXIT_FAILURE;
	}
	cl_kernel clkrn = clCreateKernel(clprg, "generate_tile", &ret);
	OCCHECK(ret);

	// Load the data
	size_t chunklen = TILE_SIZE * TILE_SIZE * CHUNK_SIZE * CHUNK_SIZE;
	size_t ptinlen = 2 * CHUNK_SIZE * CHUNK_SIZE;
	float4_t *ptin = calloc(ptinlen, sizeof(float4_t));
	uint8_t *out = calloc(chunklen, sizeof(rgba_t));

	cl_mem stats_cl = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									 datalen * sizeof(point_t),
									 datapts, &ret);
	OCCHECK(ret);
	cl_mem tdoas_cl = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									 datalen * sizeof(float),
									 datavals, &ret);
	OCCHECK(ret);

	cl_mem ptin_cl = clCreateBuffer(clctx, CL_MEM_READ_ONLY,
									ptinlen * sizeof(ptin[0]), NULL, &ret);
	OCCHECK(ret);
	cl_mem out_cl = clCreateBuffer(clctx, CL_MEM_WRITE_ONLY,
								   chunklen * sizeof(out[0]), NULL, &ret);
	OCCHECK(ret);

	// Generate the tiles
	for (unsigned int cx = tilelt.x; cx < tilerb.x; cx += CHUNK_SIZE) {
		for (unsigned int cy = tilelt.y; cy < tilerb.y; cy += CHUNK_SIZE) {
			printf("Generating chunk %dx%d +%d\n", cx, cy, CHUNK_SIZE);
			// Load the coordinate mappings
			for (unsigned int x = 0; x < CHUNK_SIZE; x++) {
				for (unsigned int y = 0; y < CHUNK_SIZE; y++) {
					char cpath[PATH_MAX];
					snprintf(cpath, sizeof(cpath),
							"%s/%d/%d/%d.map", args.outdir, args.zoomlevel, x + cx, y + cy);
					FILE *file = fopen(cpath, "r");
					fread(&ptin[2 * (y * CHUNK_SIZE + x)],
							sizeof(ptin[0]), 2, file);
					fclose(file);
				}
			}
			clEnqueueWriteBuffer(clque, ptin_cl, CL_TRUE, 0, ptinlen * sizeof(ptin[0]),
								 ptin, 0, NULL, NULL);
			ret = clSetKernelArg(clkrn, 0, sizeof(ptin_cl), &ptin_cl);
			ret = clSetKernelArg(clkrn, 1, sizeof(stats_cl), &stats_cl);
			ret = clSetKernelArg(clkrn, 2, sizeof(tdoas_cl), &tdoas_cl);
			ret = clSetKernelArg(clkrn, 3, sizeof(out_cl), &out_cl);
			size_t global_work_size[] = { CHUNK_SIZE, CHUNK_SIZE };
			size_t local_work_size[] = { 1, 1 };
			ret = clEnqueueNDRangeKernel(clque, clkrn, 2, NULL,
										 global_work_size, local_work_size,
										 0, NULL, NULL);
			OCCHECK(ret);
			clFinish(clque);

			clEnqueueReadBuffer(clque, out_cl, CL_TRUE, 0, chunklen * sizeof(out[0]),
								out, 0, NULL, NULL);
			// Write the files
			// TODO: Move this into a separate thread
			for (unsigned int x = cx; x < cx + CHUNK_SIZE; x++) {
				for (unsigned int y = cy; y < cy + CHUNK_SIZE; y++) {
					char cpath[PATH_MAX];
					snprintf(cpath, sizeof(cpath),
							"%s/%d/%d/%d.png", args.outdir, args.zoomlevel, x, y);
					write_png(cpath, TILE_SIZE, TILE_SIZE,
							  &out[((y - cy) * CHUNK_SIZE + (x - cx)) * TILE_SIZE * TILE_SIZE],
							  args.colormap);
					printf("wrote %s\n", cpath);
				}
			}
		}
	}

	ret = clFlush(clque);
	ret = clFinish(clque);
	ret = clReleaseKernel(clkrn);
	ret = clReleaseProgram(clprg);
	ret = clReleaseMemObject(stats_cl);
	ret = clReleaseMemObject(tdoas_cl);
	ret = clReleaseMemObject(out_cl);
	ret = clReleaseContext(clctx);

	free(out);
	free(ptin);
	free(clsrc);
}