#ifndef GODOT_API_H
#define GODOT_API_H

#include <stdint.h>
#include "relative_ptrs.h"
#include "api_structs.h"

const api_ptr_t godot_api_fetch(const string* key);

void debug_key(const string* key);

void print_supported_version(void);


#endif /* End GODOT_API_H */
