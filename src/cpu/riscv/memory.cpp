#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/cache-def.h"
#include "cpu/riscv/namespace.h"
#include "cpu/word.h"
#include "word.h"
#include "log.h"
#include "macro.h"

#include <cstring>

#define PGALIGN(addr) ((addr) & ~(PGSIZE - 1))
#define ADDR_PAGE_UNALIGNED(addr, len) unlikely(PGALIGN(addr) != (PGALIGN(addr + len - 1)))

using namespace kxemu::cpu;

RVCore::TLBBlock &RVCore::tlb_push(word_t vaddr, word_t paddr, word_t pteAddr, uint8_t flag) {
    word_t set = TLB_SET(vaddr);
    word_t tag = TLB_TAG(vaddr);
    TLBBlock &block = this->tlb[set];
    if (block.valid) {
        this->pm_write(block.pteAddr, block.flag, 1);
    }
    block.paddr = paddr & ~TLB_OFF_MASK;
    block.tag = tag;
    block.pteAddr = pteAddr;
    block.flag = flag;
    block.valid = true;
    return block;
}

RVCore::TLBBlock &RVCore::tlb_hit(word_t vaddr, bool &hit) {
    word_t set = TLB_SET(vaddr);
    word_t tag = TLB_TAG(vaddr);
    TLBBlock &block = this->tlb[set];
    hit = block.valid && block.tag == tag;
    return block;
}

void RVCore::tlb_fence() {
    for (unsigned int i = 0; i < sizeof(this->tlb) / sizeof(this->tlb[0]); i++) {
        this->tlb[i].valid = false;
    }
}

word_t RVCore::vaddr_translate_bare(word_t addr, MemType type, VMResult &result) {
    result = VM_OK;
    return addr;
}

template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
word_t RVCore::vaddr_translate_sv(word_t vaddr, MemType type, VMResult &result) {
    constexpr word_t PGBITS = 12;
    constexpr word_t PGSIZE = 1 << PGBITS;

    word_t vpn[5]; // We assume the maximum level is 5
    for (unsigned int i = 0; i < LEVELS; i++) {
        vpn[i] = (vaddr >> (PGBITS + i * VPNBITS)) & ((1 << VPNBITS) - 1);
    }

    // Whether the page which has the U bit set in the PTE is accessible by the current privilege mode
    bool uPageAccessible = this->privMode == USER || this->mstatus.sum;

    word_t base = this->satpPPN * PGSIZE;
    for (int i = LEVELS - 1; i >= 0; i--) {
        word_t pteAddr = base + vpn[i] * PTESIZE;

        word_t pte;
        if (unlikely(!this->pm_read_check(pteAddr, pte, PTESIZE))) {
            result = VM_ACCESS_FAULT;
            return -1;
        }

        bool v = PTE_V(pte);
        if (!v) {
            result = VM_PAGE_FAULT;
            return -1;
        }
        
        bool r = PTE_R(pte);
        bool w = PTE_W(pte);
        bool x = PTE_X(pte);
        bool u = PTE_U(pte);
        
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
                word_t ppn_lower = (pte >> 10) & ((1 << (VPNBITS * i)) - 1);
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
            if ((pte & 0xf & type) != type) {
                result = VM_PAGE_FAULT;
                return -1;
            }

            // pa.pgoff = va.pgoff
            word_t mask = PGSIZE - 1;
            
            // If i>0, then pa.ppn[i-1:0] = va.vpn[i-1:0]
            mask |= ((1 << (VPNBITS * i)) - 1) << PGBITS; // Mask for superpage ppn

            word_t paddr = ((pte << 2) & ~mask) | (vaddr & mask);
            result = VM_OK;

            this->tlb_push(vaddr, paddr, pteAddr, pte & 0xff);

            return paddr;
        } else {
            // This is a pointer to the next level of the page table
            word_t ppn = pte >> 10;
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

word_t RVCore::vaddr_translate_core(word_t vaddr, MemType type, VMResult &result) {
    bool tlbHit;
    TLBBlock &block = this->tlb_hit(vaddr, tlbHit);
    if (likely(tlbHit)) {
        bool u = PTE_U(block.flag) ? this->privMode == USER || this->mstatus.sum : this->privMode == SUPERVISOR;
        if (likely((block.flag & type) == type && (u))) {
            block.flag |= PTE_A_MASK;
            if (type & STORE) {
                block.flag |= PTE_D_MASK;
            }
            result = VM_OK;
            return block.paddr + TLB_OFF(vaddr);
        } else {
            result = VM_PAGE_FAULT;
            return -1;
        }
    } else {
        return (this->*vaddr_translate_func)(vaddr, type, result);
    }
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
        case VM_ACCESS_FAULT: this->trap(TRAP_INST_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TRAP_INST_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set");
    }

    word_t inst;
    if (ADDR_PAGE_UNALIGNED(vaddr, 4)) {
        if (unlikely(!this->csr.pmp_check_x(paddr, 2))) {
            HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TRAP_INST_ACCESS_FAULT, paddr);
            return false;
        }

        word_t low;
        if (!this->pm_read(paddr, low, 2)) {
            this->trap(TRAP_INST_ACCESS_FAULT);
            return false;
        }

        paddr = this->vaddr_translate_core(vaddr + 2, MemType::FETCH, vmresult);
        switch (vmresult) {
            case VM_OK: break;
            case VM_ACCESS_FAULT: this->trap(TRAP_INST_ACCESS_FAULT, vaddr); return false;
            case VM_PAGE_FAULT:   this->trap(TRAP_INST_PAGE_FAULT  , vaddr); return false;
            case VM_UNSET: PANIC("vmresult is not set.");
        }

        word_t high;
        if (!this->pm_read(paddr, high, 2)) {
            this->trap(TRAP_INST_ACCESS_FAULT);
            return false;
        }
        
        inst = (high << 16) | low;

    } else {
        if (unlikely(!this->csr.pmp_check_x(paddr, 4))) {
            HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TRAP_INST_ACCESS_FAULT, paddr);
            return false;
        }
        if (!this->pm_read(paddr, inst, 4)) {
            this->trap(TRAP_INST_ACCESS_FAULT);
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
        case VM_ACCESS_FAULT: this->trap(TRAP_LOAD_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TRAP_LOAD_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set.")
    }

    if (ADDR_PAGE_UNALIGNED(vaddr, len)) {
        WARN("vm_read misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
    }
    
    if (unlikely(!this->pmp_check_r(paddr, len))) {
        WARN("Physical memory protection check failed when load, addr=" FMT_WORD ", len=%d", paddr, len);
        this->trap(TRAP_LOAD_ACCESS_FAULT);
        return false;
    }
    
    return this->pm_read(paddr, data, len);
}

bool RVCore::vm_write(word_t vaddr, word_t data, unsigned int len) {
    VMResult vmresult;
    
    word_t paddr = this->vaddr_translate_core(vaddr, MemType::STORE, vmresult);
    switch (vmresult) {
        case VM_OK: break;
        case VM_ACCESS_FAULT: this->trap(TRAP_STORE_ACCESS_FAULT, vaddr); return false;
        case VM_PAGE_FAULT:   this->trap(TRAP_STORE_PAGE_FAULT  , vaddr); return false;
        case VM_UNSET: PANIC("vmresult is not set.");
    }
    
    if (ADDR_PAGE_UNALIGNED(paddr, len)) {
        // this->trap(TRAP_STORE_ADDR_MISALIGNED);
        // return false;
        WARN("vm_write misaligned, paddr=" FMT_WORD ", len=%d, pc=" FMT_WORD, paddr, len, this->pc);
    } 

    if (unlikely(!this->csr.pmp_check_w(vaddr, len))) {
        WARN("Physical memory protection check failed when store, addr=" FMT_WORD ", len=%d", paddr, len);
        this->trap(TRAP_STORE_ACCESS_FAULT);
        return false;
    }

    return this->pm_write(paddr, data, len);
}

bool RVCore::memory_fetch() {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        word_t inst;
        if(unlikely(!this->pm_read(this->pc, inst, 4))) {
            this->trap(TRAP_INST_ACCESS_FAULT);
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
            this->trap(TRAP_LOAD_ACCESS_FAULT, addr);
        }
    } else {
        vm_read(addr, data, len);
    }
    return data;
}

void RVCore::memory_store(word_t addr, word_t data, unsigned int len) {
    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        if (unlikely(!this->pm_write(addr, data, len))) {
            this->trap(TRAP_STORE_ACCESS_FAULT);
        }
    } else {
        vm_write(addr, data, len);
    }
}

void RVCore::update_satp() {
    word_t satp = this->csr.read_csr(CSR_SATP);
    this->satpPPN = SATP_PPN(satp);
    #ifdef KXEMU_ISA32
    if (SATP_MODE(satp) == SATP_MODE_SV32) {
        this->vaddr_translate_func = &RVCore::vaddr_translate_sv32;
    } else {
        this->vaddr_translate_func = &RVCore::vaddr_translate_bare;
    }
    #else
    switch (SATP_MODE(satp)) {
        case SATP_MODE_BARE: this->vaddr_translate_func = &RVCore::vaddr_translate_bare; break;
        case SATP_MODE_SV39: this->vaddr_translate_func = &RVCore::vaddr_translate_sv39; break;
        case SATP_MODE_SV48: this->vaddr_translate_func = &RVCore::vaddr_translate_sv48; break;
        case SATP_MODE_SV57: this->vaddr_translate_func = &RVCore::vaddr_translate_sv57; break;
        default: PANIC("Invalid SATP mode"); break;
    }
    #endif

    this->icache_fence();
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
