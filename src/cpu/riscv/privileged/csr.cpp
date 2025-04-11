#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "debug.h"

#include <functional>

using namespace kxemu::cpu;

void RVCore::init_csr() {
    this->csr.init(this->coreID, std::bind(&RVCore::get_uptime, this));
    this->csr.set_write_callbacks(CSRAddr::STIMECMP, std::bind(&RVCore::update_stimecmp, this));
    this->csr.set_write_callbacks(CSRAddr::SATP    , std::bind(&RVCore::update_satp    , this));
    this->csr.set_write_callbacks(CSRAddr::MSTATUS , std::bind(&RVCore::update_mstatus , this));
    this->csr.set_write_callbacks(CSRAddr::SSTATUS , std::bind(&RVCore::update_mstatus , this));
}

word_t RVCore::read_csr(unsigned int addr, bool &valid) {
    return this->csr.read_csr(addr, valid);
}

bool RVCore::write_csr(unsigned int addr, word_t value) {
    return this->csr.write_csr(addr, value);
}

word_t RVCore::get_csr_core(unsigned int addr) {
    bool valid;
    word_t t = this->csr.read_csr(addr, valid);
    SELF_PROTECT(valid, "Get csr failed.");
    return t;
}

void RVCore::set_csr_core(unsigned int addr, word_t value) {
    bool valid = this->csr.write_csr(addr, value);
    SELF_PROTECT(valid, "Set csr failed.");
}

void RVCore::update_mstatus() {
    const word_t mstatus = this->csr.read_csr(CSRAddr::MSTATUS);
    this->mstatus.mie = STATUS_MIE(mstatus);
    this->mstatus.sie = STATUS_SIE(mstatus);
    this->mstatus.sum = STATUS_SUM(mstatus);
}
