#ifndef SIF_FS_H
#define SIF_FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_MAX_FILES 32
#define FS_NAME_LEN  28
#define FS_MAX_SIZE  8192

void fs_init(void);
int  fs_create(const char *name, const void *data, uint32_t size);
int  fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size);
int  fs_delete(const char *name);
int  fs_list(void);
int  fs_exists(const char *name);

#endif
