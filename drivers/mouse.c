#include "mouse.h"
#include "io.h"
#include "irq.h"
#include "printk.h"

#define PS2_DATA 0x60
#define PS2_STAT 0x64

#define SCREEN_W 320
#define SCREEN_H 200

static volatile int mx = 160, my = 100;
static volatile int mdx, mdy;
static volatile int buttons;
static volatile int left_edge_p, left_edge_r;
static volatile int moved;
static volatile int cycle;
static volatile uint8_t packet[3];
static int prev_left;

static void ps2_wait_write(void) {
    for (int i = 0; i < 100000; i++)
        if (!(inb(PS2_STAT) & 2))
            return;
}

static void ps2_wait_read(void) {
    for (int i = 0; i < 100000; i++)
        if (inb(PS2_STAT) & 1)
            return;
}

static void ps2_write_cmd(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_STAT, cmd);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_write();
    outb(PS2_DATA, data);
}

static uint8_t ps2_read_data(void) {
    ps2_wait_read();
    return inb(PS2_DATA);
}

static void mouse_write(uint8_t data) {
    ps2_write_cmd(0xD4);
    ps2_write_data(data);
}

static void mouse_callback(void) {
    uint8_t status = inb(PS2_STAT);
    if (!(status & 1))
        return;
    /* Only consume if AUX (mouse) bit set; else leave for keyboard */
    if (!(status & 0x20))
        return;

    uint8_t data = inb(PS2_DATA);

    if (cycle == 0 && !(data & 0x08))
        return; /* resync: bit3 must be 1 in first byte */

    packet[cycle++] = data;
    if (cycle < 3)
        return;
    cycle = 0;

    int rel_x = (int8_t)packet[1];
    int rel_y = (int8_t)packet[2];
    /* Y is inverted on PS/2 vs screen */
    if (packet[0] & 0x10)
        rel_x |= ~0xFF; /* already sign via int8_t */
    if (packet[0] & 0x20)
        rel_y |= ~0xFF;

    (void)rel_x;
    rel_x = (int8_t)packet[1];
    rel_y = (int8_t)packet[2];

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

void mouse_init(void) {
    cycle = 0;
    mx = 160;
    my = 100;
    buttons = 0;
    prev_left = 0;
    left_edge_p = left_edge_r = 0;
    moved = 0;

    /* enable auxiliary device */
    ps2_write_cmd(0xA8);

    /* status / compaq byte: enable IRQ12 + mouse clock */
    ps2_write_cmd(0x20);
    uint8_t status = ps2_read_data();
    status |= 0x02;  /* enable IRQ12 */
    status &= ~0x20; /* enable mouse clock */
    ps2_write_cmd(0x60);
    ps2_write_data(status);

    /* defaults + enable streaming */
    mouse_write(0xF6);
    (void)ps2_read_data(); /* ACK */
    mouse_write(0xF4);
    (void)ps2_read_data();

    irq_install_handler(12, mouse_callback);
    printk("[mouse] PS/2 listo (IRQ12)\n");
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
