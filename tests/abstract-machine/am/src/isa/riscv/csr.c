#include "isa/riscv.h"
#include <stdint.h>

word_t csrr_mstatus() {
    uint32_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}

word_t csrr_mcause() {
    uint32_t x;
    asm volatile("csrr %0, mcause" : "=r"(x));
    return x;
}

word_t csrr_mepc() {
    uint32_t x;
    asm volatile("csrr %0, mepc" : "=r"(x));
    return x;
}

word_t csrr_medeleg() {
    uint32_t x;
    asm volatile("csrr %0, medeleg" : "=r"(x));
    return x;
}

void csrw_pmpcfg0(word_t value) {
    asm volatile("csrw pmpcfg0, %0" : : "r"(value));
}

void csrw_pmpaddr0(word_t value) {
    asm volatile("csrw pmpaddr0, %0" : : "r"(value));
}

void csrw_mcause(word_t value) {
    asm volatile("csrw mcause, %0" : : "r"(value));
}

void csrw_mstatus(word_t value) {
    asm volatile("csrw mstatus, %0" : : "r"(value));
}

void csrw_mepc(word_t value) {
    asm volatile("csrw mepc, %0" : : "r"(value));
}

void csrw_mtvec(word_t value) {
    asm volatile("csrw mtvec, %0" : : "r"(value));
}

void csrw_medeleg(word_t value) {
    asm volatile("csrw medeleg, %0" : : "r"(value));
}

void csrw_mscratch(word_t value) {
    asm volatile("csrw mscratch, %0" : : "r"(value));
}

void csrw_sepc(word_t value) {
    asm volatile("csrw sepc, %0" : : "r"(value));
}

void csrw_stvec(word_t value) {
    asm volatile("csrw stvec, %0" : : "r"(value));
}

void csrw_satp(word_t value) {
    asm volatile("csrw satp, %0" : : "r"(value));
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

void wfi() {
    asm volatile("wfi");
}

void sfence_vma() {
    asm volatile("sfence.vma zero, zero");
}
