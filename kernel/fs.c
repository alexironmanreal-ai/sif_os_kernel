#include "fs.h"
#include "kmalloc.h"
#include "printk.h"
static struct {
    char name[FS_NAME_LEN];
    uint32_t size;
    uint8_t *data;
    int used;
} files[FS_MAX_FILES];
static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}
static void scopy(char *d, const char *s, int n) {
    int i = 0; while (s[i] && i < n-1) { d[i]=s[i]; i++; } d[i]=0;
}
void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = 0; files[i].data = 0; files[i].size = 0; files[i].name[0] = 0;
    }
    fs_create("readme.txt", "SIF Kernel ramfs\nComandos: ls cat mem ps help\n", 52);
    fs_create("hello.txt", "hola desde el filesystem\n", 26);
    printk("[fs] ramfs listo\n");
}
int fs_create(const char *name, const void *data, uint32_t size) {
    if (size > FS_MAX_SIZE) return -1;
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && streq(files[i].name, name)) { slot = i; break; }
        if (!files[i].used && slot < 0) slot = i;
    }
    if (slot < 0) return -1;
    uint8_t *buf = (uint8_t *)kmalloc(size ? size : 1);
    if (!buf) return -1;
    for (uint32_t i = 0; i < size; i++) buf[i] = ((const uint8_t *)data)[i];
    scopy(files[slot].name, name, FS_NAME_LEN);
    files[slot].data = buf; files[slot].size = size; files[slot].used = 1;
    return 0;
}
int fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used || !streq(files[i].name, name)) continue;
        uint32_t n = files[i].size < max ? files[i].size : max;
        for (uint32_t j = 0; j < n; j++) ((uint8_t *)buf)[j] = files[i].data[j];
        if (out_size) *out_size = n;
        return 0;
    }
    return -1;
}
int fs_exists(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && streq(files[i].name, name)) return 1;
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
