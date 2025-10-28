#ifndef GODOT_API_H
#define GODOT_API_H

#include <stdint.h>
#include "relative_ptrs.h"
#include "api_structs.h"

const fetch_res_t api_fetch(char* key, int16_t len);

void print_supported_version(void);


#endif /* End GODOT_API_H */
