#include "keyboard.h"
#include "mouse.h"
#include "io.h"
#include "irq.h"
#include "printk.h"

#define KBD_DATA 0x60
#define KBD_STAT 0x64
#define BUF_SIZE 256

static volatile char ring[BUF_SIZE];
static volatile uint32_t head = 0, tail = 0;
static int shift = 0;
static int e0 = 0;

static const char keymap[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
};
static const char keymap_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ',
};

static void kbd_push(char c) {
    uint32_t next = (head + 1) % BUF_SIZE;
    if (next == tail)
        return;
    ring[head] = c;
    head = next;
}

static void kbd_handle_scancode(uint8_t sc) {
    if (sc == 0xFA || sc == 0xFE || sc == 0xAA)
        return;

    if (sc == 0xE0) {
        e0 = 1;
        return;
    }

    if (sc & 0x80) {
        uint8_t code = sc & 0x7F;
        if (code == 0x2A || code == 0x36)
            shift = 0;
        e0 = 0;
        return;
    }

    if (e0) {
        e0 = 0;
        /* ignore extended arrows for now */
        return;
    }

    if (sc == 0x2A || sc == 0x36) {
        shift = 1;
        return;
    }

    char c = 0;
    if (sc < 128)
        c = shift ? keymap_shift[sc] : keymap[sc];
    if (c)
        kbd_push(c);
}

static void keyboard_callback(void) {
    for (int n = 0; n < 16; n++) {
        uint8_t status = inb(KBD_STAT);
        if (!(status & 1))
            break;

        /* Only mouse if controller says AUX — never guess via cycle */
        if (status & 0x20) {
            mouse_feed_byte(inb(KBD_DATA));
            continue;
        }

        kbd_handle_scancode(inb(KBD_DATA));
    }
}

void keyboard_poll(void) {
    for (int n = 0; n < 16; n++) {
        uint8_t status = inb(KBD_STAT);
        if (!(status & 1))
            break;
        if (status & 0x20) {
            mouse_feed_byte(inb(KBD_DATA));
            continue;
        }
        kbd_handle_scancode(inb(KBD_DATA));
    }
}

void keyboard_init(void) {
    head = tail = 0;
    shift = 0;
    e0 = 0;

    /* flush */
    while (inb(KBD_STAT) & 1)
        (void)inb(KBD_DATA);

    /* enable keyboard interface + IRQ1 on controller (safe defaults) */
    /* command byte: read */
    outb(KBD_STAT, 0x20);
    for (volatile int i = 0; i < 10000; i++)
        ;
    uint8_t cb = 0;
    if (inb(KBD_STAT) & 1)
        cb = inb(KBD_DATA);
    cb |= 0x01;  /* IRQ1 enable */
    cb &= (uint8_t)~0x10; /* keyboard clock enable */
    outb(KBD_STAT, 0x60);
    for (volatile int i = 0; i < 10000; i++)
        ;
    outb(KBD_DATA, cb);

    /* enable scanning */
    outb(KBD_DATA, 0xF4);
    for (volatile int i = 0; i < 10000; i++)
        ;
    if (inb(KBD_STAT) & 1)
        (void)inb(KBD_DATA); /* ACK */

    irq_install_handler(1, keyboard_callback);
    printk("[kbd] listo\n");
}

bool keyboard_has_input(void) {
    keyboard_poll();
    return head != tail;
}

int keyboard_read_char(void) {
    keyboard_poll();
    if (head == tail)
        return 0;
    char c = ring[tail];
    tail = (tail + 1) % BUF_SIZE;
    return (unsigned char)c;
}
