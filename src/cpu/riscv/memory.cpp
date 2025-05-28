#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "word.h"
#include "log.h"
#include "macro.h"
#include "debug.h"

#include <optional>
#include <expected>

#define PGALIGN(addr) ((addr) & ~(PGSIZE - 1))
#define ADDR_PAGE_UNALIGNED(addr, len) unlikely(PGALIGN(addr) != (PGALIGN(addr + len - 1)))

using namespace kxemu::cpu;

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

word_t RVCore::vaddr_translate_bare(word_t addr, MemType type, VMResult &result) {
    result = VM_OK;
    return addr;
}

template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
word_t RVCore::vaddr_translate_sv(addr_t vaddr, MemType type, VMResult &result) {
    // Whether the page which has the U bit set in the PTE is accessible by the current privilege mode
    bool uPageAccessible = this->privMode == PrivMode::USER || this->mstatus.sum;

    word_t base = this->pageTableBase;
    for (int i = LEVELS - 1; i >= 0; i--) {
        word_t pteAddr = base + vaddr.vpn(i, VPNBITS) * PTESIZE;

        word_t pteData;
        if (unlikely(!this->pm_read_check(pteAddr, pteData, PTESIZE))) {
            result = VM_ACCESS_FAULT;
            return -1;
        }

        PTE pte = pteData;
        PTEFlag pteFlag = pte.flag();
        bool v = pteFlag.v();
        if (!v) {
            result = VM_PAGE_FAULT;
            return -1;
        }
        
        bool r = pteFlag.r();
        bool w = pteFlag.w();
        bool x = pteFlag.x();
        bool u = pteFlag.u();
        
        // If any bits or encodings that are reserved for future standard use 
        // are set within pte, stop and raise a page-fault exception.
        if (!r && w) {
            INFO("Reserved bits are set in PTE");
            result = VM_PAGE_FAULT;
            return -1;
        }

        if (r || x) {
            // This is a leaf PTE
            // A leaf PTE has been reached

            // If i>0 and pte.ppn[i-1:0] ≠ 0, this is a misaligned superpage; 
            // stop and raise a page-fault exception
            if (i > 0) {
                word_t ppn_lower = pte.ppn() & ((1 << (VPNBITS * i)) - 1);
                if (ppn_lower != 0) {
                    result = VM_PAGE_FAULT;
                    return -1;
                }
            }

            // Determine if the requested memory access is allowed by the pte.u bit     
            if (u) {
                if (!uPageAccessible) {
                    result = VM_PAGE_FAULT;
                    return -1;
                } 
            } else {
                if (this->privMode == PrivMode::USER) {
                    result = VM_PAGE_FAULT;
                    return -1;
                }
            }

            // Determine if the requested memory access is allowed by the pte.r, pte.w, and pte.x bits
            if ((pteFlag & 0xf & type) != type) {
                result = VM_PAGE_FAULT;
                return -1;
            }

            // pa.pgoff = va.pgoff
            word_t mask = PGSIZE - 1;
            
            // If i>0, then pa.ppn[i-1:0] = va.vpn[i-1:0]
            mask |= ((1 << (VPNBITS * i)) - 1) << PGBITS; // Mask for superpage ppn

            word_t paddr = (((word_t)pte << 2) & ~mask) | (vaddr & mask);
            result = VM_OK;

            this->tlb_push(vaddr, paddr, pteAddr, pte.flag());

            return paddr;
        } else {
            // This is a pointer to the next level of the page table
            word_t ppn = pte.ppn();
            base = ppn * PGSIZE;
        }
    }
    result = VM_PAGE_FAULT;
    return -1;
}

#ifdef KXEMU_ISA32
word_t RVCore::vaddr_translate_sv32(word_t vaddr, MemType type, VMResult &result) {
    constexpr unsigned int LEVELS  = 2;
    constexpr unsigned int PTESIZE = 4;
    constexpr unsigned int VPNBITS = 10;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type, result);
}
#else
word_t RVCore::vaddr_translate_sv39(word_t vaddr, MemType type, VMResult &result) {
    // Instruction fetch addresses and load and store effective addresses,
    // which are 64 bits, must have bits 63–39 all equal to bit 38, 
    // or else a page-fault exception will occur.
    if ((vaddr >> 39) != ((vaddr & (1UL << 38)) ? 0x1ffffff : 0)) {
        result = VM_PAGE_FAULT;
        return -1;
    }
    
    constexpr unsigned int LEVELS  = 3;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type, result);
}

word_t RVCore::vaddr_translate_sv48(word_t vaddr, MemType type, VMResult &result) {
    constexpr unsigned int LEVELS  = 4;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type, result);
}

word_t RVCore::vaddr_translate_sv57(word_t vaddr, MemType type, VMResult &result) {
    constexpr unsigned int LEVELS  = 5;
    constexpr unsigned int PTESIZE = 8;
    constexpr unsigned int VPNBITS = 9;

    return this->vaddr_translate_sv<LEVELS, PTESIZE, VPNBITS>(vaddr, type, result);
}

#endif

word_t RVCore::vaddr_translate_core(addr_t vaddr, MemType type, VMResult &result) {
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
            result = VM_OK;
            return block->paddr + vaddr.tlb_off();
        } else {
            result = VM_PAGE_FAULT;
            return -1;
        }
    } else {
        return (this->*vaddr_translate_func)(vaddr, type, result);
    }
    #else
    return (this->*vaddr_translate_func)(vaddr, type, result);
    #endif
}

word_t RVCore::vaddr_translate(word_t vaddr, bool &valid) {
    VMResult result;
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::DontCare, result);
    valid = result == VM_OK;
    return paddr;
}

bool RVCore::pm_read(word_t paddr, word_t &data, unsigned int len) {
    bool valid;
    data = this->bus->read(paddr, len, valid);
    return valid;
}

bool RVCore::pm_write(word_t paddr, word_t data, unsigned int len) {
    bool valid = this->bus->write(paddr, data, len);
    return valid;
}

bool RVCore::pm_read_check(word_t paddr, word_t &data, unsigned int len) {
    if (unlikely(!this->pmp_check_r(paddr, len))) {
        return false;
    }
    return this->pm_read(paddr, data, len);
}

bool RVCore::pm_write_check(word_t paddr, word_t data, unsigned int len) {
    if (unlikely(!this->pmp_check_w(paddr, len))) {
        return false;
    }
    return this->pm_write(paddr, data, len);
}

bool RVCore::vm_fetch() {
    word_t vaddr = this->pc;
    
    VMResult vmresult = VM_UNSET;
    
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::FETCH, vmresult);
    switch (vmresult) {
        case VM_OK: break;
        case VM_ACCESS_FAULT: this->trap(TrapCode::INST_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TrapCode::INST_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set");
    }

    word_t inst;
    if (ADDR_PAGE_UNALIGNED(vaddr, 4)) {
        if (unlikely(!this->csr.pmp_check_x(paddr, 2))) {
            HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TrapCode::INST_ACCESS_FAULT, paddr);
            return false;
        }

        word_t low;
        if (!this->pm_read(paddr, low, 2)) {
            this->trap(TrapCode::INST_ACCESS_FAULT);
            return false;
        }

        paddr = this->vaddr_translate_core(vaddr + 2, MemType::FETCH, vmresult);
        switch (vmresult) {
            case VM_OK: break;
            case VM_ACCESS_FAULT: this->trap(TrapCode::INST_ACCESS_FAULT, vaddr); return false;
            case VM_PAGE_FAULT:   this->trap(TrapCode::INST_PAGE_FAULT  , vaddr); return false;
            case VM_UNSET: PANIC("vmresult is not set.");
        }

        word_t high;
        if (!this->pm_read(paddr, high, 2)) {
            this->trap(TrapCode::INST_ACCESS_FAULT);
            return false;
        }
        
        inst = (high << 16) | low;

    } else {
        if (unlikely(!this->pm_read_check(paddr, inst, 4))) {
            DEBUG("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TrapCode::INST_ACCESS_FAULT, paddr);
            return false;
        }
    }
   
    this->inst = inst;
    
    return true;
}

bool RVCore::vm_read(word_t vaddr, word_t &data, unsigned int len) {
    VMResult result = VM_UNSET;
    
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::LOAD, result);
        
    switch (result) {
        case VM_OK: break;
        case VM_ACCESS_FAULT: this->trap(TrapCode::LOAD_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TrapCode::LOAD_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set.")
    }

    if (ADDR_PAGE_UNALIGNED(vaddr, len)) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_read misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    }
    
    if (unlikely(!this->pmp_check_r(paddr, len))) {
        WARN("Physical memory protection check failed when load, addr=" FMT_WORD ", len=%d", paddr, len);
        this->trap(TrapCode::LOAD_ACCESS_FAULT);
        return false;
    }
    
    return this->pm_read(paddr, data, len);
}

bool RVCore::vm_write(word_t vaddr, word_t data, unsigned int len) {
    VMResult vmresult;
    
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::STORE, vmresult);
    switch (vmresult) {
        case VM_OK: break;
        case VM_ACCESS_FAULT: this->trap(TrapCode::STORE_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TrapCode::STORE_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set.");
    }
    
    if (ADDR_PAGE_UNALIGNED(vaddr, len)) {
        // vaddr is unaligned, need to concat two memory access
        WARN("vm_write misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
        NOT_IMPLEMENTED();
    } 

    if (unlikely(!this->csr.pmp_check_w(vaddr, len))) {
        WARN("Physical memory protection check failed when store, addr=" FMT_WORD ", len=%d", paddr, len);
        this->trap(TrapCode::STORE_ACCESS_FAULT);
        return false;
    }

    return this->pm_write(paddr, data, len);
}

bool RVCore::memory_fetch() {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        word_t inst;
        if(unlikely(!this->pm_read(this->pc, inst, 4))) {
            this->trap(TrapCode::INST_ACCESS_FAULT);
            return false;
        }
        this->inst = inst;
        return true;
    } else {
        return vm_fetch();
    }
}

word_t RVCore::memory_load(word_t addr, unsigned int len) {
    word_t data = -1;
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        if (unlikely(!this->pm_read(addr, data, len))) {
            this->trap(TrapCode::LOAD_ACCESS_FAULT, addr);
        }
    } else {
        vm_read(addr, data, len);
    }
    return data;
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
            default: PANIC("Invalid SATP mode"); break;
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
