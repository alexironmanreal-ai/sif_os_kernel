#ifndef SIF_MULTIBOOT_H
#define SIF_MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_HEADER_MAGIC     0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define MULTIBOOT_INFO_MEMORY      (1u << 0)
#define MULTIBOOT_INFO_BOOTDEV     (1u << 1)
#define MULTIBOOT_INFO_CMDLINE     (1u << 2)
#define MULTIBOOT_INFO_MODS        (1u << 3)
#define MULTIBOOT_INFO_AOUT_SYMS   (1u << 4)
#define MULTIBOOT_INFO_ELF_SHDR    (1u << 5)
#define MULTIBOOT_INFO_MMAP        (1u << 6)
#define MULTIBOOT_INFO_DRIVE_INFO  (1u << 7)
#define MULTIBOOT_INFO_CONFIG_TABLE (1u << 8)
#define MULTIBOOT_INFO_BOOT_LOADER_NAME (1u << 9)
#define MULTIBOOT_INFO_APM_TABLE   (1u << 10)
#define MULTIBOOT_INFO_VBE         (1u << 11)
#define MULTIBOOT_INFO_FRAMEBUFFER (1u << 12)

#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

struct multiboot_mmap_entry {
    uint32_t size;       /* size of this entry minus 4 */
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
} __attribute__((packed));

struct multiboot_mod_list {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t pad;
} __attribute__((packed));

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;      /* KiB below 1MiB */
    uint32_t mem_upper;      /* KiB above 1MiB */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    /* Multiboot framebuffer (if flags bit 12) — optional */
    uint32_t framebuffer_addr_low;
    uint32_t framebuffer_addr_high;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed));

/* stored by kernel after parse */
void multiboot_save(uint32_t magic, struct multiboot_info *mbi);
int multiboot_ok(void);
struct multiboot_info *multiboot_get(void);
const char *multiboot_cmdline(void);
const char *multiboot_loader_name(void);
void multiboot_dump(void);
uint32_t multiboot_mem_kib(void);

#endif
