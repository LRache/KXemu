#include "klib.h"
#include "isa/riscv.h"

static int spinlock = 0;
static int sum = 0;
static int c = 4;

int main() {
    unsigned int hartid = csrr_mhartid();
    unsigned int s = 0;
    for (unsigned int i = hartid * 25 + 1; i <= (hartid + 1) * 25; i++) {
        s += i;
    }
    
    while (__sync_lock_test_and_set(&spinlock, 1));
    sum += s;
    c --;
    spinlock = 0;

    if (c == 0) {
        printf("sum = %d, core=%u\n", sum, hartid);
    }
    
    return 0;
}
