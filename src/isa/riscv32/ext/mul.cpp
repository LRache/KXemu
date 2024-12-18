#include "isa/riscv32/core.h"
#include "log.h"

#define INSTPAT(pat, name) this->decoder.add(pat, &RV32Core::do_##name)
#define RD  int rd  = get_rd (this->inst);
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

void RV32Core::do_mul() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] * this->gpr[rs2]);
}

void RV32Core::do_mulh() {
    RD; RS1; RS2;
    int64_t res = (int64_t)(int32_t)this->gpr[rs1] * (int64_t)(int32_t)this->gpr[rs2];
    this->set_gpr(rd, (int32_t)(res >> 32));
}

void RV32Core::do_mulhsu() {
    RD; RS1; RS2;
    int64_t res = (int64_t)this->gpr[rs1] * (int64_t)this->gpr[rs2];
    this->set_gpr(rd, (int32_t)(res >> 32));
}

void RV32Core::do_mulhu() {
    RD; RS1; RS2;
    uint64_t res = (uint64_t)this->gpr[rs1] * (uint64_t)this->gpr[rs2];
    this->set_gpr(rd, (uint32_t)(res >> 32));
}

void RV32Core::do_div() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (sword_t)this->gpr[rs1] / (sword_t)this->gpr[rs2]);
    }
}

void RV32Core::do_divu() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, this->gpr[rs1] / this->gpr[rs2]);
    }
}

void RV32Core::do_rem() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs1);
    } else {
        this->set_gpr(rd, (int32_t)this->gpr[rs1] % (int32_t)this->gpr[rs2]);
    }
}

void RV32Core::do_remu() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs2);
    } else {
        this->set_gpr(rd, this->gpr[rs1] % this->gpr[rs2]);
    }
}