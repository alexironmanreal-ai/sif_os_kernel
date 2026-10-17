#ifndef SIF_DISKFS_H
#define SIF_DISKFS_H
#include <stdint.h>
#include <stddef.h>

#define SFS_MAGIC 0x53494653u /* 'SIFS' */
#define SFS_VERSION 1
#define SFS_MAX_FILES 32
#define SFS_NAME_LEN 28
#define SFS_SECTOR 512
#define SFS_SUPER_LBA 1
#define SFS_TABLE_LBA 2
#define SFS_TABLE_SECTORS 8 /* 8*512 = 4096 >= 32*64 */
#define SFS_DATA_LBA 16
#define SFS_MAX_FILE_SECTORS 32 /* max 16 KiB per file */
#define SFS_DATA_BLOCKS 1024    /* ~512 KiB data region */

struct sfs_super {
    uint32_t magic;
    uint32_t version;
    uint32_t max_files;
    uint32_t data_lba;
    uint32_t data_blocks;
    uint32_t next_free; /* next free data LBA */
    uint8_t pad[512 - 24];
} __attribute__((packed));

struct sfs_entry {
    char name[SFS_NAME_LEN];
    uint32_t size;
    uint32_t start_lba;
    uint32_t nsectors;
    uint32_t used;
} __attribute__((packed)); /* 44 bytes; table uses 64-byte slots */

void diskfs_init(void);
int diskfs_mounted(void);
int diskfs_format(void);
int diskfs_mount(void);
int diskfs_sync(void);
int diskfs_create(const char *name, const void *data, uint32_t size);
int diskfs_write(const char *name, const void *data, uint32_t size);
int diskfs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size);
int diskfs_delete(const char *name);
int diskfs_list(void);
int diskfs_exists(const char *name);

#endif
