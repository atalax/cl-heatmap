
#ifndef COLORMAPS_H
#define COLORMAPS_H

#include <stdint.h>

#define COLORMAP_LEN 256

struct rgba {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

typedef struct rgba rgba_t;

rgba_t colormap_heat[COLORMAP_LEN];
rgba_t colormap_grayscale[COLORMAP_LEN];

#endif
