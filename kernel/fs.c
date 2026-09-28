#include "fs.h"
#include "kmalloc.h"
#include "printk.h"
#include "string.h"

static struct {
    char     name[FS_NAME_LEN];
    uint32_t size;
    uint8_t *data;
    int      used;
} files[FS_MAX_FILES];

void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = 0;
        files[i].data = 0;
        files[i].size = 0;
        files[i].name[0] = 0;
    }
    fs_create("readme.txt",
              "SIF Kernel ramfs v0.6\n"
              "Comandos: ls cat write rm mem heap ps help\n",
              70);
    fs_create("hello.txt", "hola desde el filesystem\n", 26);
    fs_create("version", "SIF Kernel 0.6.0\n", 17);
    printk("[fs] ramfs listo (%d slots)\n", FS_MAX_FILES);
}

int fs_create(const char *name, const void *data, uint32_t size) {
    if (!name || !*name || size > FS_MAX_SIZE) return -1;
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            slot = i;
            break;
        }
        if (!files[i].used && slot < 0) slot = i;
    }
    if (slot < 0) return -1;

    uint8_t *buf = (uint8_t *)kmalloc(size ? size : 1);
    if (!buf) return -1;
    if (size) memcpy(buf, data, size);

    if (files[slot].used && files[slot].data)
        kfree(files[slot].data);

    strncpy(files[slot].name, name, FS_NAME_LEN);
    files[slot].name[FS_NAME_LEN - 1] = 0;
    files[slot].data = buf;
    files[slot].size = size;
    files[slot].used = 1;
    return 0;
}

int fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used || strcmp(files[i].name, name) != 0) continue;
        uint32_t n = files[i].size < max ? files[i].size : max;
        if (n) memcpy(buf, files[i].data, n);
        if (out_size) *out_size = n;
        return 0;
    }
    return -1;
}

int fs_delete(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used || strcmp(files[i].name, name) != 0) continue;
        if (files[i].data) kfree(files[i].data);
        files[i].used = 0;
        files[i].data = 0;
        files[i].size = 0;
        files[i].name[0] = 0;
        return 0;
    }
    return -1;
}

int fs_exists(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && strcmp(files[i].name, name) == 0) return 1;
    return 0;
}

int fs_list(void) {
    int n = 0;
    printk("NAME                     SIZE\n");
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) continue;
        printk("%-24s %u\n", files[i].name, files[i].size);
        n++;
    }
    if (!n) printk("(vacio)\n");
    return n;
}
