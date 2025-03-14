#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#ifndef EXTERN_LOG_FILE_POINTERR
extern FILE* log_fp;
#define LOG_FILE
#endif


#define PLAIN_LOG(M, ...) if (log_fp) fprintf(log_fp, M, ##__VA_ARGS__);
#define LOG(M, ...) if (log_fp) fprintf(log_fp, "[tarragon] " M "\n", ##__VA_ARGS__); fflush(log_fp)
#define ERROR(M, ...) if (log_fp) fprintf(log_fp, "[tarragon -- ERROR] " M "\n", ##__VA_ARGS__); fflush(log_fp)

#define ENDLOG if(log_fp) fclose(log_fp);

#endif /* End LOGGER_H */
