#include "isa/riscv/core.h"
#include "log.h"
#include <cstdint>

#define INSTPAT(pat, name) this->decoder.add(pat, &RV32Core::do_##name)
#define RD int rd = get_rd(this->inst);
#define RS1 int rs1 = get_rs1(this->inst);
#define RS2 int rs2 = get_rs2(this->inst); 

static inline uint32_t sub_bits(uint64_t bits, int hi, int lo) {
    return (bits >> lo) & ((1 << (hi - lo + 1)) - 1);
}

static inline int get_rd(uint32_t inst) {
    return sub_bits(inst, 11, 7);
}

static inline int get_rs1(uint32_t inst) {
    return sub_bits(inst, 19, 15);
}

static inline int get_rs2(uint32_t inst) {
    return sub_bits(inst, 24, 20);
}

static inline int32_t sign_extend(uint32_t bits, int from) {
    int shift = 32 - from;
    return (int32_t)(bits << shift) >> shift;
}

void RV32Core::init_decoder() {
    this->decoder.init(this);

    INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add);
    
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi);

    INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak);
}

void RV32Core::do_add() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] + this->gpr[rs2]);
    this->npc = this->pc + 4;
}

void RV32Core::do_addi() {
    RD;
    RS1;
    int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 20), 11);
    
    this->set_gpr(rd, this->gpr[rs1] + imm);
    this->npc = this->pc + 4;
}

void RV32Core::do_ebreak() {
    INFO("EBREAK at pc=0x%08x", this->pc);
    this->state = BREAK;
    this->trapCode = this->gpr[10];
    this->trapPC = this->pc;
    this->npc = this->pc + 4;
}
