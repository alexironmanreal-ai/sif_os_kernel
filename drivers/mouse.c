#include "mouse.h"
#include "io.h"
#include "irq.h"
#include "printk.h"

#define PS2_DATA 0x60
#define PS2_STAT 0x64
#define SCREEN_W 320
#define SCREEN_H 200
#define PS2_TIMEOUT 50000

static volatile int mx = 160, my = 100;
static volatile int mdx, mdy;
static volatile int buttons;
static volatile int left_edge_p, left_edge_r;
static volatile int moved;
static volatile int cycle;
static volatile uint8_t packet[3];
static int prev_left;
static int mouse_ok;

static int ps2_wait_write(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++)
        if (!(inb(PS2_STAT) & 2))
            return 0;
    return -1;
}

static int ps2_wait_read(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++)
        if (inb(PS2_STAT) & 1)
            return 0;
    return -1;
}

static void ps2_flush(void) {
    for (int i = 0; i < 256; i++) {
        if (!(inb(PS2_STAT) & 1))
            break;
        (void)inb(PS2_DATA);
    }
}

static int ps2_write_cmd(uint8_t cmd) {
    if (ps2_wait_write() < 0)
        return -1;
    outb(PS2_STAT, cmd);
    return 0;
}

static int ps2_write_data(uint8_t data) {
    if (ps2_wait_write() < 0)
        return -1;
    outb(PS2_DATA, data);
    return 0;
}

static int ps2_read_data(uint8_t *out) {
    if (ps2_wait_read() < 0)
        return -1;
    *out = inb(PS2_DATA);
    return 0;
}

static int mouse_write(uint8_t data) {
    if (ps2_write_cmd(0xD4) < 0)
        return -1;
    return ps2_write_data(data);
}

void mouse_feed_byte(uint8_t data) {
    if (data == 0xFA || data == 0xAA)
        return;

    /* first byte of packet must have bit3 = 1 */
    if (cycle == 0) {
        if (!(data & 0x08))
            return;
        packet[0] = data;
        cycle = 1;
        return;
    }

    packet[cycle++] = data;
    if (cycle < 3)
        return;

    /* full packet */
    cycle = 0;

    if (packet[0] & 0xC0) {
        /* X/Y overflow — drop */
        return;
    }

    int rel_x = (int8_t)packet[1];
    int rel_y = (int8_t)packet[2];

    /* clamp step so one bad packet cannot jump off-screen */
    if (rel_x > 40)
        rel_x = 40;
    if (rel_x < -40)
        rel_x = -40;
    if (rel_y > 40)
        rel_y = 40;
    if (rel_y < -40)
        rel_y = -40;

    mx += rel_x;
    my -= rel_y;
    if (mx < 0)
        mx = 0;
    if (my < 0)
        my = 0;
    if (mx >= SCREEN_W)
        mx = SCREEN_W - 1;
    if (my >= SCREEN_H)
        my = SCREEN_H - 1;

    mdx = rel_x;
    mdy = -rel_y;
    buttons = packet[0] & 0x07;

    int left = buttons & 1;
    if (left && !prev_left)
        left_edge_p = 1;
    if (!left && prev_left)
        left_edge_r = 1;
    prev_left = left;
    moved = 1;
}

int mouse_awaiting(void) { return cycle > 0; }

/* IRQ12: only drain to avoid buffer overrun; parse in poll */
static void mouse_callback(void) {
    for (int n = 0; n < 8; n++) {
        if (!(inb(PS2_STAT) & 1))
            break;
        uint8_t st = inb(PS2_STAT);
        uint8_t data = inb(PS2_DATA);
        if ((st & 0x20) || cycle > 0)
            mouse_feed_byte(data);
        /* else keyboard byte on wrong IRQ — drop to avoid stuck */
    }
}

void mouse_poll(void) {
    if (!mouse_ok)
        return;
    for (int n = 0; n < 8; n++) {
        uint8_t st = inb(PS2_STAT);
        if (!(st & 1))
            break;
        /* only take mouse / mid-packet */
        if ((st & 0x20) || cycle > 0) {
            mouse_feed_byte(inb(PS2_DATA));
        } else {
            break;
        }
    }
}

void mouse_nudge(int dx, int dy) {
    mx += dx;
    my += dy;
    if (mx < 0)
        mx = 0;
    if (my < 0)
        my = 0;
    if (mx >= SCREEN_W)
        mx = SCREEN_W - 1;
    if (my >= SCREEN_H)
        my = SCREEN_H - 1;
    mdx = dx;
    mdy = dy;
    moved = 1;
}

void mouse_click_left(void) {
    left_edge_p = 1;
    buttons |= 1;
    moved = 1;
}

void mouse_init(void) {
    uint8_t tmp;

    cycle = 0;
    mx = 160;
    my = 100;
    buttons = 0;
    prev_left = 0;
    left_edge_p = left_edge_r = 0;
    moved = 0;
    mouse_ok = 0;

    printk("[mouse] init...\n");
    ps2_flush();

    if (ps2_write_cmd(0xA8) < 0) {
        printk("[mouse] soft-only\n");
        mouse_ok = 1;
        return;
    }

    if (ps2_write_cmd(0x20) == 0 && ps2_read_data(&tmp) == 0) {
        tmp |= 0x03;
        tmp &= (uint8_t)~0x30;
        ps2_write_cmd(0x60);
        ps2_write_data(tmp);
    }

    mouse_write(0xFF);
    (void)ps2_read_data(&tmp);
    (void)ps2_read_data(&tmp);
    (void)ps2_read_data(&tmp);

    mouse_write(0xF6);
    (void)ps2_read_data(&tmp);
    mouse_write(0xF4);
    (void)ps2_read_data(&tmp);

    ps2_flush();
    cycle = 0;
    irq_install_handler(12, mouse_callback);
    mouse_ok = 1;
    printk("[mouse] ready — WASD or move\n");
}

void mouse_get(struct mouse_state *out) {
    if (!out)
        return;
    out->x = mx;
    out->y = my;
    out->dx = mdx;
    out->dy = mdy;
    out->buttons = buttons;
    out->left_pressed = left_edge_p;
    out->left_released = left_edge_r;
}

void mouse_clear_edges(void) {
    left_edge_p = 0;
    left_edge_r = 0;
    mdx = mdy = 0;
    /* do NOT clear moved here if still drawing — caller clears after draw */
    moved = 0;
}

int mouse_moved(void) { return moved; }
