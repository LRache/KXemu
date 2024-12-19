#include "isa/riscv32/core.h"
#include "isa/riscv32/privileged-def.h"

void RV32Core::trap(word_t code) {
    this->npc = *this->mtvec;
    *this->mepc = this->pc;
    *this->mcause = code & ~CAUSE_INTERRUPT;
}

void RV32Core::do_ecall() {
    // ecall trap
    trap(11); 
}

// TODO: Implement the do_mret function
void RV32Core::do_mret() {
    this->npc = *this->mepc;
}

void RV32Core::do_ebreak() {
    INFO("EBREAK at pc=" FMT_WORD, this->pc);
    this->state = BREAK;
    this->haltCode = this->gpr[10];
    this->haltPC = this->pc;
    
    // breakpoint trap
    this->trap(3); 
}
