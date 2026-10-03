#ifndef SIF_VFS_H
#define SIF_VFS_H
#include <stdint.h>
#include <stddef.h>
#define VFS_MAX_FD 16
#define VFS_PATH_LEN 32
void vfs_init(void);
int vfs_open(const char *path, int write);
int vfs_close(int fd);
int vfs_read(int fd, void *buf, uint32_t n);
int vfs_write(int fd, const void *buf, uint32_t n);
int vfs_is_open(int fd);
#endif
