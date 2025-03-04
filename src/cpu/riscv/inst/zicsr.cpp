#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"

#include "./local-decoder.h"
#include "cpu/word.h"
#include "debug.h"

// #define CSR unsigned int csrAddr = BITS(31, 20)
// #define RD  unsigned int rd  = BITS(11, 7)
// #define RS1 unsigned int rs1 = BITS(19, 15)
// #define IMM word_t imm = BITS(19, 15)

// #define CSR unsigned int csrAddr = decodeInfo.csr
// #define RD  unsigned int rd  = decodeInfo.rd
// #define RS1 unsigned int rs1 = decodeInfo.rs1
// #define IMM word_t imm = decodeInfo.imm

#define REQUIRE_WRITABLE do {if (IS_CSR_READ_ONLY(csr)) {do_invalid_inst(); return;}} while(0);
#define CHECK_SUCCESS do{if (!s) {do_invalid_inst(); return;}} while(0);

using namespace kxemu::cpu;

void RVCore::do_csrrw(const DecodeInfo &decodeInfo) {
    CSR; RD; RS1;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->read_csr(csr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    s = this->write_csr(csr, this->get_gpr(rs1));
    CHECK_SUCCESS;
}

void RVCore::do_csrrs(const DecodeInfo &decodeInfo) {
    CSR; RD; RS1;
    bool s;
    word_t value = this->read_csr(csr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csr, value | this->get_gpr(rs1));
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrc(const DecodeInfo &decodeInfo) {
    CSR; RD; RS1;
    bool s;
    word_t value = this->read_csr(csr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csr, value & (~this->get_gpr(rs1)));
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrwi(const DecodeInfo &decodeInfo) {
    CSR; RD; IMM;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->read_csr(csr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    s = this->write_csr(csr, imm);
    CHECK_SUCCESS;
}

void RVCore::do_csrrsi(const DecodeInfo &decodeInfo) {
    CSR; RD; IMM;
    bool s;
    word_t value = this->read_csr(csr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csr, value | imm);
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrci(const DecodeInfo &decodeInfo) {
    CSR; RD; IMM;
    bool s;
    word_t value = this->read_csr(csr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csr, value & (~imm));
        CHECK_SUCCESS;
    }
}
