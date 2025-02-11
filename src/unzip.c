#include "unzip.h"
#include <stdlib.h>
#include <archive.h>
#include <stdint.h>

#define MEGABYTE 1048576

typedef struct archive archive_t;
typedef struct archive_entry archive_entry_t;


// this is PRIME for an arena malloc
char* unzip(char* buf, int buf_len, uint32_t* out_len, unzip_malloc_t malloc_fn)
{
	archive_t* a = archive_read_new();
	if (archive_read_support_filter_gzip(a) == ARCHIVE_WARN)
	{
		fprintf(stderr, "WARNING: gzip format only supported through an external program!\n");
		goto bruh;
	}
	if (archive_read_open_memory(a, buf, buf_len) == ARCHIVE_FATAL)
	{
		fprintf(stderr, "ERROR: failed to read archive in memory\n");
		goto bruh;
	}
	archive_entry_t* entry = NULL;
	char* res = malloc_fn(12 * MEGABYTE); // biggest is ~10MB unarchived
	if (archive_read_next_header(a, &entry) != ARCHIVE_OK)
	{
		fprintf(stderr, "archive is NOT ok :(");
		goto bruh;
	}
	*out_len = 0;
	for (;;)
	{
		ssize_t temp = archive_read_data(a, res + *out_len, MEGABYTE);
		*out_len += temp;
		if (temp < MEGABYTE)
		{
			if (temp == ARCHIVE_FATAL || temp == ARCHIVE_WARN || temp == ARCHIVE_RETRY)
			{
				fprintf(stderr, "failure during archive_read_data()");
				archive_read_close(a);
				archive_read_free(a);
				goto bruh;
			}
			archive_read_close(a);
			archive_read_free(a);
			return res;
		}
	}
bruh:
	*out_len = 0;
	return NULL;
}
