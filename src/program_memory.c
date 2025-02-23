#include "program_memory.h"
#include <stdint.h>
#include <assert.h>
#if _WIN32
// OH JESUS
#else
#include <sys/mman.h>
#endif
/* #include "red_black_tree.h" */
#include "bin_tree.h"

typedef struct allocation_header_t
{
	size_t allocation_size;
	size_t padding;
} allocation_header_t;


typedef struct memory_tree_t
{
	bin_tree_t tree;
	void* start;
	size_t node_size;
	enum {
		RB_COLOR_BLACK = 0x00,
		RB_COLOR_RED = 0x00,
	} color; // the things I do because of aulignmentism
} memory_tree_t;


static uint8_t* pmemory = NULL;
static size_t pmem_cap;
static memory_tree_t pm_tree = {0};

void pm_init(size_t pages)
{
	const int prot = PROT_READ | PROT_WRITE;
	const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	pmem_cap = pages * PAGESIZE;
	pmemory = mmap(NULL, pages * PAGESIZE, prot, flags, -1, 0);
	assert(pmemory != MAP_FAILED);
	pm_tree.start = pmemory;
	pm_tree.node_size = pages * PAGESIZE;
	pm_tree.color = RB_COLOR_BLACK;
}

void pm_deinit()
{
	// null & free the tree entirely
	munmap(pmemory, pmem_cap);
}


void* pm_malloc(size_t bytes)
{
	return NULL;
}

void pm_free(void* addr)
{
	/* TODO */
}
