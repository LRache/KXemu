void __trap(int code) {
    __asm__ volatile("mv a0, %0;"
                     "ebreak"
                     : : "r"(code));
}