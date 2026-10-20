#ifndef SIF_MOUSE_H
#define SIF_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

struct mouse_state {
    int x;
    int y;
    int dx;
    int dy;
    int buttons;
    int left_pressed;
    int left_released;
};

void mouse_init(void);
void mouse_get(struct mouse_state *out);
void mouse_clear_edges(void);
int mouse_moved(void);

/* call from desk loop — works even if IRQ12 is silent */
void mouse_poll(void);

/* feed a byte from PS/2 (IRQ or keyboard demux) */
void mouse_feed_byte(uint8_t data);
int mouse_awaiting(void); /* mid 3-byte packet */

/* software nudge (WASD) */
void mouse_nudge(int dx, int dy);
void mouse_click_left(void);

#endif
