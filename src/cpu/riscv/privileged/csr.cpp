#include "cpu/riscv/core.h"
#include "cpu/riscv/csr-field.h"
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

word_t RVCore::get_csr_core(CSRAddr addr) {
    bool valid;
    word_t t = this->csr.read_csr(addr, valid);
    SELF_PROTECT(valid, "Get csr failed.");
    return t;
}

void RVCore::set_csr_core(CSRAddr addr, word_t value) {
    bool valid = this->csr.write_csr(addr, value);
    SELF_PROTECT(valid, "Set csr failed.");
}

void RVCore::update_mstatus() {
    const csr::MStatus mstatus = this->get_csr_core(CSRAddr::MSTATUS);
    this->mstatus.mie = mstatus.mie();
    this->mstatus.sie = mstatus.sie();
    this->mstatus.sum = mstatus.sum();
}
