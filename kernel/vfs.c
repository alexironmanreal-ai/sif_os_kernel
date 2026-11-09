#include "vfs.h"
#include "fs.h"
#include "diskfs.h"
#include "string.h"
#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "serial.h"
#include "task.h"
#include "uaccess.h"

static struct proc_fd kfds[VFS_MAX_FD];

static struct proc_fd *fd_table(void) {
    struct task *t = task_current();
    if (t && t->is_user)
        return t->fds;
    return kfds;
}

/* unified lookup: prefer disk if mounted and present, else ramfs */
static int resolve_read(const char *path, void *buf, uint32_t max, uint32_t *out,
                        int *from_disk) {
    if (diskfs_mounted() && diskfs_exists(path)) {
        if (diskfs_read(path, buf, max, out) == 0) {
            if (from_disk)
                *from_disk = 1;
            return 0;
        }
    }
    if (fs_exists(path)) {
        if (fs_read(path, buf, max, out) == 0) {
            if (from_disk)
                *from_disk = 0;
            return 0;
        }
    }
    return -1;
}

static int resolve_exists(const char *path) {
    if (diskfs_mounted() && diskfs_exists(path))
        return 1;
    return fs_exists(path);
}

void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_FD; i++) {
        kfds[i].used = 0;
        kfds[i].name[0] = 0;
        kfds[i].offset = 0;
        kfds[i].on_disk = 0;
    }
    kfds[0].used = 1;
    kfds[0].kind = FD_SPECIAL;
    kfds[0].special_id = 0;
    kfds[0].writable = 0;
    kfds[1].used = 1;
    kfds[1].kind = FD_SPECIAL;
    kfds[1].special_id = 1;
    kfds[1].writable = 1;
    kfds[2].used = 1;
    kfds[2].kind = FD_SPECIAL;
    kfds[2].special_id = 2;
    kfds[2].writable = 1;
    printk("[vfs] unified ramfs+diskfs\n");
}

int vfs_open(const char *path, int write) {
    if (!path_ok(path))
        return -1;

    if (!write && !resolve_exists(path))
        return -1;

    int on_disk = 0;
    if (write) {
        /* prefer writing to disk if mounted */
        if (diskfs_mounted()) {
            if (!diskfs_exists(path) && diskfs_create(path, "", 0) < 0) {
                if (!fs_exists(path) && fs_create(path, "", 0) < 0)
                    return -1;
                on_disk = 0;
            } else
                on_disk = 1;
        } else if (!fs_exists(path) && fs_create(path, "", 0) < 0)
            return -1;
    } else {
        on_disk = (diskfs_mounted() && diskfs_exists(path)) ? 1 : 0;
    }

    struct proc_fd *fds = fd_table();
    int slot = -1;
    for (int i = 3; i < VFS_MAX_FD; i++) {
        if (!fds[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return -1;

    strncpy(fds[slot].name, path, PROC_PATH_LEN - 1);
    fds[slot].name[PROC_PATH_LEN - 1] = 0;
    fds[slot].offset = 0;
    fds[slot].writable = write ? 1 : 0;
    fds[slot].kind = FD_FILE;
    fds[slot].special_id = 0;
    fds[slot].on_disk = on_disk;
    fds[slot].used = 1;
    return slot;
}

int vfs_close(int fd) {
    struct proc_fd *fds = fd_table();
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used)
        return -1;
    if (fd <= 2)
        return 0;
    fds[fd].used = 0;
    fds[fd].name[0] = 0;
    fds[fd].offset = 0;
    fds[fd].writable = 0;
    fds[fd].on_disk = 0;
    return 0;
}

int vfs_read(int fd, void *buf, uint32_t n) {
    struct proc_fd *fds = fd_table();
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used || !buf || !n)
        return -1;
    if (n > 4096)
        n = 4096;

    if (fds[fd].kind == FD_SPECIAL && fds[fd].special_id == 0) {
        uint8_t *out = (uint8_t *)buf;
        uint32_t got = 0;
        while (got < n) {
            while (!keyboard_has_input())
                __asm__ volatile("hlt");
            int ch = keyboard_read_char();
            if (!ch)
                continue;
            out[got++] = (uint8_t)ch;
            if (ch == '\n' || ch == '\r')
                break;
        }
        return (int)got;
    }

    if (fds[fd].kind != FD_FILE)
        return -1;

    uint8_t tmp[4096];
    uint32_t sz = 0;
    int from_disk = 0;
    if (resolve_read(fds[fd].name, tmp, sizeof(tmp), &sz, &from_disk) < 0)
        return -1;
    (void)from_disk;
    if (fds[fd].offset >= sz)
        return 0;
    uint32_t avail = sz - fds[fd].offset;
    if (n > avail)
        n = avail;
    memcpy(buf, tmp + fds[fd].offset, n);
    fds[fd].offset += n;
    return (int)n;
}

int vfs_write(int fd, const void *buf, uint32_t n) {
    struct proc_fd *fds = fd_table();
    if (fd < 0 || fd >= VFS_MAX_FD || !fds[fd].used || !buf)
        return -1;
    if (n > 4096)
        n = 4096;

    if (fds[fd].kind == FD_SPECIAL) {
        if (!fds[fd].writable)
            return -1;
        if (fds[fd].special_id == 1 || fds[fd].special_id == 2) {
            const char *p = (const char *)buf;
            for (uint32_t i = 0; i < n; i++) {
                vga_putc(p[i]);
                serial_putc(p[i]);
            }
            return (int)n;
        }
        return -1;
    }

    if (!fds[fd].writable)
        return -1;

    uint8_t old[4096];
    uint32_t osz = 0;
    resolve_read(fds[fd].name, old, sizeof(old), &osz, 0);
    if (osz >= sizeof(old))
        return -1;
    if (osz + n > sizeof(old))
        n = sizeof(old) - osz;
    memcpy(old + osz, buf, n);
    uint32_t total = osz + n;

    if (fds[fd].on_disk && diskfs_mounted()) {
        if (diskfs_write(fds[fd].name, old, total) < 0)
            return -1;
    } else {
        if (fs_write(fds[fd].name, old, total) < 0)
            return -1;
    }
    fds[fd].offset = total;
    return (int)n;
}

int vfs_is_open(int fd) {
    struct proc_fd *fds = fd_table();
    return fd >= 0 && fd < VFS_MAX_FD && fds[fd].used;
}
