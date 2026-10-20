#include "ne2k.h"
#include "pci.h"
#include "io.h"
#include "printk.h"
#include "string.h"

/*
 * NE2000 / RTL8029 PCI — QEMU: -device ne2k_pci
 * Vendor 10EC:8029 or class 02:00
 */

#define NE_CMD      0x00
#define NE_PSTART   0x01
#define NE_PSTOP    0x02
#define NE_BNRY     0x03
#define NE_TPSR     0x04
#define NE_TBCR0    0x05
#define NE_TBCR1    0x06
#define NE_ISR      0x07
#define NE_RSAR0    0x08
#define NE_RSAR1    0x09
#define NE_RBCR0    0x0A
#define NE_RBCR1    0x0B
#define NE_RCR      0x0C
#define NE_TCR      0x0D
#define NE_DCR      0x0E
#define NE_IMR      0x0F
#define NE_PAR0     0x01 /* page 1 */
#define NE_CURR     0x07 /* page 1 */
#define NE_DATA     0x10
#define NE_RESET    0x1F

#define PAGE_TX     0x40
#define PAGE_RX_S   0x46
#define PAGE_RX_E   0x80

static uint16_t iobase;
static uint8_t mac[6];
static int present;

static inline void nw(uint8_t reg, uint8_t v) { outb(iobase + reg, v); }
static inline uint8_t nr(uint8_t reg) { return inb(iobase + reg); }

static void ne_page(int p) {
    uint8_t c = nr(NE_CMD) & 0x3F;
    nw(NE_CMD, (uint8_t)((p << 6) | c));
}

static int find_ne2k(uint8_t *bus, uint8_t *slot, uint8_t *func) {
    for (int i = 0; i < pci_count(); i++) {
        const struct pci_device *d = pci_get(i);
        if (!d)
            continue;
        if (d->vendor == 0x10EC && d->device == 0x8029) {
            *bus = d->bus;
            *slot = d->slot;
            *func = d->func;
            return 0;
        }
        if (d->class_code == 0x02 && d->subclass == 0x00) {
            *bus = d->bus;
            *slot = d->slot;
            *func = d->func;
            return 0;
        }
    }
    return -1;
}

int ne2k_init(void) {
    uint8_t bus, slot, func;
    present = 0;
    iobase = 0;

    if (find_ne2k(&bus, &slot, &func) < 0) {
        printk("[ne2k] no NIC\n");
        return -1;
    }

    iobase = pci_bar_io(bus, slot, func, 0);
    if (!iobase) {
        printk("[ne2k] no IO BAR\n");
        return -1;
    }

    pci_enable_bus_master(bus, slot, func);

    /* reset */
    (void)inb(iobase + NE_RESET);
    for (volatile int i = 0; i < 100000; i++)
        ;
    outb(iobase + NE_RESET, 0);
    for (volatile int i = 0; i < 100000; i++)
        ;

    nw(NE_CMD, 0x21); /* page0 stop */
    nw(NE_DCR, 0x49); /* word, fifo 8, normal */
    nw(NE_RBCR0, 0);
    nw(NE_RBCR1, 0);
    nw(NE_RCR, 0x20); /* monitor */
    nw(NE_TCR, 0x02); /* loopback internal while setup */
    nw(NE_PSTART, PAGE_RX_S);
    nw(NE_PSTOP, PAGE_RX_E);
    nw(NE_BNRY, PAGE_RX_S);
    nw(NE_ISR, 0xFF);
    nw(NE_IMR, 0x00);

    /* read MAC from PROM (first 6 words remote DMA) */
    nw(NE_RSAR0, 0);
    nw(NE_RSAR1, 0);
    nw(NE_RBCR0, 24);
    nw(NE_RBCR1, 0);
    nw(NE_CMD, 0x0A); /* remote read */

    for (int i = 0; i < 6; i++) {
        mac[i] = inb(iobase + NE_DATA);
        (void)inb(iobase + NE_DATA); /* duplicate on some cards */
    }
    /* drain rest of prom read */
    for (int i = 0; i < 6; i++) {
        (void)inb(iobase + NE_DATA);
        (void)inb(iobase + NE_DATA);
    }

    ne_page(1);
    for (int i = 0; i < 6; i++)
        nw((uint8_t)(NE_PAR0 + i), mac[i]);
    nw(NE_CURR, PAGE_RX_S + 1);
    ne_page(0);

    nw(NE_RCR, 0x04); /* accept broadcast */
    nw(NE_TCR, 0x00); /* normal */
    nw(NE_ISR, 0xFF);
    nw(NE_CMD, 0x22); /* start, no DMA */

    present = 1;
    printk("[ne2k] IO=%x MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           iobase, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 0;
}

int ne2k_present(void) { return present; }

void ne2k_get_mac(uint8_t out[6]) {
    for (int i = 0; i < 6; i++)
        out[i] = mac[i];
}

int ne2k_send(const void *data, uint16_t len) {
    if (!present || !data || len < 14)
        return -1;
    if (len > 1514)
        len = 1514;
    if (len < 60)
        len = 60; /* pad eth min — caller should zero pad */

    /* wait for idle TX */
    for (int t = 0; t < 100000; t++)
        if (!(nr(NE_CMD) & 0x04))
            break;

    nw(NE_CMD, 0x22);
    nw(NE_RBCR0, (uint8_t)(len & 0xFF));
    nw(NE_RBCR1, (uint8_t)(len >> 8));
    nw(NE_RSAR0, 0);
    nw(NE_RSAR1, PAGE_TX);
    nw(NE_CMD, 0x12); /* remote write */

    const uint8_t *p = (const uint8_t *)data;
    for (uint16_t i = 0; i < len; i++)
        outb(iobase + NE_DATA, p[i]);

    nw(NE_TBCR0, (uint8_t)(len & 0xFF));
    nw(NE_TBCR1, (uint8_t)(len >> 8));
    nw(NE_TPSR, PAGE_TX);
    nw(NE_CMD, 0x26); /* TX + start */

    return (int)len;
}

int ne2k_recv(void *buf, uint16_t max) {
    if (!present || !buf || max < 64)
        return -1;

    ne_page(1);
    uint8_t curr = nr(NE_CURR);
    ne_page(0);
    uint8_t bnry = nr(NE_BNRY);

    uint8_t next = (uint8_t)(bnry + 1);
    if (next >= PAGE_RX_E)
        next = PAGE_RX_S;
    if (next == curr)
        return 0; /* empty */

    /* read 4-byte header at next page */
    nw(NE_RBCR0, 4);
    nw(NE_RBCR1, 0);
    nw(NE_RSAR0, 0);
    nw(NE_RSAR1, next);
    nw(NE_CMD, 0x0A);

    uint8_t status = inb(iobase + NE_DATA);
    uint8_t next_pkt = inb(iobase + NE_DATA);
    uint8_t len_lo = inb(iobase + NE_DATA);
    uint8_t len_hi = inb(iobase + NE_DATA);
    uint16_t len = (uint16_t)(len_lo | (len_hi << 8));

    (void)status;
    if (len < 64 || len > 1518) {
        /* resync */
        nw(NE_BNRY, curr == PAGE_RX_S ? PAGE_RX_E - 1 : (uint8_t)(curr - 1));
        return -1;
    }

    uint16_t copy = len - 4; /* strip FCS-ish header count includes header */
    if (copy > max)
        copy = max;

    nw(NE_RBCR0, (uint8_t)(copy & 0xFF));
    nw(NE_RBCR1, (uint8_t)(copy >> 8));
    nw(NE_RSAR0, 4);
    nw(NE_RSAR1, next);
    nw(NE_CMD, 0x0A);

    uint8_t *out = (uint8_t *)buf;
    for (uint16_t i = 0; i < copy; i++)
        out[i] = inb(iobase + NE_DATA);

    /* advance boundary */
    uint8_t new_bnry = (uint8_t)(next_pkt - 1);
    if (new_bnry < PAGE_RX_S || new_bnry >= PAGE_RX_E)
        new_bnry = PAGE_RX_S;
    nw(NE_BNRY, new_bnry);

    return (int)copy;
}
