#include "mouse.h"
#include "io.h"
#include "irq.h"
#include "printk.h"

#define PS2_DATA 0x60
#define PS2_STAT 0x64
#define SCREEN_W 320
#define SCREEN_H 200
#define PS2_TIMEOUT 100000

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

/* Drain one byte from mouse stream (called from IRQ12) */
static void mouse_handle_byte(uint8_t data) {
    /* ignore ACKs mid-stream */
    if (data == 0xFA || data == 0xAA)
        return;

    if (cycle == 0 && !(data & 0x08)) {
        /* out of sync — drop until bit3 set */
        return;
    }

    packet[cycle++] = data;
    if (cycle < 3)
        return;
    cycle = 0;

    /* overflow bits */
    if (packet[0] & 0xC0)
        return;

    int rel_x = (int8_t)packet[1];
    int rel_y = (int8_t)packet[2];

    mx += rel_x;
    my -= rel_y; /* PS/2 Y is opposite to screen */
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

static void mouse_callback(void) {
    /*
     * On IRQ12 always consume available bytes.
     * Some QEMU builds do not set status AUX bit (0x20).
     */
    for (int n = 0; n < 8; n++) {
        uint8_t status = inb(PS2_STAT);
        if (!(status & 1))
            break;
        uint8_t data = inb(PS2_DATA);
        mouse_handle_byte(data);
    }
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

    /* enable aux device interface */
    if (ps2_write_cmd(0xA8) < 0) {
        printk("[mouse] skip (A8)\n");
        return;
    }

    /* controller config: IRQ1+IRQ12, clocks on */
    if (ps2_write_cmd(0x20) < 0 || ps2_read_data(&tmp) < 0) {
        printk("[mouse] skip (cfg)\n");
        return;
    }
    tmp |= 0x03;           /* IRQ1 + IRQ12 */
    tmp &= (uint8_t)~0x30; /* enable kbd+mouse clocks */
    tmp &= (uint8_t)~0x20; /* ensure mouse clock not disabled */
    if (ps2_write_cmd(0x60) < 0 || ps2_write_data(tmp) < 0) {
        printk("[mouse] skip (cfg write)\n");
        return;
    }

    /* reset mouse */
    mouse_write(0xFF);
    (void)ps2_read_data(&tmp); /* ACK */
    (void)ps2_read_data(&tmp); /* BAT */
    (void)ps2_read_data(&tmp); /* device id */

    /* defaults + enable reporting */
    mouse_write(0xF6);
    (void)ps2_read_data(&tmp);
    mouse_write(0xF4);
    (void)ps2_read_data(&tmp);

    ps2_flush();
    cycle = 0;
    irq_install_handler(12, mouse_callback);
    mouse_ok = 1;
    printk("[mouse] PS/2 OK — click VGA window to capture\n");
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
    moved = 0;
}

int mouse_moved(void) { return moved; }
