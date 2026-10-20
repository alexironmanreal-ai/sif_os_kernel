#include "net.h"
#include "printk.h"
#include "string.h"

int http_get(const char *host, const char *path, char *out, uint16_t out_max) {
    if (!host || !path || !out || out_max < 16)
        return -1;

    uint8_t ip[4];
    /* host is IP or name */
    if (net_parse_ip(host, ip) < 0) {
        if (dns_resolve(host, ip) < 0)
            return -1;
    }

    if (tcp_connect(ip, 80) < 0)
        return -1;

    char req[256];
    int n = 0;
    const char *p;
#define APP(s)                                                                 \
    do {                                                                       \
        p = (s);                                                               \
        while (*p && n < 250)                                                  \
            req[n++] = *p++;                                                   \
    } while (0)

    APP("GET ");
    APP(path);
    APP(" HTTP/1.0\r\nHost: ");
    APP(host);
    APP("\r\nConnection: close\r\n\r\n");
    req[n] = 0;

    if (tcp_send(req, (uint16_t)n) < 0) {
        tcp_close();
        return -1;
    }

    uint16_t total = 0;
    out[0] = 0;
    for (int round = 0; round < 40 && total < out_max - 1; round++) {
        int r = tcp_recv((uint8_t *)out + total, (uint16_t)(out_max - 1 - total),
                         5);
        if (r > 0)
            total = (uint16_t)(total + r);
        else if (r == 0 && !tcp_connected())
            break;
    }
    out[total] = 0;
    tcp_close();
    printk("[http] %u bytes\n", total);
    return (int)total;
}
