#include "isa/riscv.h"
#include <stdint.h>

uint32_t csrr_mstatus() {
    uint32_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}

uint32_t csrr_mcause() {
    uint32_t x;
    asm volatile("csrr %0, mcause" : "=r"(x));
    return x;
}

uint32_t csrr_mepc() {
    uint32_t x;
    asm volatile("csrr %0, mepc" : "=r"(x));
    return x;
}

uint32_t csrr_medeleg() {
    uint32_t x;
    asm volatile("csrr %0, medeleg" : "=r"(x));
    return x;
}

void csrw_mcause(uint32_t value) {
    asm volatile("csrw mcause, %0" : : "r"(value));
}

void csrw_mstatus(uint32_t value) {
    asm volatile("csrw mstatus, %0" : : "r"(value));
}

void csrw_mepc(uint32_t value) {
    asm volatile("csrw mepc, %0" : : "r"(value));
}

void csrw_mtvec(uint32_t value) {
    asm volatile("csrw mtvec, %0" : : "r"(value));
}

void csrw_medeleg(uint32_t value) {
    asm volatile("csrw medeleg, %0" : : "r"(value));
}

void csrw_sepc(uint32_t value) {
    asm volatile("csrw sepc, %0" : : "r"(value));
}

void csrw_stvec(uint32_t value) {
    asm volatile("csrw stvec, %0" : : "r"(value));
}

void mret() {
    asm volatile("mret");
}

void sret() {
    asm volatile("sret");
}

void ecall() {
    asm volatile("ecall");
}
