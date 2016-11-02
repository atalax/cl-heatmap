
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

struct rgba {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} __packed;

typedef struct rgba rgba_t;

typedef struct point {
	union {
		float x;
		float lat;
	};
	union {
		float y;
		float lng;
	};
} point_t;

struct float4 {
	union {
		float x;
		float s0;
	};
	union {
		float y;
		float s1;
	};
	union {
		float z;
		float s2;
	};
	union {
		float w;
		float s3;
	};
} __attribute__((packed));

typedef struct float4 float4_t;

#endif
