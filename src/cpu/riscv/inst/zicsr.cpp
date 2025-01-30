#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"

#include "./local-decoder.h"

#include <functional>

#define CSR unsigned int csrAddr = BITS(31, 20)
#define RD  unsigned int rd  = BITS(11, 7)
#define RS1 unsigned int rs1 = BITS(19, 15)
#define IMM word_t imm = BITS(19, 15)

#define REQUIRE_WRITABLE do {if (IS_CSR_READ_ONLY(csrAddr)) {do_invalid_inst(); return;}} while(0);
#define CHECK_SUCCESS do{if (!s) {do_invalid_inst(); return;}} while(0);

using namespace kxemu::cpu;

void RVCore::init_csr() {
    this->csr.init(0, std::bind(&RVCore::get_uptime, this));
    this->csr.init_callbacks(this, [](void *core) {((RVCore *)core)->update_stimecmp();});
    
    this->mstatus = this->csr.get_csr_ptr(CSR_MSTATUS);
    this->medeleg = this->csr.get_csr_ptr_readonly(CSR_MEDELEG);
    this->mideleg = this->csr.get_csr_ptr_readonly(CSR_MIDELEG);
    this->mie     = this->csr.get_csr_ptr_readonly(CSR_MIE);
    this->mip     = this->csr.get_csr_ptr(CSR_MIP);

    // RV32 only
#ifdef KXEMU_ISA32
    this->medelegh= this->csr.get_csr_ptr_readonly(CSR_MEDELEGH);
#endif

    this->satp    = this->csr.get_csr_ptr_readonly(CSR_SATP);
}

word_t RVCore::read_csr(unsigned int addr, bool &valid) {
    return this->csr.read_csr(addr, valid);
}

bool RVCore::write_csr(unsigned int addr, word_t value) {
    return this->csr.write_csr(addr, value);
}

void RVCore::do_csrrw() {
    CSR; RD; RS1;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->read_csr(csrAddr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    s = this->write_csr(csrAddr, this->get_gpr(rs1));
    CHECK_SUCCESS;
}

void RVCore::do_csrrs() {
    CSR; RD; RS1;
    bool s;
    word_t value = this->read_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csrAddr, value | this->get_gpr(rs1));
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrc() {
    CSR; RD; RS1;
    bool s;
    word_t value = this->read_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (rs1 != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csrAddr, value & (~this->get_gpr(rs1)));
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrwi() {
    CSR; RD; IMM;
    
    REQUIRE_WRITABLE;
    
    bool s;
    if (rd != 0) {
        word_t value = this->read_csr(csrAddr, s);
        CHECK_SUCCESS;
        this->set_gpr(rd, value);
    }

    s = this->write_csr(csrAddr, imm);
    CHECK_SUCCESS;
}

void RVCore::do_csrrsi() {
    CSR; RD; IMM;
    bool s;
    word_t value = this->read_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csrAddr, value | imm);
        CHECK_SUCCESS;
    }
}

void RVCore::do_csrrci() {
    CSR; RD; IMM;
    bool s;
    word_t value = this->read_csr(csrAddr, s);
    CHECK_SUCCESS;
    this->set_gpr(rd, value);
    if (imm != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(csrAddr, value & (~imm));
        CHECK_SUCCESS;
    }
}
