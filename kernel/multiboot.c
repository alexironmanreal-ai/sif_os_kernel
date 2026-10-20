#include "multiboot.h"
#include "printk.h"

static struct multiboot_info *g_mbi;
static uint32_t g_magic;
static int g_ok;

void multiboot_save(uint32_t magic, struct multiboot_info *mbi) {
    g_magic = magic;
    g_mbi = mbi;
    g_ok = (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi != 0) ? 1 : 0;
}

int multiboot_ok(void) { return g_ok; }

struct multiboot_info *multiboot_get(void) {
    return g_ok ? g_mbi : 0;
}

const char *multiboot_cmdline(void) {
    if (!g_ok || !(g_mbi->flags & MULTIBOOT_INFO_CMDLINE))
        return "";
    return (const char *)g_mbi->cmdline;
}

const char *multiboot_loader_name(void) {
    if (!g_ok || !(g_mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME))
        return "unknown";
    return (const char *)g_mbi->boot_loader_name;
}

uint32_t multiboot_mem_kib(void) {
    if (!g_ok)
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
    printk("magic=%x %s\n", g_magic,
           g_magic == MULTIBOOT_BOOTLOADER_MAGIC ? "OK" : "BAD");
    if (!g_ok) {
        printk("no valid info struct\n");
        return;
    }
    printk("flags=%x\n", g_mbi->flags);
    printk("loader=%s\n", multiboot_loader_name());
    printk("cmdline=\"%s\"\n", multiboot_cmdline());

    if (g_mbi->flags & MULTIBOOT_INFO_MEMORY)
        printk("mem lower=%u KiB upper=%u KiB (~%u MiB)\n",
               g_mbi->mem_lower, g_mbi->mem_upper,
               (g_mbi->mem_lower + g_mbi->mem_upper) / 1024);

    if (g_mbi->flags & MULTIBOOT_INFO_BOOTDEV)
        printk("boot_device=%x\n", g_mbi->boot_device);

    if (g_mbi->flags & MULTIBOOT_INFO_MODS)
        printk("modules=%u @ %x\n", g_mbi->mods_count, g_mbi->mods_addr);

    if (g_mbi->flags & MULTIBOOT_INFO_MMAP) {
        printk("mmap len=%u @ %x\n", g_mbi->mmap_length, g_mbi->mmap_addr);
        struct multiboot_mmap_entry *e =
            (struct multiboot_mmap_entry *)g_mbi->mmap_addr;
        uint32_t end = g_mbi->mmap_addr + g_mbi->mmap_length;
        int n = 0;
        while ((uint32_t)e < end && n < 16) {
            if (e->addr_high == 0 && e->len_high == 0)
                printk("  %x +%x %s\n", e->addr_low, e->len_low,
                       mmap_type_name(e->type));
            e = (struct multiboot_mmap_entry *)((uint32_t)e + e->size + 4);
            n++;
        }
    }
}
