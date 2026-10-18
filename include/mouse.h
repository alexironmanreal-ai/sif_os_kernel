#ifndef SIF_MOUSE_H
#define SIF_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

struct mouse_state {
    int x;
    int y;
    int dx;
    int dy;
    int buttons; /* bit0=left bit1=right bit2=middle */
    int left_pressed;  /* edge: just pressed */
    int left_released;
};

void mouse_init(void);
void mouse_get(struct mouse_state *out);
void mouse_clear_edges(void);
int mouse_moved(void);

#endif
