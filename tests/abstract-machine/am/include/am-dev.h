#ifndef __AM_DEV_H__
#define __AM_DEV_H__

#include <stdint.h>

// uart
uint8_t __uart_rx();
void __am_uart_tx(uint8_t ch);

// timer
struct _RTC {
    unsigned int year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
};
uint64_t __am_timer_uptime();
void __am_timer_rtc(struct _RTC *rtc);

#endif
