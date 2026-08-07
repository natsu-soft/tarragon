#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef LOG_FILE_HERE
FILE* log_fp;
#else
extern FILE* log_fp;
#endif

#define PLAIN_LOG(M, ...) if (log_fp) fprintf(log_fp, M "\n", ##__VA_ARGS__); fflush(log_fp)
#define LOG(M, ...) if (log_fp) fprintf(log_fp, "[tarragon] " M "\n", ##__VA_ARGS__); fflush(log_fp)
#define ERROR(M, ...) if (log_fp) fprintf(log_fp, "[ERROR] " M "\n", ##__VA_ARGS__); fflush(log_fp)

#define ENDLOG if(log_fp) fclose(log_fp)

#endif /* End LOGGER_H */
