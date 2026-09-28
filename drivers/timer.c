#include "timer.h"
#include "io.h"
#include "irq.h"
#include "printk.h"

#define PIT_CH0   0x40
#define PIT_CMD   0x43
#define PIT_BASE  1193182u

static volatile uint32_t ticks = 0;

static void timer_callback(void) {
    ticks++;
}

void timer_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) frequency_hz = 100;
    uint32_t divisor = PIT_BASE / frequency_hz;
    if (divisor == 0) divisor = 1;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
    irq_install_handler(0, timer_callback);
    printk("[timer] PIT @ %u Hz (divisor=%u)\n", frequency_hz, divisor);
}

uint32_t timer_ticks(void) { return ticks; }
uint32_t timer_seconds(void) { return ticks / 100; }

void timer_wait(uint32_t n) {
    uint32_t target = ticks + n;
    while (ticks < target) {
        __asm__ volatile ("hlt");
    }
}
