#ifndef USER_API_H
#define USER_API_H


#include <stdint.h>
typedef struct token_table_t token_table_t;

const int32_t user_api(struct api_map_t* dest, const token_table_t* src);

#endif /* End USER_API_H */
