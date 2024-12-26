#include "am-dev.h"
#include <stdint.h>

#define MTIME_BASE 0x0200BFF8
#define MTIME_LOW  (*(volatile uint32_t *)MTIME_BASE)
#define MTIME_HIGH (*(volatile uint32_t *)(MTIME_BASE + 4))

uint64_t __am_timer_uptime() {
    uint64_t uptime = MTIME_LOW;
    uptime |= (uint64_t)MTIME_HIGH << 32;
    return uptime / 10;
}

void __am_timer_rtc(struct _RTC *rtc) {
    rtc->year = 2020;
    rtc->month = 1;
    rtc->day = 1;
    rtc->hour = 0;
    rtc->minute = 0;
    rtc->second = 0;
}
