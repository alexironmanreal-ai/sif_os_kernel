#include "vfs.h"
#include "fs.h"
#include "string.h"
#include "printk.h"
static struct { int used; char name[VFS_PATH_LEN]; uint32_t offset; int writable; } fds[VFS_MAX_FD];
void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_FD; i++) fds[i].used = 0;
    printk("[vfs] fd table\n");
}
int vfs_open(const char *path, int write) {
    if (!path || !path[0]) return -1;
    if (!write && !fs_exists(path)) return -1;
    if (write && !fs_exists(path) && fs_create(path, "", 0) < 0) return -1;
    int slot = -1;
    for (int i = 0; i < VFS_MAX_FD; i++) if (!fds[i].used) { slot = i; break; }
    if (slot < 0) return -1;
    strncpy(fds[slot].name, path, VFS_PATH_LEN - 1);
    fds[slot].name[VFS_PATH_LEN - 1] = 0;
    fds[slot].offset = 0; fds[slot].writable = write ? 1 : 0; fds[slot].used = 1;
    return slot;
}
int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used) return -1;
    fds[fd].used = 0; return 0;
}
int vfs_read(int fd, void *buf, uint32_t n) {
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used) return -1;
    uint8_t tmp[512]; uint32_t sz = 0;
    if (fs_read(fds[fd].name, tmp, sizeof(tmp), &sz) < 0) return -1;
    if (fds[fd].offset >= sz) return 0;
    uint32_t avail = sz - fds[fd].offset;
    if (n > avail) n = avail;
    memcpy(buf, tmp + fds[fd].offset, n);
    fds[fd].offset += n;
    return (int)n;
}
int vfs_write(int fd, const void *buf, uint32_t n) {
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used || !fds[fd].writable) return -1;
    uint8_t old[4096]; uint32_t osz = 0;
    fs_read(fds[fd].name, old, sizeof(old), &osz);
    if (osz + n > sizeof(old)) n = sizeof(old) - osz;
    memcpy(old + osz, buf, n);
    if (fs_write(fds[fd].name, old, osz + n) < 0) return -1;
    fds[fd].offset = osz + n;
    return (int)n;
}
int vfs_is_open(int fd) { return fd >= 0 && fd < VFS_MAX_FD && fds[fd].used; }
