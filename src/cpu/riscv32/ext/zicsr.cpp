#include "cpu/riscv32/core.h"
#include "cpu/riscv32/csr-def.h"

#define INSTPAT(pat, name) this->decoder.add(pat, &RV32Core::do_##name)
#define BITS(hi, lo) sub_bits(this->inst, hi, lo)
#define CSR int csrAddr = BITS(31, 20)
#define RD  int rd  = BITS(11, 7)
#define RS1 int rs1 = BITS(19, 15)
#define IMM word_t imm = BITS(19, 15)

#define REQUIRE_WRITABLE do {if (IS_CSR_READ_ONLY(csrAddr)) {do_invalid_inst(); return;}} while(0);
#define CHECK_SUCCESS do{if (!s) {do_invalid_inst(); return;}} while(0);

static inline uint32_t sub_bits(uint64_t bits, int hi, int lo) {
    return (bits >> lo) & ((1 << (hi - lo + 1)) - 1);
}

using namespace kxemu::cpu;

void RV32Core::init_csr() {
    this->csr.init(0);
    this->mepc    = this->csr.get_csr_ptr(CSR_MEPC);
    this->mtvec   = this->csr.get_csr_ptr_readonly(CSR_MTVEC);
    this->mcause  = this->csr.get_csr_ptr(CSR_MCAUSE);
    this->mstatus = this->csr.get_csr_ptr(CSR_MSTATUS);
    this->mtval   = this->csr.get_csr_ptr(CSR_MTVAL);
    this->medeleg = this->csr.get_csr_ptr_readonly(CSR_MEDELEG);
    this->medelegh= this->csr.get_csr_ptr_readonly(CSR_MEDELEGH);
    this->mideleg = this->csr.get_csr_ptr_readonly(CSR_MIDELEG);
    this->mie     = this->csr.get_csr_ptr_readonly(CSR_MIE);
    this->mip     = this->csr.get_csr_ptr(CSR_MIP);

    this->sepc    = this->csr.get_csr_ptr(CSR_SEPC);
    this->stvec   = this->csr.get_csr_ptr_readonly(CSR_STVEC);
    this->scause  = this->csr.get_csr_ptr(CSR_SCAUSE);
    this->stval   = this->csr.get_csr_ptr(CSR_STVAL);
}

RV32Core::word_t RV32Core::get_csr(unsigned int addr, bool &success) {
    return this->csr.get_csr(addr, success);
}

void RV32Core::set_csr(unsigned int addr, word_t value, bool &success) {
    this->csr.set_csr(addr, value, success);
}

void RV32Core::do_csrrw() {
    CSR; RD; RS1;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->get_csr(csrAddr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    this->set_csr(csrAddr, this->get_gpr(rs1), s);
    CHECK_SUCCESS;
}

void RV32Core::do_csrrs() {
    CSR; RD; RS1;
    bool s;
    word_t value = this->get_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        this->set_csr(csrAddr, value | this->get_gpr(rs1), s);
        CHECK_SUCCESS;
    }
}

void RV32Core::do_csrrc() {
    CSR; RD; RS1;
    bool s;
    word_t value = this->get_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        this->set_csr(csrAddr, value & (~this->get_gpr(rs1)), s);
        CHECK_SUCCESS;
    }
}

void RV32Core::do_csrrwi() {
    CSR; RD; IMM;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->csr.get_csr(csrAddr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    this->set_csr(csrAddr, imm, s);
    CHECK_SUCCESS;
}

void RV32Core::do_csrrsi() {
    CSR; RD; IMM;
    bool s;
    word_t value = this->get_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        this->set_csr(csrAddr, value | imm, s);
        CHECK_SUCCESS;
    }
}

void RV32Core::do_csrrci() {
    CSR; RD; IMM;
    bool s;
    word_t value = this->get_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        this->set_csr(csrAddr, value & (~imm), s);
        CHECK_SUCCESS;
    }
}
