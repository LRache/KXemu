#include "isa/riscv.h"
#include <stdint.h>

uint32_t csrr_mstatus() {
    uint32_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}

void csrw_mstatus(uint32_t value) {
    asm volatile("csrw mstatus, %0" : : "r"(value));
}

uint32_t csrr_mcause() {
    uint32_t x;
    asm volatile("csrr %0, mcause" : "=r"(x));
    return x;
}

void csrw_mcause(uint32_t value) {
    asm volatile("csrw mcause, %0" : : "r"(value));
}

uint32_t csrr_mepc() {
    uint32_t x;
    asm volatile("csrr %0, mepc" : "=r"(x));
    return x;
}

void csrw_mepc(uint32_t value) {
    asm volatile("csrw mepc, %0" : : "r"(value));
}

void mret() {
    asm volatile("mret");
}

void csrw_sepc(uint32_t value) {
    asm volatile("csrw sepc, %0" : : "r"(value));
}

void sret() {
    asm volatile("sret");
}
