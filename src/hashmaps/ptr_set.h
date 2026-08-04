#ifndef PTR_SEt_H
#define PTR_SEt_H

#include <stdint.h>

struct ptr_set
{
	void** pointers;
	uint64_t* bits;
	int32_t cap;
	int32_t size;
};

const int32_t ptr_hash(const void* ptr, const int32_t cap);
void ptr_set_init(struct ptr_set* set, uint8_t* allocation, const int32_t cap);
void ptr_set_clear(struct ptr_set* set);
void ptr_set_add(struct ptr_set* set, void* ptr);
uint64_t ptr_set_has(struct ptr_set* set, const void* ptr);

#endif /* End PTR_SEt_H */
