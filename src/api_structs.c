#include "api_structs.h"
/* #include <stdlib.h> */

api_ptr_t tag_api_ptr(void* ptr, enum api_tag tag)
{
	api_ptr_t res = { .ptr = ptr };
	res.tag = tag;
	return res;
}

api_ptr_t flag_api_ptr(void* ptr,  uint8_t bitflags)
{
	api_ptr_t res = { .ptr=ptr };
	res.tag = bitflags & 0x0f;
	res.metadata = (bitflags >> 4) & 0x07;
	return res;
}

enum api_tag api_tag(const api_ptr_t api_ptr)
{
	return api_ptr.tag & 0x0f;
}

enum api_metadata api_metadata(const api_ptr_t api_ptr)
{
	return (api_ptr.metadata << 4);
}

void* api_ptr(const api_ptr_t api_ptr)
{
	return (void*)(0x01ffffffffffffff & api_ptr.bits);
}
