#include "fs.h"
#include "kmalloc.h"
#include "printk.h"
#include "string.h"
struct fs_file { char name[FS_NAME_LEN]; uint32_t size; uint8_t *data; int used; };
static struct fs_file files[FS_MAX_FILES];
static int find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && strcmp(files[i].name, name) == 0) return i;
    return -1;
}
static int free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) if (!files[i].used) return i;
    return -1;
}
int fs_create(const char *name, const void *data, uint32_t size) {
    if (size > FS_MAX_SIZE) return -1;
    int slot = find(name);
    if (slot < 0) slot = free_slot();
    if (slot < 0) return -1;
    if (files[slot].used && files[slot].data) kfree(files[slot].data);
    uint8_t *buf = (uint8_t *)kmalloc(size ? size : 1);
    if (!buf) return -1;
    if (data && size) memcpy(buf, data, size);
    strncpy(files[slot].name, name, FS_NAME_LEN - 1);
    files[slot].name[FS_NAME_LEN - 1] = 0;
    files[slot].data = buf; files[slot].size = size; files[slot].used = 1;
    return 0;
}
void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = 0; files[i].data = 0; files[i].size = 0; files[i].name[0] = 0;
    }
    fs_create("readme.txt", "SIF Kernel ramfs\nComandos: ls cat write rm exec help\n", 58);
    fs_create("hello.txt", "hola desde el filesystem\n", 26);
    fs_create("motd", "Bienvenido a SIF OS\n", 20);
    /* minimal ELF32 hello at 0x08000000 — generado offline */
    {
        static const uint8_t hello_elf[] = {
            0x7f,0x45,0x4c,0x46,0x01,0x01,0x01,0,0,0,0,0,0,0,0,0,
            0x02,0,0x03,0,1,0,0,0,0,0,0,0x08,0x34,0,0,0,0,0,0,0,0,0,0,0,
            0x34,0,0x20,0,1,0,0,0,0,0,0,0,
            1,0,0,0,0x54,0,0,0,0,0,0,0x08,0,0,0,0x08,0x6e,1,0,0,0,10,0,0,7,0,0,0,0,16,0,0,
            0xb8,2,0,0,0,0xbb,0,1,0,0x08,0xcd,0x80,0xb8,1,0,0,0,0xcd,0x80,0xf4,0xeb,0xfd
        };
        /* Note: full valid ELF is in local tree; this stub may need rebuild from repo scripts */
        fs_create("hello.elf", hello_elf, sizeof(hello_elf));
    }
    printk("[fs] ramfs listo\n");
}
int fs_write(const char *name, const void *data, uint32_t size) { return fs_create(name, data, size); }
int fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size) {
    int i = find(name); if (i < 0) return -1;
    uint32_t n = files[i].size < max ? files[i].size : max;
    memcpy(buf, files[i].data, n);
    if (out_size) *out_size = n;
    return 0;
}
int fs_delete(const char *name) {
    int i = find(name); if (i < 0) return -1;
    if (files[i].data) kfree(files[i].data);
    files[i].data=0; files[i].size=0; files[i].used=0; files[i].name[0]=0;
    return 0;
}
int fs_exists(const char *name) { return find(name) >= 0; }
int fs_list(void) {
    int n=0; printk("NAME                             SIZE\n");
    for (int i=0;i<FS_MAX_FILES;i++) if (files[i].used) { printk("%-32s %u\n", files[i].name, files[i].size); n++; }
    if (!n) printk("(vacio)\n"); return n;
}
int fs_append(const char *name, const void *data, uint32_t size) {
    int i = find(name);
    if (i < 0) return fs_create(name, data, size);
    uint32_t ns = files[i].size + size;
    if (ns > FS_MAX_SIZE) return -1;
    uint8_t *buf = (uint8_t *)kmalloc(ns); if (!buf) return -1;
    memcpy(buf, files[i].data, files[i].size);
    if (data && size) memcpy(buf + files[i].size, data, size);
    kfree(files[i].data); files[i].data = buf; files[i].size = ns;
    return 0;
}
