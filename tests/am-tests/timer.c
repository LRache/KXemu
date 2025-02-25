#include "klib.h"

uint64_t __am_timer_uptime();

int main() {
    printf("%lld\n", __am_timer_uptime());
    return 0;
}
