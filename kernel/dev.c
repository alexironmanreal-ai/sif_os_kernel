#include "dev.h"
#include "string.h"
#include "printk.h"
#include "ata.h"
#include "pci.h"

static struct sif_dev table[DEV_MAX];
static int nreg;

void dev_init(void) {
    nreg = 0;
    for (int i = 0; i < DEV_MAX; i++) {
        table[i].tag[0] = 0;
        table[i].present = 0;
        table[i].note = 0;
    }

    dev_register("kbd", DEV_KIND_INPUT, 1, "PS/2 keys");
    dev_register("mouse", DEV_KIND_INPUT, 1, "PS/2 pointer");
    dev_register("vga", DEV_KIND_DISPLAY, 1, "text+mode13");
    dev_register("serial", DEV_KIND_LINK, 1, "COM1 log");
    dev_register("rtc", DEV_KIND_CLOCK, 1, "CMOS clock");
    dev_register("pit", DEV_KIND_CLOCK, 1, "100Hz timer");
    dev_register("ata0", DEV_KIND_STORAGE, ata_present() ? 1 : 0, "IDE PIO");
    dev_register("pci", DEV_KIND_BUS, pci_count() > 0 ? 1 : 0, "config space");

    printk("[dev] registry %d units\n", nreg);
}

int dev_register(const char *tag, enum dev_kind kind, int present, const char *note) {
    if (!tag || !tag[0] || nreg >= DEV_MAX)
        return -1;
    for (int i = 0; i < nreg; i++)
        if (strcmp(table[i].tag, tag) == 0) {
            table[i].kind = kind;
            table[i].present = present;
            table[i].note = note;
            return 0;
        }
    strncpy(table[nreg].tag, tag, DEV_TAG_LEN - 1);
    table[nreg].tag[DEV_TAG_LEN - 1] = 0;
    table[nreg].kind = kind;
    table[nreg].present = present;
    table[nreg].note = note;
    nreg++;
    return 0;
}

int dev_set_present(const char *tag, int present) {
    for (int i = 0; i < nreg; i++)
        if (strcmp(table[i].tag, tag) == 0) {
            table[i].present = present;
            return 0;
        }
    return -1;
}

int dev_count_present(void) {
    int n = 0;
    for (int i = 0; i < nreg; i++)
        if (table[i].present)
            n++;
    return n;
}

static const char *kind_name(enum dev_kind k) {
    switch (k) {
    case DEV_KIND_INPUT: return "input";
    case DEV_KIND_STORAGE: return "store";
    case DEV_KIND_DISPLAY: return "view";
    case DEV_KIND_CLOCK: return "clock";
    case DEV_KIND_BUS: return "bus";
    case DEV_KIND_LINK: return "link";
    default: return "?";
    }
}

void dev_list(void) {
    printk("TAG        KIND    ON  NOTE\n");
    for (int i = 0; i < nreg; i++) {
        printk("%-10s %-7s %s  %s\n", table[i].tag, kind_name(table[i].kind),
               table[i].present ? "yes" : "no ",
               table[i].note ? table[i].note : "-");
    }
    printk("present %d / %d\n", dev_count_present(), nreg);
}
