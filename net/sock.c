#include "net.h"
#include "string.h"

#define SK_MAX 8

struct sk {
    int used;
    int type;
    uint16_t lport;
    uint8_t rip[4];
    uint16_t rport;
    int connected;
};

static struct sk sks[SK_MAX];
static uint16_t ephemeral = 41000;

int sk_socket(int type) {
    if (type != SK_UDP && type != SK_TCP)
        return -1;
    for (int i = 0; i < SK_MAX; i++) {
        if (!sks[i].used) {
            memset(&sks[i], 0, sizeof(sks[i]));
            sks[i].used = 1;
            sks[i].type = type;
            sks[i].lport = ephemeral++;
            return i;
        }
    }
    return -1;
}

int sk_bind(int fd, uint16_t port) {
    if (fd < 0 || fd >= SK_MAX || !sks[fd].used)
        return -1;
    sks[fd].lport = port;
    return 0;
}

int sk_connect(int fd, const uint8_t ip[4], uint16_t port) {
    if (fd < 0 || fd >= SK_MAX || !sks[fd].used || !ip)
        return -1;
    memcpy(sks[fd].rip, ip, 4);
    sks[fd].rport = port;
    if (sks[fd].type == SK_TCP) {
        if (tcp_connect(ip, port) < 0)
            return -1;
        sks[fd].connected = 1;
    } else
        sks[fd].connected = 1;
    return 0;
}

int sk_send(int fd, const void *buf, uint16_t len) {
    if (fd < 0 || fd >= SK_MAX || !sks[fd].used || !sks[fd].connected)
        return -1;
    if (sks[fd].type == SK_UDP)
        return udp_send(sks[fd].rip, sks[fd].lport, sks[fd].rport, buf, len);
    return tcp_send(buf, len);
}

int sk_recv(int fd, void *buf, uint16_t max) {
    if (fd < 0 || fd >= SK_MAX || !sks[fd].used || !buf)
        return -1;
    if (sks[fd].type == SK_UDP) {
        uint8_t sip[4];
        uint16_t sport;
        return udp_recv(sks[fd].lport, (uint8_t *)buf, max, sip, &sport, 20);
    }
    return tcp_recv((uint8_t *)buf, max, 20);
}

void sk_close(int fd) {
    if (fd < 0 || fd >= SK_MAX || !sks[fd].used)
        return;
    if (sks[fd].type == SK_TCP && sks[fd].connected)
        tcp_close();
    sks[fd].used = 0;
}
