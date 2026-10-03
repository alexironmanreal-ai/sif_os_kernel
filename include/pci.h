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
#endif
