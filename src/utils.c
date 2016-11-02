
#include <stdlib.h>
#include <sys/stat.h>

#include "utils.h"

int file_read_whole(char *path, char **data, size_t *len)
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
