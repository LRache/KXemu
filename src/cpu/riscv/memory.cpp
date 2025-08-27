#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "log.h"
#include "debug.h"
#include "macro.h"

#include <cstdint>
#include <optional>
#include <expected>
#include <utility>

using namespace kxemu::cpu;

static inline bool access_page_unaligned(word_t addr, unsigned int len) {
    return ((addr & ~(PGSIZE-1)) != ((addr+len-1) & ~(PGSIZE-1)));
}

#ifdef CONFIG_TLB

void RVCore::tlb_push(addr_t vaddr, addr_t paddr, word_t pteAddr, uint8_t flag) {
    TLBBlock &block = this->tlb[vaddr.tlb_set()];
    if (block.valid) {
        this->pm_write(block.pteAddr, block.flag, 1);
    }
    block.paddr = paddr.tlb_tag_and_set();
    block.tag = vaddr.tlb_tag();
    block.pteAddr = pteAddr;
    block.flag = flag;
    block.valid = true;
}

std::optional<RVCore::TLBBlock *> RVCore::tlb_hit(addr_t vaddr) {
    TLBBlock *block = &this->tlb[vaddr.tlb_set()];
    bool hit = block->valid && block->tag == vaddr.tlb_tag();
    if (hit) {
        return block;
    } else {
        return std::nullopt;
    }
}

void RVCore::tlb_fence() {
    for (unsigned int i = 0; i < sizeof(this->tlb) / sizeof(this->tlb[0]); i++) {
        this->tlb[i].valid = false;
    }
}

#else

void RVCore::tlb_push(addr_t, addr_t, word_t, uint8_t) {}

std::optional<RVCore::TLBBlock *> RVCore::tlb_hit(addr_t vaddr) {
    return std::nullopt;
}

void RVCore::tlb_fence() {}

#endif

RVCore::VMResult RVCore::vaddr_translate_bare(word_t addr, MemType type) {
    return addr; // No translation, return the address directly
}

template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
RVCore::VMResult RVCore::vaddr_translate_sv(addr_t vaddr, MemType type) noexcept {
    // Whether the page which has the U bit set in the PTE is accessible by the current privilege mode
    bool uPageAccessible = this->privMode == PrivMode::USER || this->mstatus.sum;

    word_t base = this->pageTableBase;
    for (int i = LEVELS - 1; i >= 0; i--) {
        word_t pteAddr = base + vaddr.vpn(i, VPNBITS) * PTESIZE;
        PTE pte;
        
        if (!this->pm_read_check_optional(pteAddr, PTESIZE).and_then([&](word_t data) {
            pte = data;
            return std::optional<word_t>(data);
        })) {
            return std::unexpected(VMFault::ACCESS_FAULT);
        }

        PTEFlag pteFlag = pte.flag();
        if (!pteFlag.v()) {
            return std::unexpected(VMFault::PAGE_FAULT);
        }
        
        bool r = pteFlag.r();
        bool w = pteFlag.w();
        bool x = pteFlag.x();
        bool u = pteFlag.u();
        
        // If any bits or encodings that are reserved for future standard use 
        // are set within pte, stop and raise a page-fault exception.
        if (!r && w) {
            return std::unexpected(VMFault::PAGE_FAULT);
        }

        if (r || x) {
            // This is a leaf PTE
            // A leaf PTE has been reached

            // If i>0 and pte.ppn[i-1:0] ≠ 0, this is a misaligned superpage; 
            // stop and raise a page-fault exception
            if (i > 0) {
                word_t ppn_lower = pte.ppn() & ((1 << (VPNBITS * i)) - 1);
                if (ppn_lower != 0) {
                    return std::unexpected(VMFault::PAGE_FAULT);
                }
            }

            // Determine if the requested memory access is allowed by the pte.u bit     
            if (u) {
                if (!uPageAccessible) {
                    return std::unexpected(VMFault::PAGE_FAULT);
                } 
            } else {
                if (this->privMode == PrivMode::USER) {
                    return std::unexpected(VMFault::PAGE_FAULT);
                }
            }

            // Determine if the requested memory access is allowed by the pte.r, pte.w, and pte.x bits
            if ((pteFlag & 0xf & type) != type) {
                return std::unexpected(VMFault::PAGE_FAULT);
            }

            // pa.pgoff = va.pgoff
            word_t mask = PGSIZE - 1;
            
            // If i>0, then pa.ppn[i-1:0] = va.vpn[i-1:0]
            mask |= ((1 << (VPNBITS * i)) - 1) << PGBITS; // Mask for superpage ppn

            word_t paddr = (((word_t)pte << 2) & ~mask) | (vaddr & mask);

            this->tlb_push(vaddr, paddr, pteAddr, pte.flag());

            return paddr;
        } else {
            // This is a pointer to the next level of the page table
            word_t ppn = pte.ppn();
            base = ppn * PGSIZE;
        }
    }

    std::unreachable();
}

#ifdef KXEMU_ISA32
RVCore::VMResult RVCore::vaddr_translate_sv32(word_t vaddr, MemType type) {
    constexpr unsigned int LEVELS  = 2;
    constexpr unsigned int PTESIZE = 4;
    constexpr unsigned int VPNBITS = 10;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type);
}
#else
RVCore::VMResult RVCore::vaddr_translate_sv39(word_t vaddr, MemType type) {
    // Instruction fetch addresses and load and store effective addresses,
    // which are 64 bits, must have bits 63–39 all equal to bit 38, 
    // or else a page-fault exception will occur.
    if ((vaddr >> 39) != ((vaddr & (1UL << 38)) ? 0x1ffffff : 0)) {
        return std::unexpected(VMFault::PAGE_FAULT);
    }
    
    constexpr unsigned int LEVELS  = 3;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type);
}

RVCore::VMResult RVCore::vaddr_translate_sv48(word_t vaddr, MemType type) {
    constexpr unsigned int LEVELS  = 4;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type);
}

RVCore::VMResult RVCore::vaddr_translate_sv57(word_t vaddr, MemType type) {
    constexpr unsigned int LEVELS  = 5;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type);
}

#endif

RVCore::VMResult RVCore::vaddr_translate_core(addr_t vaddr, MemType type) noexcept {
    #ifdef CONFIG_TLB
    auto b = this->tlb_hit(vaddr);
    if (b.has_value()) {  
        TLBBlock *block = b.value();
        bool u = block->flag.u() ? this->privMode == PrivMode::USER || this->mstatus.sum : this->privMode == PrivMode::SUPERVISOR;
        if (likely((block->flag & type) == type && (u))) {
            block->flag.set_a();
            if (type & STORE) {
                block->flag.set_d();
            }
            return block->paddr + vaddr.tlb_off();
        } else {
            return std::unexpected(VMFault::PAGE_FAULT);
        }
    } else {
        return (this->*vaddr_translate_func)(vaddr, type);
    }
    #else
    return (this->*vaddr_translate_func)(vaddr, type);
    #endif
}

word_t RVCore::vaddr_translate(word_t vaddr, bool &valid) {
    auto t = this->vaddr_translate_core(vaddr, MemType::DontCare);
    if (t) {
        valid = true;
        return t.value();
    } else {
        valid = false;
        return -0x00caffee; // Translation fails, this is a placeholder
    }
}

std::optional<word_t> RVCore::pm_read_check_optional(word_t paddr, unsigned int len) {
    bool valid;
    word_t data = this->bus->read(paddr, len, valid);
    return valid ? std::optional<word_t>(data) : std::nullopt;
}

#ifdef CONFIG_USE_EXCEPTION

void RVCore::pm_fetch(word_t paddr) {
    bool valid;
    this->inst = this->bus->read(paddr, 4, valid);
    if (!valid) {
        throw TrapException(TrapCode::INST_ACCESS_FAULT, paddr);
    }
}

word_t RVCore::pm_read(word_t paddr, unsigned int len) {
    bool valid;
    word_t data = this->bus->read(paddr, len, valid);
    if (unlikely(!valid)) {
        WARN("pm_read failed, paddr=" FMT_WORD ", len=%d", paddr, len);
        throw TrapException(TrapCode::LOAD_ACCESS_FAULT, paddr);
    }
    return data;
}

void RVCore::pm_write(word_t paddr, word_t data, unsigned int len) {
    if (unlikely(!this->bus->write(paddr, data, len))) {
        WARN("pm_write failed, paddr=" FMT_WORD ", data=" FMT_WORD ", len=%d", paddr, data, len);
        throw TrapException(TrapCode::STORE_ACCESS_FAULT, paddr);
    }
}

word_t RVCore::pm_read_check(word_t paddr, unsigned int len) {
    if (unlikely(!this->pmp_check_r(paddr, len))) {
        throw TrapException(TrapCode::LOAD_ACCESS_FAULT, paddr);
    }
    return this->pm_read(paddr, len);
}

void RVCore::pm_write_check(word_t paddr, word_t data, unsigned int len) {
    if (unlikely(!this->pmp_check_w(paddr, len))) {
        throw TrapException(TrapCode::STORE_ACCESS_FAULT, paddr);
    }
    this->pm_write(paddr, data, len);
}

void RVCore::vm_fetch() {
    word_t vaddr = this->pc;

    word_t paddr = this->vaddr_translate_core(vaddr, MemType::FETCH).or_else([&](VMFault fault) -> std::expected<word_t, VMFault> {
        switch (fault) {
            case VMFault::ACCESS_FAULT: throw TrapException(TrapCode::INST_ACCESS_FAULT, vaddr);
            case VMFault::PAGE_FAULT:   throw TrapException(TrapCode::INST_PAGE_FAULT  , vaddr);
        }
    }).value();

    if (unlikely(access_page_unaligned(vaddr, 4))) {
        word_t low;
        low = this->pm_read_check(paddr, 2);

        if (likely((low & 0x3) != 0x3)) {
            // Compressed instruction
            this->inst = low;
            return ;
        }

        paddr = this->vaddr_translate_core(vaddr + 2, MemType::FETCH).or_else([&](VMFault fault) -> std::expected<word_t, VMFault> {
            switch (fault) {
                case VMFault::ACCESS_FAULT: throw TrapException(TrapCode::INST_ACCESS_FAULT, vaddr);
                case VMFault::PAGE_FAULT:   throw TrapException(TrapCode::INST_PAGE_FAULT  , vaddr);
            }
            return std::unexpected(fault);
        }).value();

        uint32_t high = this->pm_read_check(paddr, 2);
        this->inst = (high << 16) | low;
    } else {
        this->inst = this->pm_read_check(paddr, 4);
    }
}

word_t RVCore::vm_read(word_t vaddr, unsigned int len) {
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::LOAD)
    .or_else([&](VMFault fault) -> VMResult {
        switch (fault) {
            case VMFault::ACCESS_FAULT: throw TrapException(TrapCode::LOAD_ACCESS_FAULT, vaddr);
            case VMFault::PAGE_FAULT:   throw TrapException(TrapCode::LOAD_PAGE_FAULT  , vaddr); break;
        }
    }).value();
    
    if (unlikely(access_page_unaligned(vaddr, len))) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_read misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    }

    return this->pm_read_check(paddr, len);
}

void RVCore::vm_write(word_t vaddr, word_t data, unsigned int len) {
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::STORE)
    .or_else([&](VMFault fault) -> VMResult {
        switch (fault) {
            case VMFault::ACCESS_FAULT: throw TrapException(TrapCode::STORE_ACCESS_FAULT, vaddr);
            case VMFault::PAGE_FAULT:   throw TrapException(TrapCode::STORE_PAGE_FAULT  , vaddr); break;
        }
    }).value();

    if (unlikely(access_page_unaligned(vaddr, len))) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_write misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    }

    this->pm_write_check(paddr, data, len);
}

void RVCore::memory_fetch() {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        this->pm_fetch(this->pc);
    } else {
        this->vm_fetch();
    }
}

word_t RVCore::memory_load(word_t addr, unsigned int len) {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        return this->pm_read(addr, len);
    } else {
        return this->vm_read(addr, len);
    }
}

void RVCore::memory_store(word_t addr, word_t data, unsigned int len) {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        this->pm_write(addr, data, len);
    } else {
        this->vm_write(addr, data, len);
    }
}

#else // CONFIG_USE_EXCEPTION

std::optional<word_t> RVCore::pm_read(word_t paddr, unsigned int len) {
    bool valid;
    word_t data = this->bus->read(paddr, len, valid);
    return valid ? std::optional<word_t>(data) : std::nullopt;
}

bool RVCore::pm_write(word_t paddr, word_t data, unsigned int len) {
    bool valid = this->bus->write(paddr, data, len);
    return valid;
}

std::optional<word_t> RVCore::pm_read_check(word_t paddr, unsigned int len) {
    if (unlikely(!this->pmp_check_r(paddr, len))) {
        return std::nullopt;
    }
    return this->pm_read(paddr, len);
}

bool RVCore::pm_write_check(word_t paddr, word_t data, unsigned int len) {
    if (unlikely(!this->pmp_check_w(paddr, len))) {
        return false;
    }
    return this->pm_write(paddr, data, len);
}

bool RVCore::vm_fetch() {
    word_t vaddr = this->pc;

    word_t paddr;
    if (!this->vaddr_translate_core(vaddr, MemType::FETCH).and_then([&](word_t addr) -> VMResult {
        paddr = addr;
        return addr;
    }).or_else([&](VMFault fault) -> std::expected<word_t, VMFault> {
        switch (fault) {
            case VMFault::ACCESS_FAULT: this->trap(TrapCode::INST_ACCESS_FAULT, vaddr); break;
            case VMFault::PAGE_FAULT:   this->trap(TrapCode::INST_PAGE_FAULT  , vaddr); break;
        }
        return std::unexpected(fault);
    })) {
        return false;
    }

    if (unlikely(access_page_unaligned(vaddr, 4))) {
        if (unlikely(!this->csr.pmp_check_x(paddr, 2))) {
            HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TrapCode::INST_ACCESS_FAULT, paddr);
            return false;
        }

        word_t low;
        if (
            !this->pm_read(paddr, 2).and_then([&](word_t data) {
                low = data;
                return std::optional<word_t>(data);
            }).or_else([&]() -> std::optional<word_t> {
                this->trap(TrapCode::INST_ACCESS_FAULT);
                return std::nullopt;
            })
        ) {
            return false;
        }

        if (likely((low & 0x3) != 0x3)) {
            // Compressed instruction
            this->inst = low;
            return true;
        }

        if (
            !this->vaddr_translate_core(vaddr + 2, MemType::FETCH)
            .and_then([&](word_t addr) -> VMResult {
                paddr = addr;
                return std::expected<word_t, VMFault>(addr);
            })
            .or_else([&](VMFault fault) -> std::expected<word_t, VMFault> {
                switch (fault) {
                    case VMFault::ACCESS_FAULT: this->trap(TrapCode::INST_ACCESS_FAULT, vaddr); break;
                    case VMFault::PAGE_FAULT:   this->trap(TrapCode::INST_PAGE_FAULT  , vaddr); break;
                }
                return std::unexpected(fault);
        })) {
            return false;
        }

        return this->pm_read(paddr, 2).and_then([&](word_t high) -> std::optional<word_t> {
                this->inst = (high << 16) | low;
                return std::optional<word_t>(inst);
            }).or_else([&]() -> std::optional<word_t> {
                this->trap(TrapCode::INST_ACCESS_FAULT, paddr);
                return std::nullopt;
            }).has_value();
    } else {
        return this->pm_read_check(paddr, 4).and_then([&](word_t data) {
                this->inst = data;
                return std::optional<word_t>(data);
            }).or_else([&]() -> std::optional<word_t> {
                this->trap(TrapCode::INST_ACCESS_FAULT, paddr);
                return std::nullopt;
            }).has_value();
    }
    
    return true;
}

std::optional<word_t> RVCore::vm_read(word_t vaddr, unsigned int len) {
    word_t paddr;
    if (
        !this->vaddr_translate_core(vaddr, MemType::LOAD)
        .and_then([&](word_t addr) -> VMResult {
            paddr = addr;
            return addr;
        })
        .or_else([&](VMFault fault) -> VMResult {
            switch (fault) {
                case VMFault::ACCESS_FAULT: this->trap(TrapCode::LOAD_ACCESS_FAULT, vaddr); break;
                case VMFault::PAGE_FAULT:   this->trap(TrapCode::LOAD_PAGE_FAULT  , vaddr); break;
            }
            return std::unexpected(fault);
        })
    ) {
        return std::nullopt;
    }

    if (unlikely(access_page_unaligned(vaddr, len))) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_read misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    }

    return this->pm_read_check(paddr, len).or_else([&]() -> std::optional<word_t> {
            WARN("Physical memory protection check failed when load, addr=" FMT_WORD ", len=%d", paddr, len);
            this->trap(TrapCode::LOAD_ACCESS_FAULT, vaddr);
            return std::nullopt;
        });
}

bool RVCore::vm_write(word_t vaddr, word_t data, unsigned int len) {
    word_t paddr;
    if (
        !this->vaddr_translate_core(vaddr, MemType::STORE)
        .and_then([&](word_t addr) -> VMResult {
            paddr = addr;
            return addr;
        }).or_else([&](VMFault fault) -> VMResult {
            switch (fault) {
                case VMFault::ACCESS_FAULT: this->trap(TrapCode::STORE_ACCESS_FAULT, vaddr); break;
                case VMFault::PAGE_FAULT:   this->trap(TrapCode::STORE_PAGE_FAULT  , vaddr); break;
            }
            return std::unexpected(fault);
        })
    ) {
        return false;
    }
    
    if (unlikely(access_page_unaligned(vaddr, len))) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_write misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    } 

    if (unlikely(!this->pm_write_check(paddr, data, len))) {
        WARN("Physical memory protection check failed when store, addr=" FMT_WORD ", len=%d", paddr, len);
        this->trap(TrapCode::STORE_ACCESS_FAULT, vaddr);
        return false;
    } else {
        return true;
    }

    // return this->pm_write(paddr, data, len);
}

bool RVCore::memory_fetch() {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        return this->pm_read(this->pc, 4)
            .and_then([&](word_t inst) -> std::optional<word_t> {
                this->inst = inst;
                return inst;
            })
            .or_else([&]() -> std::optional<word_t> {
            #ifdef CONFIG_USE_EXCEPTION
                throw TrapException(TrapCode::INST_ACCESS_FAULT, this->pc);
            #else
                this->trap(TrapCode::INST_ACCESS_FAULT, this->pc);
                return std::nullopt;
            #endif
            }).has_value();
    } else {
        return vm_fetch();
    }
}

std::optional<word_t> RVCore::memory_load(word_t addr, unsigned int len) {
    word_t data = -1;
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        return this->pm_read(addr, len).or_else([&]() -> std::optional<word_t> {
            this->trap(TrapCode::LOAD_ACCESS_FAULT, addr);
            return std::nullopt;
        });
    } else {
        return vm_read(addr, len);
    }
}

void RVCore::memory_store(word_t addr, word_t data, unsigned int len) {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        if (unlikely(!this->pm_write(addr, data, len))) {
            this->trap(TrapCode::STORE_ACCESS_FAULT, addr);
        }
    } else {
        vm_write(addr, data, len);
    }
}

#endif

void RVCore::update_vm_translate() {
    csr::Satp satp = this->csr.get_csr_value(CSRAddr::SATP);
    
    this->pageTableBase = satp.ppn() * PGSIZE;

    if (this->privMode == PrivMode::MACHINE) {
        this->vaddr_translate_func = &RVCore::vaddr_translate_bare;
    } else {
        #ifdef KXEMU_ISA32
        if (satp.mode() == csr::Satp::SV32) {
            this->vaddr_translate_func = &RVCore::vaddr_translate_sv32;
        } else {
            this->vaddr_translate_func = &RVCore::vaddr_translate_bare;
        }
        #else
        switch (satp.mode()) {
            case csr::Satp::BARE: this->vaddr_translate_func = &RVCore::vaddr_translate_bare; break;
            case csr::Satp::SV39: this->vaddr_translate_func = &RVCore::vaddr_translate_sv39; break;
            case csr::Satp::SV48: this->vaddr_translate_func = &RVCore::vaddr_translate_sv48; break;
            case csr::Satp::SV57: this->vaddr_translate_func = &RVCore::vaddr_translate_sv57; break;
            default: std::unreachable();
        }
        #endif
    }
    
    this->icache_fence();
    this->tlb_fence();
}

bool RVCore::pmp_check_x(word_t paddr, unsigned int len) {
    return this->csr.pmp_check_x(paddr, len);
}

bool RVCore::pmp_check_r(word_t paddr, unsigned int len) {
    return this->csr.pmp_check_r(paddr, len);
}

bool RVCore::pmp_check_w(word_t paddr, unsigned int len) {  
    return this->csr.pmp_check_w(paddr, len);
}
