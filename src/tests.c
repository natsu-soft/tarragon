#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "file_sys.h"


int run_tests(void)
{
	int temp = 0;
	char* str_file_abs_path = fs_abs_path( "test_stuff/str_file.txt");
	assertf(!strcmp(str_file(fs_abs_path( "test_stuff/str_file.txt"), &temp), "hello!\n"), "str_file test failed"); // have to ! because strcmp returns 0 (false) if equal
	free(str_file_abs_path);
	// load config file
	return 0;
}
