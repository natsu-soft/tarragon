#ifndef ERROR_MACROS_H
#define ERROR_MACROS_H

/*************/
/* STOLEN AF */
/*************/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) { log_error(M, ##__VA_ARGS__); assert(A); }

#define flog_error(F, M, ...) fprintf(F, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define fassertf(F, A, M, ...) if(!(A)) { flog_error(F, M, ##__VA_ARGS__); fflush(F); assert(A); }

#endif /* End ERROR_MACROS_H */
