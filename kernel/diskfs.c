#include "diskfs.h"
#include "ata.h"
#include "fs.h"
#include "printk.h"
#include "string.h"

#define ENTRY_SLOT 64

static int mounted;
static struct sfs_super super;
static struct sfs_entry table[SFS_MAX_FILES];
static int dirty;

static int load_super(void) {
    uint8_t sec[SFS_SECTOR];
    if (ata_read_sectors(SFS_SUPER_LBA, 1, sec) < 0)
        return -1;
    memcpy(&super, sec, sizeof(super));
    if (super.magic != SFS_MAGIC || super.version != SFS_VERSION)
        return -1;
    return 0;
}

static int save_super(void) {
    uint8_t sec[SFS_SECTOR];
    memset(sec, 0, sizeof(sec));
    struct sfs_super *s = (struct sfs_super *)sec;
    s->magic = super.magic;
    s->version = super.version;
    s->max_files = super.max_files;
    s->data_lba = super.data_lba;
    s->data_blocks = super.data_blocks;
    s->next_free = super.next_free;
    return ata_write_sectors(SFS_SUPER_LBA, 1, sec);
}

static int load_table(void) {
    uint8_t buf[SFS_TABLE_SECTORS * SFS_SECTOR];
    if (ata_read_sectors(SFS_TABLE_LBA, SFS_TABLE_SECTORS, buf) < 0)
        return -1;
    for (int i = 0; i < SFS_MAX_FILES; i++)
        memcpy(&table[i], buf + i * ENTRY_SLOT, sizeof(struct sfs_entry));
    return 0;
}

static int save_table(void) {
    uint8_t buf[SFS_TABLE_SECTORS * SFS_SECTOR];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < SFS_MAX_FILES; i++)
        memcpy(buf + i * ENTRY_SLOT, &table[i], sizeof(struct sfs_entry));
    return ata_write_sectors(SFS_TABLE_LBA, SFS_TABLE_SECTORS, buf);
}

static int find_name(const char *name) {
    for (int i = 0; i < SFS_MAX_FILES; i++)
        if (table[i].used && strcmp(table[i].name, name) == 0)
            return i;
    return -1;
}

static int free_slot(void) {
    for (int i = 0; i < SFS_MAX_FILES; i++)
        if (!table[i].used)
            return i;
    return -1;
}

int diskfs_import_ramfs(void) {
    if (!mounted)
        return -1;
    int n = 0;
    uint8_t buf[SFS_MAX_FILE_SECTORS * SFS_SECTOR];
    for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (!table[i].used)
            continue;
        uint32_t sz = 0;
        if (diskfs_read(table[i].name, buf, sizeof(buf), &sz) < 0)
            continue;
        if (fs_create(table[i].name, buf, sz) == 0)
            n++;
    }
    printk("[sfs] import %d files → ramfs\n", n);
    return n;
}

void diskfs_init(void) {
    mounted = 0;
    dirty = 0;
    memset(&super, 0, sizeof(super));
    memset(table, 0, sizeof(table));
    if (!ata_present()) {
        printk("[sfs] sin disco — solo ramfs\n");
        return;
    }
    if (diskfs_mount() == 0) {
        printk("[sfs] montado\n");
        diskfs_import_ramfs();
    } else
        printk("[sfs] no formateado — usa mkfs\n");
}

int diskfs_mounted(void) { return mounted; }

int diskfs_format(void) {
    if (!ata_present()) {
        printk("[sfs] no hay ATA\n");
        return -1;
    }
    memset(&super, 0, sizeof(super));
    super.magic = SFS_MAGIC;
    super.version = SFS_VERSION;
    super.max_files = SFS_MAX_FILES;
    super.data_lba = SFS_DATA_LBA;
    super.data_blocks = SFS_DATA_BLOCKS;
    super.next_free = SFS_DATA_LBA;
    memset(table, 0, sizeof(table));
    if (save_super() < 0 || save_table() < 0) {
        printk("[sfs] format fail\n");
        return -1;
    }
    mounted = 1;
    dirty = 0;
    printk("[sfs] formateado OK\n");
    return 0;
}

int diskfs_mount(void) {
    if (!ata_present())
        return -1;
    if (load_super() < 0)
        return -1;
    if (load_table() < 0)
        return -1;
    mounted = 1;
    dirty = 0;
    return 0;
}

int diskfs_sync(void) {
    if (!mounted)
        return -1;
    if (save_super() < 0 || save_table() < 0)
        return -1;
    dirty = 0;
    printk("[sfs] sync OK\n");
    return 0;
}

int diskfs_exists(const char *name) {
    if (!mounted || !name)
        return 0;
    return find_name(name) >= 0;
}

int diskfs_create(const char *name, const void *data, uint32_t size) {
    if (!mounted || !name || !name[0])
        return -1;
    if (size > SFS_MAX_FILE_SECTORS * SFS_SECTOR)
        return -1;

    int idx = find_name(name);
    if (idx < 0)
        idx = free_slot();
    if (idx < 0)
        return -1;

    uint32_t need = (size + SFS_SECTOR - 1) / SFS_SECTOR;
    if (need == 0)
        need = 1;
    if (super.next_free + need > super.data_lba + super.data_blocks)
        return -1;

    uint32_t start = super.next_free;
    uint8_t sec[SFS_SECTOR];
    const uint8_t *src = (const uint8_t *)data;
    uint32_t left = size;

    for (uint32_t s = 0; s < need; s++) {
        memset(sec, 0, SFS_SECTOR);
        uint32_t chunk = left > SFS_SECTOR ? SFS_SECTOR : left;
        if (src && chunk)
            memcpy(sec, src, chunk);
        if (ata_write_sectors(start + s, 1, sec) < 0)
            return -1;
        if (src)
            src += chunk;
        left -= chunk;
    }

    memset(&table[idx], 0, sizeof(table[idx]));
    strncpy(table[idx].name, name, SFS_NAME_LEN - 1);
    table[idx].size = size;
    table[idx].start_lba = start;
    table[idx].nsectors = need;
    table[idx].used = 1;
    super.next_free = start + need;
    dirty = 1;
    /* also mirror to ramfs */
    fs_create(name, data, size);
    return diskfs_sync();
}

int diskfs_write(const char *name, const void *data, uint32_t size) {
    return diskfs_create(name, data, size);
}

int diskfs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size) {
    if (!mounted || !name || !buf)
        return -1;
    int idx = find_name(name);
    if (idx < 0)
        return -1;

    uint32_t total = table[idx].size;
    if (total > max)
        total = max;

    uint8_t sec[SFS_SECTOR];
    uint8_t *dst = (uint8_t *)buf;
    uint32_t got = 0;
    uint32_t lba = table[idx].start_lba;

    while (got < total) {
        if (ata_read_sectors(lba, 1, sec) < 0)
            return -1;
        uint32_t chunk = total - got;
        if (chunk > SFS_SECTOR)
            chunk = SFS_SECTOR;
        memcpy(dst + got, sec, chunk);
        got += chunk;
        lba++;
    }
    if (out_size)
        *out_size = got;
    return 0;
}

int diskfs_delete(const char *name) {
    if (!mounted || !name)
        return -1;
    int idx = find_name(name);
    if (idx < 0)
        return -1;
    table[idx].used = 0;
    table[idx].name[0] = 0;
    table[idx].size = 0;
    dirty = 1;
    return diskfs_sync();
}

int diskfs_list(void) {
    if (!mounted) {
        printk("[sfs] no montado\n");
        return -1;
    }
    int n = 0;
    printk("DISK NAME                        SIZE   LBA\n");
    for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (!table[i].used)
            continue;
        printk("%-28s %6u  %u\n", table[i].name, table[i].size,
               table[i].start_lba);
        n++;
    }
    if (!n)
        printk("(vacio)\n");
    printk("next_free LBA=%u dirty=%d\n", super.next_free, dirty);
    return n;
}
