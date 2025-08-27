#include "cpu/riscv/core.hpp"
#include "cpu/word.hpp"
#include "device/bus.hpp"
#include "macro.h"

#include <cstdint>
#include <expected>
#include <optional>
#include <utility>

#include "./local-decoder.h"

using namespace kxemu::cpu;

word_t RVCore::amo_vaddr_translate_and_set_trap(word_t vaddr, int len) {
    if (unlikely(vaddr & (len - 1))) {
        throw TrapException(TrapCode::AMO_ACCESS_MISALIGNED, vaddr);
    }

    word_t paddr = vaddr;
    if (unlikely(this->privMode != PrivMode::MACHINE)) {
        paddr = this->vaddr_translate_core(vaddr, MemType::AMO).or_else([&](VMFault fault) -> VMResult {
                    switch (fault) {
                        case VMFault::ACCESS_FAULT: throw TrapException(TrapCode::AMO_ACCESS_FAULT, vaddr);
                        case VMFault::PAGE_FAULT:   throw TrapException(TrapCode::AMO_PAGE_FAULT  , vaddr);
                    }
                    std::unreachable();
                }).value();

        bool pmp = this->pmp_check_r(paddr, len) && this->pmp_check_w(paddr, len);
        if (unlikely(!pmp)) {
            throw TrapException(TrapCode::AMO_ACCESS_FAULT, paddr);
        }
    }

    return paddr;
}

template<typename sunit_t>
void RVCore::do_load_reserved(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1;

    word_t paddr = this->amo_vaddr_translate_and_set_trap(SRC1, sizeof(sunit_t));

    bool valid;
    word_t value = (sword_t)(sunit_t)this->bus->read(paddr, sizeof(sunit_t), valid);
    if (unlikely(!valid)) {
        throw TrapException(TrapCode::AMO_ACCESS_FAULT, paddr);
    }

    DEST = value;
    this->reservedMemory[paddr] = value;
}

template<typename sunit_t>
void RVCore::do_store_conditional(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;

    word_t paddr = this->amo_vaddr_translate_and_set_trap(SRC1, sizeof(sunit_t));

    auto iter = this->reservedMemory.find(paddr);
    if (iter == this->reservedMemory.end()) {
        DEST = 0;
        return;
    }

    // sunit_t expected = iter->second;
    // this->bus->compare_and_swap(addr, &expected, SRC2, sizeof(sunit_t)).and_then([&](bool success) -> std::optional<bool> {
    //     DEST = success ? 0 : 1;
    //     return success;
    // }).or_else([&]() -> std::optional<bool> {
    //     this->trap(TrapCode::AMO_ACCESS_FAULT, addr);
    //     return std::nullopt;
    // });

    sunit_t *ptr = (sunit_t *)this->bus->get_ptr(paddr);
    if (ptr == nullptr) {
        throw TrapException(TrapCode::AMO_ACCESS_FAULT, paddr);
    }

    sunit_t expected = iter->second;
    sunit_t desired  = SRC2;
    bool success = __atomic_compare_exchange_n(ptr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    DEST = success ? 0 : 1;
    this->reservedMemory.erase(iter);
}

template<kxemu::device::AMO amo, typename sunit_t>
void RVCore::do_amo_inst(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2
    
    constexpr int LEN = sizeof(sunit_t);

    word_t paddr = this->amo_vaddr_translate_and_set_trap(SRC1, LEN);

    this->bus->do_atomic(paddr, SRC2, LEN, amo).and_then([&](word_t oldValue) -> std::optional<word_t> {
        DEST = (sword_t)(sunit_t)oldValue;
        return oldValue;
    }).or_else([&]() -> std::optional<word_t> {
        throw TrapException(TrapCode::AMO_ACCESS_FAULT, paddr);
    });
}

void RVCore::do_lr_w(const DecodeInfo &decodeInfo) {
   this->do_load_reserved<int32_t>(decodeInfo);
}

void RVCore::do_sc_w(const DecodeInfo &decodeInfo) {
    this->do_store_conditional<int32_t>(decodeInfo);
}

void RVCore::do_amoswap_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::SWAP, int32_t>(decodeInfo);
}

void RVCore::do_amoadd_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::ADD, int32_t>(decodeInfo);
}

void RVCore::do_amoxor_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::XOR, int32_t>(decodeInfo);
}

void RVCore::do_amoand_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::AND, int32_t>(decodeInfo);
}

void RVCore::do_amoor_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::OR, int32_t>(decodeInfo);
}

void RVCore::do_amomin_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MIN, int32_t>(decodeInfo);
}

void RVCore::do_amomax_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MAX, int32_t>(decodeInfo);
}

void RVCore::do_amominu_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MINU, int32_t>(decodeInfo);
}

void RVCore::do_amomaxu_w(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MAXU, int32_t>(decodeInfo);
}

#ifdef KXEMU_ISA64

void RVCore::do_lr_d(const DecodeInfo &decodeInfo) {
    this->do_load_reserved<int64_t>(decodeInfo);
}

void RVCore::do_sc_d(const DecodeInfo &decodeInfo) {
    this->do_store_conditional<int64_t>(decodeInfo);
}

void RVCore::do_amoswap_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::SWAP, int64_t>(decodeInfo);
}

void RVCore::do_amoadd_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::ADD, int64_t>(decodeInfo);
}

void RVCore::do_amoxor_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::XOR, int64_t>(decodeInfo);
}

void RVCore::do_amoand_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::AND, int64_t>(decodeInfo);
}

void RVCore::do_amoor_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::OR, int64_t>(decodeInfo);
}

void RVCore::do_amomin_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MIN, int64_t>(decodeInfo);
}

void RVCore::do_amomax_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MAX, int64_t>(decodeInfo);
}

void RVCore::do_amominu_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MINU, int64_t>(decodeInfo);
}

void RVCore::do_amomaxu_d(const DecodeInfo &decodeInfo) {
    this->do_amo_inst<device::AMO::MAXU, int64_t>(decodeInfo);
}

#endif
