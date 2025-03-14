#include <unistd.h>
#include <assert.h>
#include "logger.h"


#define UNIT_TESTS \
	TEST(file_sys) \
	TEST(mem_sys) \
	TEST(base_protocol) \
	TEST(json) \
	TEST(trie)

#define TEST(A) extern int A##_test(int argc, char** argv);
UNIT_TESTS
#undef TEST

int main(int argc, char** argv)
{
	int c;
	log_fp = NULL;
	while ((c = getopt(argc, argv, "l:")) != -1)
	{
		switch (c)
		{
		case 'l':
			log_fp = fopen(optarg , "w");
			LOG("opened tarragon.log for logging");
			break;
		}
	}
	#define TEST(A) assert(A##_test(argc, argv) == 0); printf("A-OK: " #A ".c\n");
	UNIT_TESTS;
	#undef TEST
	ENDLOG;
	return 0;
}
