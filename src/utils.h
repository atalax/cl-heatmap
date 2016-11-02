

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define max(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })

#define min(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define UNUSED(x) ((void)(x))

#define round(x, t) ({ \
	__typeof__(x) _x = (x); \
	__typeof__(t) _t = (t); \
	((long)(_x / _t)) * _t;})

int file_read_whole(char *path, char **data, size_t *len);

#endif
