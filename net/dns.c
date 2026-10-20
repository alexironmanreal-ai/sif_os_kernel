#include "net.h"
#include "printk.h"
#include "string.h"

/* minimal DNS A query → first A answer */

int dns_resolve(const char *name, uint8_t out_ip[4]) {
    if (!name || !out_ip)
        return -1;

    uint8_t dns_ip[4] = {10, 0, 2, 3}; /* QEMU user DNS */
    uint8_t q[256];
    memset(q, 0, sizeof(q));
    q[0] = 0x12;
    q[1] = 0x34; /* id */
    q[2] = 0x01;
    q[3] = 0x00; /* recursion desired */
    q[4] = 0x00;
    q[5] = 0x01; /* QDCOUNT=1 */

    /* QNAME */
    int pos = 12;
    const char *p = name;
    while (*p && pos < 200) {
        const char *dot = p;
        while (*dot && *dot != '.')
            dot++;
        int lab = (int)(dot - p);
        if (lab <= 0 || lab > 63)
            return -1;
        q[pos++] = (uint8_t)lab;
        for (int i = 0; i < lab; i++)
            q[pos++] = (uint8_t)p[i];
        if (*dot == '.')
            p = dot + 1;
        else {
            p = dot;
            break;
        }
    }
    q[pos++] = 0;
    q[pos++] = 0x00;
    q[pos++] = 0x01; /* type A */
    q[pos++] = 0x00;
    q[pos++] = 0x01; /* class IN */

    uint16_t myport = 53000;
    if (udp_send(dns_ip, myport, 53, q, (uint16_t)pos) < 0) {
        printk("[dns] send fail\n");
        return -1;
    }

    uint8_t resp[512];
    uint8_t sip[4];
    uint16_t sport;
    int n = udp_recv(myport, resp, sizeof(resp), sip, &sport, 30);
    if (n < 12) {
        printk("[dns] timeout/empty\n");
        return -1;
    }

    /* skip header + question to answers — naive scan for type A rdata */
    int i = 12;
    /* skip QNAME */
    while (i < n && resp[i] != 0) {
        if ((resp[i] & 0xC0) == 0xC0) {
            i += 2;
            break;
        }
        i += resp[i] + 1;
    }
    if (i < n && resp[i] == 0)
        i++;
    i += 4; /* type class */

    uint16_t ancount = (uint16_t)((resp[6] << 8) | resp[7]);
    for (uint16_t a = 0; a < ancount && i + 12 <= n; a++) {
        if ((resp[i] & 0xC0) == 0xC0)
            i += 2;
        else {
            while (i < n && resp[i] != 0)
                i += resp[i] + 1;
            i++;
        }
        if (i + 10 > n)
            break;
        uint16_t type = (uint16_t)((resp[i] << 8) | resp[i + 1]);
        uint16_t rdlen = (uint16_t)((resp[i + 8] << 8) | resp[i + 9]);
        i += 10;
        if (type == 1 && rdlen == 4 && i + 4 <= n) {
            out_ip[0] = resp[i];
            out_ip[1] = resp[i + 1];
            out_ip[2] = resp[i + 2];
            out_ip[3] = resp[i + 3];
            printk("[dns] %s → %u.%u.%u.%u\n", name, out_ip[0], out_ip[1],
                   out_ip[2], out_ip[3]);
            return 0;
        }
        i += rdlen;
    }
    printk("[dns] no A record\n");
    return -1;
}
