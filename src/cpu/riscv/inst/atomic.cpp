#include "cpu/riscv/core.h"
#include "cpu/word.h"
#include "device/bus.h"
#include "log.h"

#include <cstdint>
#include <optional>

#include "./local-decoder.h"

using namespace kxemu::cpu;
using kxemu::device::AMO;

word_t RVCore::amo_vaddr_translate_and_set_trap(word_t vaddr, int len, bool &valid) {
    valid = false;
    
    if (vaddr & (len - 1)) {
        this->trap(TrapCode::AMO_ACCESS_FAULT, vaddr);
        return -1;
    }

    word_t paddr = vaddr;
    if (unlikely(this->privMode != PrivMode::MACHINE)) {
        VMResult vmresult;
        
        paddr = this->vaddr_translate_core(vaddr, MemType::AMO, vmresult);
        
        switch (vmresult) {
            case VMResult::VM_OK: break;
            case VMResult::VM_ACCESS_FAULT: this->trap(TrapCode::AMO_ACCESS_FAULT); return -1;
            case VMResult::VM_PAGE_FAULT:   this->trap(TrapCode::AMO_PAGE_FAULT)  ; return -1;
            case VMResult::VM_UNSET: PANIC("vmresult is not set.");
        }

        bool pmp = this->pmp_check_r(paddr, len) && this->pmp_check_w(paddr, len);
        if (unlikely(!pmp)) {
            this->trap(TrapCode::AMO_ACCESS_FAULT);
            valid = false;
            return -1;
        }
    }
    
    valid = true;
    return paddr;
}

template<typename unit_t>
void RVCore::do_load_reserved(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1;

    bool valid;
    word_t addr = SRC1;
    addr = this->amo_vaddr_translate_and_set_trap(addr, sizeof(unit_t), valid);
    if (!valid) return;

    unit_t value = this->bus->read(addr, sizeof(unit_t), valid);
    if (!valid) {
        this->trap(TrapCode::AMO_ACCESS_FAULT, addr);
        return;
    }

    DEST = value;
    this->reservedMemory[addr] = value;
    // INFO("Load reserved at address=" FMT_WORD ", pc=" FMT_WORD ", value=" FMT_WORD, addr, this->pc, (word_t)value);
}

template<typename unit_t>
void RVCore::do_store_conditional(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;

    bool valid;
    word_t addr = SRC1;
    addr = this->amo_vaddr_translate_and_set_trap(addr, sizeof(unit_t), valid);
    if (!valid) return;

    auto iter = this->reservedMemory.find(addr);
    if (iter == this->reservedMemory.end()) {
        DEST = 0;
        return;
    }

    unit_t *ptr = (unit_t *)this->bus->get_ptr(addr);
    if (ptr == nullptr) {
        this->trap(TrapCode::AMO_ACCESS_FAULT, addr);
        return;
    }

    unit_t expected = iter->second;
    unit_t desired  = SRC2;
    bool success = __atomic_compare_exchange_n(ptr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    DEST = success ? 0 : 1;
    this->reservedMemory.erase(iter);

    // if (!success) {
    //     // Store conditional failed
    //     INFO("Store conditional failed at address=" FMT_WORD ", pc=" FMT_WORD ", expected=" FMT_WORD ", v=" FMT_WORD, addr, this->pc, iter->second, *(word_t *)ptr);
    //     return;
    // }
}

template<kxemu::device::AMO amo, typename sw_t>
void RVCore::do_amo_inst(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2
    
    constexpr int len = sizeof(sw_t);

    bool valid;
    word_t paddr = this->amo_vaddr_translate_and_set_trap(SRC1, len, valid);
    if (!valid) return;

    // word_t src = SRC2;
    // sw_t oldValue = this->bus->do_atomic(addr, src, len, amo, valid); // signed extend
    
    // if (!valid) {
    //     this->trap(TrapCode::AMO_ACCESS_FAULT, addr);
    //     return;
    // }

    // DEST = (sword_t)oldValue;
    this->bus->do_atomic(paddr, SRC2, len, amo).and_then([&](word_t oldValue) -> std::optional<word_t> {
        if (paddr == 0x81858780) {
            INFO("AMO at 0x81858780, amo=%d, oldValue=" FMT_WORD ", data=" FMT_WORD ", pc=" FMT_WORD, (int)amo, oldValue, SRC2, this->pc);
        }
        DEST = (sw_t)oldValue;
        return oldValue;
    }).or_else([&]() -> std::optional<word_t> {
        this->trap(TrapCode::AMO_ACCESS_FAULT, paddr);
        return std::nullopt;
    });
}

void RVCore::do_lr_w(const DecodeInfo &decodeInfo) {
   this->do_load_reserved<uint32_t>(decodeInfo);
}

void RVCore::do_sc_w(const DecodeInfo &decodeInfo) {
    this->do_store_conditional<uint32_t>(decodeInfo);
}

void RVCore::do_amoswap_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::SWAP, int32_t>(decodeInfo);
}

void RVCore::do_amoadd_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::ADD, int32_t>(decodeInfo);
}

void RVCore::do_amoxor_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::XOR, int32_t>(decodeInfo);
}

void RVCore::do_amoand_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::AND, int32_t>(decodeInfo);
}

void RVCore::do_amoor_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::OR, int32_t>(decodeInfo);
}

void RVCore::do_amomin_w(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::MIN, int32_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amomax_w(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::MAX, int32_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amominu_w(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::MINU, int32_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amomaxu_w(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::MAXU, int32_t>(decodeInfo);
    this->do_invalid_inst();
}

#ifdef KXEMU_ISA64

void RVCore::do_lr_d(const DecodeInfo &decodeInfo) {
    this->do_load_reserved<uint64_t>(decodeInfo);
}

void RVCore::do_sc_d(const DecodeInfo &decodeInfo) {
    this->do_store_conditional<uint64_t>(decodeInfo);
}

void RVCore::do_amoswap_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::SWAP, int64_t>(decodeInfo);
}

void RVCore::do_amoadd_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::ADD, int64_t>(decodeInfo);
}

void RVCore::do_amoxor_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::XOR, int64_t>(decodeInfo);
}

void RVCore::do_amoand_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::AND, int64_t>(decodeInfo);
}

void RVCore::do_amoor_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<AMO::OR, int64_t>(decodeInfo);
}

void RVCore::do_amomin_d(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::AMO_MIN, int64_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amomax_d(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::AMO_MAX, int64_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amominu_d(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::AMO_MINU, int64_t>(decodeInfo);
    this->do_invalid_inst();
}

void RVCore::do_amomaxu_d(const DecodeInfo &decodeInfo) {
    // this->do_amo_inst<AMO::AMO_MAXU, int64_t>(decodeInfo);
    this->do_invalid_inst();
}

#endif
