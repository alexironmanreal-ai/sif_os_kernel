#ifndef SIF_TIMER_H
#define SIF_TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency_hz);
uint32_t timer_ticks(void);
uint32_t timer_seconds(void);
void timer_wait(uint32_t ticks);

#endif
