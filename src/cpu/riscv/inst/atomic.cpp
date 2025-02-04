#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"

#include "./local-decoder.h"
#include "cpu/word.h"
#include "device/bus.h"
#include <cstdint>

using namespace kxemu::cpu;
using kxemu::device::AMO;

#define RD  unsigned int rd  = BITS(11, 7);
#define RS1 unsigned int rs1 = BITS(19, 15);
#define RS2 unsigned int rs2 = BITS(24, 20);

word_t RVCore::amo_vaddr_translate_and_set_trap(word_t vaddr, int len, bool &valid) {
    valid = false;
    
    if (vaddr & (len - 1)) {
        this->trap(TRAP_AMO_ACCESS_FAULT, vaddr);
        return -1;
    }

    word_t paddr = vaddr;
    if (unlikely(this->privMode != PrivMode::MACHINE)) {
        VMResult result;
        paddr = this->vaddr_translate(vaddr, MemType::AMO, result);
        switch (result) {
            case VM_OK: break;
            case VM_ACCESS_FAULT: this->trap(TRAP_AMO_ACCESS_FAULT); return -1;
            case VM_PAGE_FAULT:   this->trap(TRAP_AMO_PAGE_FAULT)  ; return -1;
        }

        bool pmp = this->csr.pmp_check_w(paddr, len) && this->csr.pmp_check_r(paddr, len);
        if (unlikely(!pmp)) {
            this->trap(TRAP_AMO_ACCESS_FAULT);
            valid = false;
            return -1;
        }
    }
    
    valid = true;
    return paddr;
}

template<int len>
void RVCore::do_load_reserved() {
    RD; RS1;

    bool valid;
    word_t addr = this->get_gpr(rs1);
    addr = this->amo_vaddr_translate_and_set_trap(addr, len, valid);
    if (!valid) return;

    word_t value = this->bus->read(addr, len, valid);
    if (!valid) {
        this->trap(TRAP_AMO_ACCESS_FAULT, addr);
        return;
    }

    this->reservedMemory.insert(std::make_pair(addr, value));
    this->set_gpr(rd, value);
}

template<int len>
void RVCore::do_store_conditional() {
    RD; RS1; RS2;

    bool valid;
    word_t addr = this->get_gpr(rs1);
    addr = this->amo_vaddr_translate_and_set_trap(addr, len, valid);
    if (!valid) return;

    auto iter = this->reservedMemory.find(addr);
    if (iter == this->reservedMemory.end()) {
        this->set_gpr(rd, 0);
        return;
    }

    uint8_t *ptr = this->bus->get_ptr(addr);
    if (ptr == nullptr) {
        this->trap(TRAP_AMO_ACCESS_FAULT, addr);
        return;
    }

    word_t expected = iter->second;
    word_t desired  = this->get_gpr(rs2);
    bool success = __atomic_compare_exchange_n((word_t *)ptr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    this->set_gpr(rd, success ? 0 : 1);
}

template<kxemu::device::AMO amo, typename sw_t>
void RVCore::do_amo_inst() {
    RS1; RS2; RD;
    constexpr int len = sizeof(sw_t);

    bool valid;
    word_t addr = this->get_gpr(rs1);
    addr = this->amo_vaddr_translate_and_set_trap(addr, len, valid);
    if (!valid) return;

    word_t src = this->get_gpr(rs2);
    sw_t oldValue = this->bus->do_atomic(addr, src, len, amo, valid); // signed extend
    
    if (!valid) {
        this->trap(TRAP_AMO_ACCESS_FAULT, addr);
        return;
    }

    this->set_gpr(rd, (sword_t)oldValue);
}

void RVCore::do_lr_w() {
   this->do_load_reserved<4>();
}

void RVCore::do_sc_w() {
    this->do_store_conditional<4>();
}

void RVCore::do_amoswap_w() {
    this->do_amo_inst<AMO::AMO_SWAP, int32_t>();
}

void RVCore::do_amoadd_w() {
    this->do_amo_inst<AMO::AMO_ADD, int32_t>();
}

void RVCore::do_amoxor_w() {
    this->do_amo_inst<AMO::AMO_XOR, int32_t>();
}

void RVCore::do_amoand_w() {
    this->do_amo_inst<AMO::AMO_AND, int32_t>();
}

void RVCore::do_amoor_w() {
    this->do_amo_inst<AMO::AMO_OR, int32_t>();
}

void RVCore::do_amomin_w() {
    this->do_amo_inst<AMO::AMO_MIN, int32_t>();
}

void RVCore::do_amomax_w() {
    this->do_amo_inst<AMO::AMO_MAX, int32_t>();
}

void RVCore::do_amominu_w() {
    this->do_amo_inst<AMO::AMO_MINU, int32_t>();
}

void RVCore::do_amomaxu_w() {
    this->do_amo_inst<AMO::AMO_MAXU, int32_t>();
}

#ifdef KXEMU_ISA64

void RVCore::do_lr_d() {
    this->do_load_reserved<8>();
}

void RVCore::do_sc_d() {
    this->do_store_conditional<8>();
}

void RVCore::do_amoswap_d() {
    this->do_amo_inst<AMO::AMO_SWAP, int64_t>();
}

void RVCore::do_amoadd_d() {
    this->do_amo_inst<AMO::AMO_ADD, int64_t>();
}

void RVCore::do_amoxor_d() {
    this->do_amo_inst<AMO::AMO_XOR, int64_t>();
}

void RVCore::do_amoand_d() {
    this->do_amo_inst<AMO::AMO_AND, int64_t>();
}

void RVCore::do_amoor_d() {
    this->do_amo_inst<AMO::AMO_OR, int64_t>();
}

void RVCore::do_amomin_d() {
    this->do_amo_inst<AMO::AMO_MIN, int64_t>();
}

void RVCore::do_amomax_d() {
    this->do_amo_inst<AMO::AMO_MAX, int64_t>();
}

void RVCore::do_amominu_d() {
    this->do_amo_inst<AMO::AMO_MINU, int64_t>();
}

void RVCore::do_amomaxu_d() {
    this->do_amo_inst<AMO::AMO_MAXU, int64_t>();
}

#endif
