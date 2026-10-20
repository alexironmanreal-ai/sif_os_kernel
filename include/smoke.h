#ifndef SIF_SMOKE_H
#define SIF_SMOKE_H

#include <stdint.h>

/* QEMU isa-debug-exit at 0x501: write (code << 1) | 1 */
void smoke_exit(uint32_t code);
int smoke_wanted(void); /* cmdline contains smoke */
void smoke_note(const char *msg);

#endif
