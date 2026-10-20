#include "wire.h"
#include "ne2k.h"
#include "printk.h"
#include "string.h"
#include "dev.h"

static uint32_t tx_n, rx_n;
static uint8_t my_mac[6];
static int up;

void wire_init(void) {
    tx_n = rx_n = 0;
    up = 0;
    if (ne2k_init() == 0) {
        ne2k_get_mac(my_mac);
        up = 1;
        dev_register("ne2k", DEV_KIND_LINK, 1, "NE2000 wire");
        printk("[wire] link UP\n");
    } else {
        dev_register("ne2k", DEV_KIND_LINK, 0, "no NIC");
        printk("[wire] link DOWN\n");
    }
}

int wire_up(void) { return up; }

void wire_mac(uint8_t mac[6]) {
    for (int i = 0; i < 6; i++)
        mac[i] = my_mac[i];
}

uint32_t wire_tx_count(void) { return tx_n; }
uint32_t wire_rx_count(void) { return rx_n; }

void wire_status(void) {
    if (!up) {
        printk("wire: DOWN (add -device ne2k_pci in QEMU)\n");
        return;
    }
    printk("wire: UP\n");
    printk("mac:  %02x:%02x:%02x:%02x:%02x:%02x\n",
           my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
    printk("tx=%u rx=%u\n", tx_n, rx_n);
}

int wire_cast(const uint8_t dst[6], const void *payload, uint16_t len) {
    if (!up || !payload)
        return -1;
    if (len > 1400)
        len = 1400;

    uint8_t frame[1514];
    memset(frame, 0, sizeof(frame));

    for (int i = 0; i < 6; i++)
        frame[i] = dst[i];
    for (int i = 0; i < 6; i++)
        frame[6 + i] = my_mac[i];
    frame[12] = (uint8_t)(WIRE_ETHTYPE_SIF >> 8);
    frame[13] = (uint8_t)(WIRE_ETHTYPE_SIF & 0xFF);
    memcpy(frame + 14, payload, len);

    uint16_t total = (uint16_t)(14 + len);
    if (total < 60)
        total = 60;

    int r = ne2k_send(frame, total);
    if (r > 0)
        tx_n++;
    return r;
}

int wire_cast_text(const char *text) {
    if (!text)
        return -1;
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint16_t n = 0;
    while (text[n] && n < 200)
        n++;
    return wire_cast(bcast, text, n);
}

int wire_sniff(int verbose) {
    if (!up)
        return -1;
    uint8_t buf[1514];
    int n = ne2k_recv(buf, sizeof(buf));
    if (n <= 0)
        return n;
    rx_n++;
    if (verbose) {
        printk("rx %d B  dst=%02x:%02x:%02x:%02x:%02x:%02x",
               n, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        printk(" src=%02x:%02x:%02x:%02x:%02x:%02x",
               buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
        if (n >= 14) {
            uint16_t et = (uint16_t)((buf[12] << 8) | buf[13]);
            printk(" type=%04x\n", et);
            if (et == WIRE_ETHTYPE_SIF && n > 14) {
                int plen = n - 14;
                if (plen > 64)
                    plen = 64;
                printk("  sif: ");
                for (int i = 0; i < plen; i++) {
                    char c = (char)buf[14 + i];
                    if (c >= 32 && c < 127)
                        printk("%c", c);
                    else
                        printk(".");
                }
                printk("\n");
            }
        } else
            printk("\n");
    }
    return n;
}
