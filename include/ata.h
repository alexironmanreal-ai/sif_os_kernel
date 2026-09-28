#ifndef SIF_ATA_H
#define SIF_ATA_H
#include <stdint.h>
#include <stddef.h>
void ata_init(void);
int ata_read_sectors(uint32_t lba, uint8_t count, void *buf);
int ata_present(void);
#endif
