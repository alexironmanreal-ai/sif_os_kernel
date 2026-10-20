#include "pci.h"
#include "io.h"
#include "printk.h"

static struct pci_device devs[PCI_MAX_DEV];
static int ndev = 0;

uint32_t pci_cfg_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                               (offset & 0xFC) | 0x80000000u);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

void pci_cfg_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                     uint32_t val) {
    uint32_t addr = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                               (offset & 0xFC) | 0x80000000u);
    outl(0xCF8, addr);
    outl(0xCFC, val);
}

uint16_t pci_cfg_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t v = pci_cfg_read32(bus, slot, func, offset);
    return (uint16_t)(v >> ((offset & 2) * 8));
}

void pci_cfg_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                     uint16_t val) {
    uint32_t v = pci_cfg_read32(bus, slot, func, offset);
    int shift = (offset & 2) * 8;
    v &= ~(0xFFFFu << shift);
    v |= ((uint32_t)val) << shift;
    pci_cfg_write32(bus, slot, func, offset, v);
}

uint16_t pci_bar_io(uint8_t bus, uint8_t slot, uint8_t func, int bar) {
    if (bar < 0 || bar > 5)
        return 0;
    uint32_t barv = pci_cfg_read32(bus, slot, func, (uint8_t)(0x10 + bar * 4));
    if (!(barv & 1))
        return 0; /* MMIO */
    return (uint16_t)(barv & ~0x3u);
}

void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t cmd = pci_cfg_read16(bus, slot, func, 0x04);
    cmd |= 0x05; /* IO space + bus master */
    pci_cfg_write16(bus, slot, func, 0x04, cmd);
}

static uint16_t pci_vendor(uint8_t bus, uint8_t slot, uint8_t func) {
    return (uint16_t)(pci_cfg_read32(bus, slot, func, 0) & 0xFFFF);
}

static void scan_func(uint8_t bus, uint8_t slot, uint8_t func) {
    if (ndev >= PCI_MAX_DEV)
        return;
    uint16_t vend = pci_vendor(bus, slot, func);
    if (vend == 0xFFFF)
        return;
    uint32_t reg0 = pci_cfg_read32(bus, slot, func, 0);
    uint32_t reg8 = pci_cfg_read32(bus, slot, func, 0x08);
    uint32_t regc = pci_cfg_read32(bus, slot, func, 0x0C);
    struct pci_device *d = &devs[ndev++];
    d->bus = bus;
    d->slot = slot;
    d->func = func;
    d->vendor = vend;
    d->device = (uint16_t)(reg0 >> 16);
    d->class_code = (uint8_t)(reg8 >> 24);
    d->subclass = (uint8_t)(reg8 >> 16);
    d->prog_if = (uint8_t)(reg8 >> 8);
    d->header_type = (uint8_t)((regc >> 16) & 0xFF);
}

static void scan_slot(uint8_t bus, uint8_t slot) {
    if (pci_vendor(bus, slot, 0) == 0xFFFF)
        return;
    scan_func(bus, slot, 0);
    uint32_t regc = pci_cfg_read32(bus, slot, 0, 0x0C);
    if ((regc >> 16) & 0x80)
        for (uint8_t f = 1; f < 8; f++)
            scan_func(bus, slot, f);
}

void pci_init(void) {
    ndev = 0;
    for (uint16_t bus = 0; bus < 256; bus++)
        for (uint8_t slot = 0; slot < 32; slot++)
            scan_slot((uint8_t)bus, slot);
    printk("[pci] %d device(s)\n", ndev);
}

int pci_count(void) { return ndev; }

const struct pci_device *pci_get(int i) {
    return (i >= 0 && i < ndev) ? &devs[i] : 0;
}

static const char *class_name(uint8_t c) {
    switch (c) {
    case 0x01: return "storage";
    case 0x02: return "network";
    case 0x03: return "display";
    case 0x06: return "bridge";
    case 0x0C: return "serial-bus";
    default: return "other";
    }
}

void pci_list(void) {
    printk("BUS:SLT.FN  VENDOR DEVICE  CLASS\n");
    for (int i = 0; i < ndev; i++) {
        const struct pci_device *d = &devs[i];
        printk("%02x:%02x.%x   %04x   %04x   %02x:%02x %s\n",
               d->bus, d->slot, d->func, d->vendor, d->device,
               d->class_code, d->subclass, class_name(d->class_code));
    }
    if (!ndev)
        printk("(none)\n");
}
