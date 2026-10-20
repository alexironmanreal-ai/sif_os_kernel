#include "multiboot.h"
#include "printk.h"

static struct multiboot_info *g_mbi;
static uint32_t g_magic;
static int g_ok;
static int g_v2;

void multiboot_save(uint32_t magic, struct multiboot_info *mbi) {
    g_magic = magic;
    g_mbi = mbi;
    g_v2 = (magic == MULTIBOOT2_BOOTLOADER_MAGIC) ? 1 : 0;
    g_ok = ((magic == MULTIBOOT_BOOTLOADER_MAGIC || g_v2) && mbi != 0) ? 1 : 0;
}

int multiboot_ok(void) { return g_ok; }
int multiboot_is_v2(void) { return g_v2; }

struct multiboot_info *multiboot_get(void) {
    return g_ok && !g_v2 ? g_mbi : 0;
}

const char *multiboot_cmdline(void) {
    if (!g_ok || g_v2)
        return "";
    if (!(g_mbi->flags & MULTIBOOT_INFO_CMDLINE))
        return "";
    return (const char *)g_mbi->cmdline;
}

const char *multiboot_loader_name(void) {
    if (!g_ok)
        return "unknown";
    if (g_v2)
        return "Multiboot2";
    if (!(g_mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME))
        return "Multiboot1";
    return (const char *)g_mbi->boot_loader_name;
}

uint32_t multiboot_mem_kib(void) {
    if (!g_ok || g_v2)
        return 0;
    if (g_mbi->flags & MULTIBOOT_INFO_MEMORY)
        return g_mbi->mem_lower + g_mbi->mem_upper;
    return 0;
}

static const char *mmap_type_name(uint32_t t) {
    switch (t) {
    case MULTIBOOT_MEMORY_AVAILABLE: return "avail";
    case MULTIBOOT_MEMORY_RESERVED: return "reserv";
    case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: return "acpi";
    case MULTIBOOT_MEMORY_NVS: return "nvs";
    case MULTIBOOT_MEMORY_BADRAM: return "bad";
    default: return "?";
    }
}

void multiboot_dump(void) {
    printk("--- Multiboot ---\n");
    printk("magic=%x ", g_magic);
    if (g_magic == MULTIBOOT_BOOTLOADER_MAGIC)
        printk("MB1 OK\n");
    else if (g_magic == MULTIBOOT2_BOOTLOADER_MAGIC)
        printk("MB2 OK (info parse limited)\n");
    else
        printk("BAD\n");
    if (!g_ok) {
        printk("no valid info\n");
        return;
    }
    printk("loader=%s\n", multiboot_loader_name());
    if (g_v2)
        return;
    printk("flags=%x\n", g_mbi->flags);
    printk("cmdline=\"%s\"\n", multiboot_cmdline());
    if (g_mbi->flags & MULTIBOOT_INFO_MEMORY)
        printk("mem lower=%u KiB upper=%u KiB\n", g_mbi->mem_lower,
               g_mbi->mem_upper);
    if (g_mbi->flags & MULTIBOOT_INFO_MMAP) {
        struct multiboot_mmap_entry *e =
            (struct multiboot_mmap_entry *)g_mbi->mmap_addr;
        uint32_t end = g_mbi->mmap_addr + g_mbi->mmap_length;
        int n = 0;
        while ((uint32_t)e < end && n < 12) {
            if (e->addr_high == 0 && e->len_high == 0)
                printk("  %x +%x %s\n", e->addr_low, e->len_low,
                       mmap_type_name(e->type));
            e = (struct multiboot_mmap_entry *)((uint32_t)e + e->size + 4);
            n++;
        }
    }
}
