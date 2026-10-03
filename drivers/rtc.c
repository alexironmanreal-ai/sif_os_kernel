#include "rtc.h"
#include "io.h"
#include "printk.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t v) {
    return (v & 0x0F) + ((v >> 4) * 10);
}

void rtc_init(void) {
    printk("[rtc] CMOS RTC listo\n");
}

void rtc_read(struct rtc_time *t) {
    while (cmos_read(0x0A) & 0x80) {}
    uint8_t sec = cmos_read(0x00);
    uint8_t min = cmos_read(0x02);
    uint8_t hr  = cmos_read(0x04);
    uint8_t day = cmos_read(0x07);
    uint8_t mon = cmos_read(0x08);
    uint8_t yr  = cmos_read(0x09);
    uint8_t cent = cmos_read(0x32);
    uint8_t regb = cmos_read(0x0B);
    if (!(regb & 0x04)) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hr  = bcd_to_bin(hr);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        yr  = bcd_to_bin(yr);
        cent = bcd_to_bin(cent);
    }
    t->second = sec;
    t->minute = min;
    t->hour = hr;
    t->day = day;
    t->month = mon;
    if (cent >= 19 && cent <= 21)
        t->year = cent * 100 + yr;
    else
        t->year = 2000 + yr;
}

void rtc_print(void) {
    struct rtc_time t;
    rtc_read(&t);
    printk("%04u-%02u-%02u %02u:%02u:%02u\n",
           t.year, t.month, t.day, t.hour, t.minute, t.second);
}
