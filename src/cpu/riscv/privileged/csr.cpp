#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/word.hpp"

using namespace kxemu::cpu;

void RVCore::init_csr() {
    this->csr.init(this->coreID, [this] { return this->get_uptime(); });
}

RVCSR::ModifyResult RVCore::modify_csr(unsigned int addr, word_t operand, RVCSR::ModifyOp op, bool readOld) {
    const auto result = this->csr.modify_csr(addr, operand, op, readOld);
    if (result.valid) this->apply_csr_effect(result.effect);
    return result;
}

void RVCore::apply_csr_effect(RVCSR::WriteEffect effect) {
    switch (effect) {
        case RVCSR::WriteEffect::None: break;
        case RVCSR::WriteEffect::VmTranslate: this->update_vm_translate(); break;
        case RVCSR::WriteEffect::STimer: this->update_stimecmp(); break;
    }
}
