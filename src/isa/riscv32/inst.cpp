#include "isa/riscv32/core.h"
#include "log.h"

#define INSTPAT(pat, name) this->decoder.add(pat, &RV32Core::do_##name)
#define RD  int rd  = get_rd (this->inst);
#define RS1 int rs1 = get_rs1(this->inst);
#define RS2 int rs2 = get_rs2(this->inst); 
#define IMM_I int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 20), 12);
#define IMM_S int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 25) << 5 | sub_bits(this->inst, 11, 7), 12);
#define IMM_B int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 31) << 12 | sub_bits(this->inst, 30, 25) << 5 | sub_bits(this->inst, 11, 8) << 1 | sub_bits(this->inst, 7, 7) << 11, 13);
#define IMM_J int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 31) << 20 | sub_bits(this->inst, 30, 21) << 1 | sub_bits(this->inst, 20, 20) << 11 | sub_bits(this->inst, 19, 12) << 12, 21);
#define IMM_U int32_t imm = (int32_t)sub_bits(this->inst, 31, 12) << 12;

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
    INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub);
    INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and);
    INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or);
    INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor);
    INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll);
    INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl);
    INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra);
    INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt);
    INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu);
    
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi);
    INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi);
    INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori);
    INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori);
    INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli);
    INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli);
    INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai);
    INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti);
    INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu);

    INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb);
    INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu);
    INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh);
    INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu);
    INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw);

    INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb);
    INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh);
    INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw);

    INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq);
    INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne);
    INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge);
    INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu);
    INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt);
    INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu);

    INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr);
    INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal);

    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc);
    INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui);

    // M extension
    INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul);
    INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh);
    INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu);
    INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu);
    INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div);
    INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu);
    INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem);
    INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu);

    INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak);
}

void RV32Core::do_add() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] + this->gpr[rs2]);
}

void RV32Core::do_sub() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] - this->gpr[rs2]);
}

void RV32Core::do_and() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] & this->gpr[rs2]);
}

void RV32Core::do_or() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] | this->gpr[rs2]);
}

void RV32Core::do_xor() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] ^ this->gpr[rs2]);
}

void RV32Core::do_sll() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] << (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_srl() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_sra() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_slt() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2] ? 1 : 0);
}

void RV32Core::do_sltu() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] < this->gpr[rs2] ? 1 : 0);
}

void RV32Core::do_addi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] + imm);        
}

void RV32Core::do_andi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] & imm);
}

void RV32Core::do_ori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] | imm);
}

void RV32Core::do_xori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] ^ imm);
}

void RV32Core::do_slli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] << (imm & 0x1F));
}

void RV32Core::do_srli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] >> (imm & 0x1F));
}

void RV32Core::do_srai() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (imm & 0x1F));
}

void RV32Core::do_slti() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)imm ? 1 : 0);
}

void RV32Core::do_sltiu() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] < (word_t)imm ? 1 : 0);
}

void RV32Core::do_lb() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (int8_t)this->memory->read(this->gpr[rs1] + imm, 1));
}

void RV32Core::do_lbu() {
    RD; RS1; IMM_I;
    word_t data = this->memory->read(this->gpr[rs1] + imm, 1);
    this->set_gpr(rd, data);
}

void RV32Core::do_lh() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (int16_t)this->memory->read(this->gpr[rs1] + imm, 2));
}

void RV32Core::do_lhu() {
    RD; RS1; IMM_I;
    word_t data = this->memory->read(this->gpr[rs1] + imm, 2);
    this->set_gpr(rd, data);
}

void RV32Core::do_lw() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory->read(this->gpr[rs1] + imm, 4));
}

void RV32Core::do_sb() {
    RS1; RS2; IMM_S;
    this->memory->write(this->gpr[rs1] + imm, this->gpr[rs2], 1);
}

void RV32Core::do_sh() {
    RS1; RS2; IMM_S;
    this->memory->write(this->gpr[rs1] + imm, this->gpr[rs2], 2);
}

void RV32Core::do_sw() {
    RS1; RS2; IMM_S;
    this->memory->write(this->gpr[rs1] + imm, this->gpr[rs2], 4);
}

void RV32Core::do_beq() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] == this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bge() {
    RS1; RS2; IMM_B;
    if ((sword_t)this->gpr[rs1] >= (sword_t)this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bgeu() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] >= this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_blt() {
    RS1; RS2; IMM_B;
    if ((sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bltu() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] < this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bne() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] != this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_jal() {
    RD; IMM_J;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->pc + imm;
    INFO("JAL: pc=0x%08x, npc=0x%08x", this->pc, this->npc);
}

void RV32Core::do_jalr() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->gpr[rs1] + imm;
}

void RV32Core::do_lui() {
    RD; IMM_U;
    this->set_gpr(rd, imm);
}

void RV32Core::do_auipc() {
    RD; IMM_U;
    DEBUG("AUIPC: pc=0x%08x, imm=0x%08x", this->pc, imm);
    this->set_gpr(rd, this->pc + imm);
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

void RV32Core::do_ebreak() {
    INFO("EBREAK at pc=0x%08x", this->pc);
    this->state = BREAK;
    this->trapCode = this->gpr[10];
    this->trapPC = this->pc;
}
