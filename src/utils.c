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

#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <bsd/string.h>
#include <string.h>

#include "utils.h"

int file_read_whole(const char *path, char **data, size_t *len)
{
	size_t llen;
	if (len == NULL) {
		len = &llen;
	}

	FILE *f = fopen(path, "rb");
	if (f == NULL)
		return -1;

	struct stat st;
	fstat(fileno(f), &st);
	*len = st.st_size;

	rewind(f);
	*data = malloc(*len);
	if (*data == NULL) {
		return -1;
	}

	fread(*data, 1, *len, f);
	fclose(f);

	return 0;
}

int mkdir_recursive(char *path, mode_t mode)
{
	char *save;
	char dir[PATH_MAX];
	bzero(dir, sizeof(dir));
	while (true) {
		char *tok = strtok_r(path, "/", &save);
		path = NULL;
		if (tok == NULL) {
			break;
		}
		strlcat(dir, tok, sizeof(dir));
		strlcat(dir, "/", sizeof(dir));
		int ret = mkdir(dir, mode);
		if (ret <= 0 && errno != EEXIST) {
			perror("Failed to mkdir!");
			return ret;
		}

	}
	return 0;
}

bool strends(const char *str, const char *suffix)
{
	int strl = strlen(str);
	int suffl = strlen(suffix);

	if (suffl > strl) {
		return false;
	}

	return strcmp(&str[strl - suffl], suffix) == 0;
}
