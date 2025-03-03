/* #include "unzip.h" */
/* #include <stdlib.h> */
/* #include <archive.h> */
/* #include <stdint.h> */
/* #include <stdio.h> */
/* #include <assert.h> */

/* #define MEGABYTE 1048576 */

/* /\* typedef struct archive archive_t; *\/ */
/* /\* typedef struct archive_entry archive_entry_t; *\/ */

/* // this is PRIME for an arena malloc */
/* char* unzip(char* buf, size_t buf_len, uint32_t* out_len, unzip_malloc_t malloc_fn) */
/* { */
/* 	struct archive* a = archive_read_new(); */
/* 	if (archive_read_support_format_raw(a) == ARCHIVE_FATAL) */
/* 	{ */
/* 		fprintf(stderr, "ERROR: failed to support raw format\n"); */
/* 		goto bruh; */
/* 	} */
/* 	else if (archive_read_support_filter_gzip(a) == ARCHIVE_WARN) */
/* 	{ */
/* 		fprintf(stderr, "WARNING: gzip format only supported through an external program!\n"); */
/* 		goto bruh; */
/* 	} */
/* 	if (archive_read_open_memory(a, buf, buf_len) != ARCHIVE_OK) */
/* 	{ */
/* 		fprintf(stderr, "ERROR: failed to read archive in memory\n"); */
/* 		goto bruh; */
/* 	} */
/* 	struct archive_entry* entry = NULL; */
/* 	char* res = malloc_fn(12 * MEGABYTE); // biggest is ~10MB unarchived */
/* 	if (archive_read_next_header(a, &entry) != ARCHIVE_OK) */
/* 	{ */
/* 		fprintf(stderr, "archive is NOT ok :(\n"); */
/* 		goto bruh; */
/* 	} */
/* 	*out_len = 0; */
/* 	for (;;) */
/* 	{ */
/* 		ssize_t temp = archive_read_data(a, res + *out_len, MEGABYTE); */
/* 		*out_len += temp; */
/* 		if (temp < MEGABYTE) */
/* 		{ */
/* 			if (temp == ARCHIVE_FATAL || temp == ARCHIVE_WARN || temp == ARCHIVE_RETRY) */
/* 			{ */
/* 				fprintf(stderr, "failure during archive_read_data()"); */
/* 				archive_read_close(a); */
/* 				archive_read_free(a); */
/* 				goto bruh; */
/* 			} */
/* 			archive_read_close(a); */
/* 			archive_read_free(a); */
/* 			return res; */
/* 		} */
/* 	} */
/* bruh: */
/* 	*out_len = 0; */
/* 	return NULL; */
/* } */

/* // literally just here for testing because I was LOSING my mind */
/* char* unzip_file(FILE* file, uint32_t* out_len, unzip_malloc_t malloc_fn) */
/* { */
/* 	char* res = NULL; */
/* 	struct archive* a = archive_read_new(); */
/* 	/\* archive_read_support_format_tar(a); *\/ */
/* 	archive_read_support_format_raw(a); */
/* 	archive_read_support_filter_gzip(a); */
/* 	/\* archive_read_support_filter_all(a); *\/ */
/* 	if (archive_read_open_FILE(a, file) != ARCHIVE_OK) */
/* 	{ */
/* 		fprintf(stderr, "[ERR] ITS SO OVER"); */
/* 		goto bruh; */
/* 	} */
/* 	archive_read_close(a); */
/* 	archive_read_free(a); */
/* bruh: */
/* 	*out_len = 0; */
/* 	return res; */
/* } */

/* extern unsigned char utility_functions_gz[]; */
/* extern unsigned int utility_functions_gz_len; */
/* int unzip_test(int argc, char** argv) */
/* { */
/* 	printf("testing unzip functions\n"); */
/* 	FILE* fp = fopen("file_sys.gz", "r"); */
/* 	uint32_t len; */
/* 	char* file_sys_str = unzip_file(fp, &len, malloc); */
/* 	fclose(fp); */

/* 	/\* printf("file_sys.c:\n%s\n", file_sys_str); *\/ */
/* 	/\* free(file_sys_str); *\/ */
/* 	/\* int zip_len; *\/ */
/* 	/\* char* zip_str = fs_filestr(fs_abs_path("file_sys.gz"), &zip_len); *\/ */
/* 	uint32_t bruh_len; */
/* 	char* bruh = unzip(&utility_functions_gz[0], utility_functions_gz_len, &bruh_len, malloc); */
/* 	/\* printf("unzip_res: %s\n", bruh); *\/ */
/* 	printf("archived_len: %d, full_len: %d, compression_ratio: %f\n", utility_functions_gz_len, bruh_len, 1.0f - (float)utility_functions_gz_len / (float)bruh_len); */
/* 	free(file_sys_str); */
/* 	free(bruh); */
/* 	return 0; */
/* } */
