#ifndef SIF_PCI_H
#define SIF_PCI_H
#include <stdint.h>

struct pci_device {
    uint8_t bus, slot, func;
    uint16_t vendor, device;
    uint8_t class_code, subclass, prog_if, header_type;
};

#define PCI_MAX_DEV 32

void pci_init(void);
int pci_count(void);
const struct pci_device *pci_get(int i);
void pci_list(void);

uint32_t pci_cfg_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void pci_cfg_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t val);
uint16_t pci_cfg_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void pci_cfg_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint16_t val);

/* BAR index 0..5 — returns IO port base (bit0 set) or 0 if MMIO/absent */
uint16_t pci_bar_io(uint8_t bus, uint8_t slot, uint8_t func, int bar);
void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t func);

#endif
