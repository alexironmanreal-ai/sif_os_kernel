#include "ata.h"
#include "io.h"
#include "printk.h"
#define ATA_DATA 0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_CMD 0x1F7
#define ATA_ALT_STATUS 0x3F6
#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_IDENTIFY 0xEC
static int present = 0;
static inline uint16_t inw(uint16_t port) {
    uint16_t v; __asm__ volatile ("inw %1, %0" : "=a"(v) : "Nd"(port)); return v;
}
static int wait_bsy_clear(void) {
    for (int i = 0; i < 1000000; i++) if (!(inb(ATA_STATUS) & ATA_SR_BSY)) return 0;
    return -1;
}
static int wait_drq(void) {
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s & ATA_SR_ERR) return -1;
        if (s & ATA_SR_DRQ) return 0;
    }
    return -1;
}
void ata_init(void) {
    outb(ATA_DRIVE, 0xE0);
    for (int i = 0; i < 15; i++) (void)inb(ATA_ALT_STATUS);
    outb(ATA_SECCOUNT, 0); outb(ATA_LBA_LO, 0); outb(ATA_LBA_MID, 0); outb(ATA_LBA_HI, 0);
    outb(ATA_CMD, ATA_CMD_IDENTIFY);
    if (inb(ATA_STATUS) == 0) { printk("[ata] sin disco\n"); present = 0; return; }
    if (wait_bsy_clear() < 0) { printk("[ata] timeout\n"); present = 0; return; }
    if (inb(ATA_LBA_MID) || inb(ATA_LBA_HI)) { printk("[ata] no-ATA\n"); present = 0; return; }
    if (wait_drq() < 0) { printk("[ata] identify fail\n"); present = 0; return; }
    for (int i = 0; i < 256; i++) (void)inw(ATA_DATA);
    present = 1; printk("[ata] primary master listo\n");
}
int ata_present(void) { return present; }
int ata_read_sectors(uint32_t lba, uint8_t count, void *buf) {
    if (!present || !count) return -1;
    if (wait_bsy_clear() < 0) return -1;
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_CMD, ATA_CMD_READ_PIO);
    uint16_t *out = (uint16_t *)buf;
    for (uint8_t s = 0; s < count; s++) {
        if (wait_bsy_clear() < 0) return -1;
        if (wait_drq() < 0) return -1;
        for (int i = 0; i < 256; i++) *out++ = inw(ATA_DATA);
    }
    return 0;
}
