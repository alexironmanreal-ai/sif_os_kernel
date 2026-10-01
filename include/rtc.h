#ifndef SIF_RTC_H
#define SIF_RTC_H
#include <stdint.h>
struct rtc_time {
    uint8_t second, minute, hour;
    uint8_t day, month;
    uint16_t year;
};
void rtc_init(void);
void rtc_read(struct rtc_time *t);
void rtc_print(void);
#endif
