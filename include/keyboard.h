#ifndef SIF_KEYBOARD_H
#define SIF_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void keyboard_init(void);
int  keyboard_read_char(void);
bool keyboard_has_input(void);

#endif
