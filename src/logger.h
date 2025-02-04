#ifndef LOGGER_H
#define LOGGER_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#define LOG(M, ...) if (log_fp) fprintf(log_fp, "[tarragon] " M "\n", ##__VA_ARGS__);

#endif /* End LOGGER_H */
