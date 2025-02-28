#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/cache-def.h"
#include "cpu/word.h"
#include "debug.h"
#include "word.h"
#include "log.h"
#include "macro.h"
#include <cstdarg>
#include <cstdint>
#include <cstring>

using namespace kxemu::cpu;

word_t RVCore::vaddr_translate(word_t addr, MemType type, VMResult &result) {
    result = VM_OK;
    if (likely(this->privMode == PrivMode::MACHINE)) {
        return addr;
    }
#ifdef KXEMU_ISA32
    if (SATP_MODE(*this->satp) == SATP_MODE_SV32) {
        addr = this->vaddr_translate_sv32(addr, type, result);
    }
#else
    switch (SATP_MODE(*this->satp)) {
        case SATP_MODE_BARE: break;
        case SATP_MODE_SV39:
            addr = this->vaddr_translate_sv39(addr, type, result);
            break;
        case SATP_MODE_SV48:
            addr = this->vaddr_translate_sv48(addr, type, result);
            break;
        case SATP_MODE_SV57:
            addr = this->vaddr_translate_sv57(addr, type, result);
            break;
        default: PANIC("Invalid SATP mode"); break;
    }
#endif
    return addr;
}

word_t RVCore::vaddr_translate(word_t addr, bool &valid) {
    VMResult result;
    word_t paddr = this->vaddr_translate(addr, MemType::LOAD, result);
    if (result == VM_OK) {
        valid = true;
        return paddr;
    }
    paddr = this->vaddr_translate(addr, MemType::FETCH, result);
    valid = result == VM_OK;
    return paddr;
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
    bool uPageAccessible = this->privMode == USER || STATUS_SUM(*this->mstatus);

    word_t base = SATP_PPN(*this->satp) * PGSIZE;
    for (int i = LEVELS - 1; i >= 0; i--) {
        word_t addr = base + vpn[i] * PTESIZE;
        
        if (!check_pmp(addr, PTESIZE, MemType::LOAD)) {
            DEBUG("PMP check failed when translate vaddr=" FMT_WORD, vaddr);
            result = VM_ACCESS_FAULT;
            return -1;
        }

        bool valid;
        word_t pte = this->bus->read(addr, PTESIZE, valid);
        if (!valid) {
            DEBUG("Read PTE failed when translate vaddr=" FMT_WORD, vaddr);
            result = VM_ACCESS_FAULT;
            return -1;
        }

        bool v = PTE_V(pte);
        if (!v) {
            DEBUG("PTE is not valid: PTE=" FMT_WORD ", vpn=" FMT_WORD, pte, vpn[i]);
            DEBUG("pte.addr=" FMT_WORD, addr);
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
            DEBUG("Reserved bits are set in PTE");
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
                    DEBUG("Misaligned superpage");
                    result = VM_PAGE_FAULT;
                    return -1;
                }
            }

            // Determine if the requested memory access is allowed by the pte.u bit     
            if (u) {
                if (!uPageAccessible) {
                    DEBUG("Supervisor access user PTE");
                    result = VM_ACCESS_FAULT;
                    return -1;
                } 
            } else {
                if (this->privMode == PrivMode::USER) {
                    DEBUG("User access supervisor PTE");
                    result = VM_ACCESS_FAULT;
                    return -1;
                }
            }

            // Determine if the requested memory access is allowed by the pte.r, pte.w, and pte.x bits
            if (!(pte & 0xff & type)) {
                DEBUG("PTE is not valid for the access type");
                DEBUG("Access type=%d, vaddr=" FMT_WORD " pte=" FMT_WORD, type, vaddr, pte);
                result = VM_ACCESS_FAULT;
                return -1;
            }

            // pa.pgoff = va.pgoff
            word_t mask = PGSIZE - 1;
            
            // If i>0, then pa.ppn[i-1:0] = va.vpn[i-1:0]
            mask |= ((1 << (VPNBITS * i)) - 1) << PGBITS; // Mask for superpage ppn

            word_t paddr = ((pte << 2) & ~mask) | (vaddr & mask);
            result = VM_OK;
            return paddr;
        } else {
            // This is a pointer to the next level of the page table
            word_t ppn = pte >> 10;
            base = ppn * PGSIZE;
        }
    }
    DEBUG("PTE is not valid");
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

// word_t RVCore::vm_fetch_translate(word_t addr, bool &success) {
//     success = false;
    
//     VMResult result;
//     addr = this->vaddr_translate(addr, MemType::FETCH, result);
//     switch (result) {
//         case VM_OK: break;
//         case VM_ACCESS_FAULT: this->trap(TRAP_INST_ACCESS_FAULT); return -1;
//         case VM_PAGE_FAULT:   this->trap(TRAP_INST_PAGE_FAULT);   return -1;
//     }
    
//     if (unlikely(this->privMode != PrivMode::MACHINE)) {    
//         if (unlikely(!this->csr.pmp_check_x(addr, 4))) {
//             HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
//             this->trap(TRAP_LOAD_ACCESS_FAULT);
//             return -1;
//         }
//     }

//     success = true;
//     return addr;
// }

bool RVCore::memory_fetch() {
    word_t addr = this->pc;
    VMResult result;
    addr = this->vaddr_translate(addr, MemType::FETCH, result);
    switch (result) {
        case VM_OK: break;
        case VM_ACCESS_FAULT: this->trap(TRAP_INST_ACCESS_FAULT); return -1;
        case VM_PAGE_FAULT:   this->trap(TRAP_INST_PAGE_FAULT);   return -1;
    }
    
    if (unlikely(this->privMode != PrivMode::MACHINE)) {    
        if (unlikely(!this->csr.pmp_check_x(addr, 4))) {
            HINT("Physical memory protection check failed when fetch, pc=" FMT_WORD, this->pc);
            this->trap(TRAP_LOAD_ACCESS_FAULT);
            return -1;
        }
    }

    bool valid;    
    this->inst = this->bus->read(addr, 4, valid);
    return valid;
}

#ifdef CONFIG_DCache

RVCore::DCacheBlock *RVCore::dcache_hit(word_t addr, int len) {
    word_t set = DCACHE_SET(addr);
    if (unlikely(set != DCACHE_SET(addr + len))) {
        return nullptr;
    }
    
    word_t tag = DCACHE_TAG(addr);
    DCacheBlock *block = &this->dcache[set];
    if (block->valid) {
        if (block->tag == tag) {
            // INFO("DCache hit, addr=" FMT_WORD64 ", pc=" FMT_WORD, addr, this->pc);
            return block;
        } else {
            if (block->dirty) {
                std::memcpy(block->raw, block->data, DCACHE_BLOCK_SIZE);
                // INFO("WriteBack " FMT_WORD, block->addr);
            }
        }
    }

    if (!block->valid || block->tag != tag) {
        // INFO("DCache miss, addr=" FMT_WORD64 ", pc=" FMT_WORD, addr, this->pc);
    }

    auto mem = this->bus->match_memory(addr, len);
        
    if (mem == nullptr) {
        return nullptr;
    }
    if (mem->end - addr < DCACHE_BLOCK_SIZE) {
        return nullptr;
    }

    block->tag = tag;
    block->valid = true;
    block->dirty = false;
    block->raw = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    // block.data = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    std::memcpy(block->data, block->raw, DCACHE_BLOCK_SIZE);

    return block;
}

bool RVCore::dcache_load(word_t addr, int len, word_t &data) {
    if (unlikely(DCACHE_SET(addr) != DCACHE_SET(addr + len))) {
        return false;
    }
    
    // word_t tag = DCACHE_TAG(addr);
    // word_t set = DCACHE_SET(addr);
    // DCacheBlock &block = this->dcache[set];
    // bool hit = block.tag == tag && block.valid;
    // if (block.tag != tag) {
    //     if (block.valid && block.dirty) {
    //         std::memcpy(block.raw, block.data, sizeof(block.data));
    //         INFO("WriteBack " FMT_WORD, block.addr);
    //     } 
        
    //     auto mem = this->bus->match_memory(addr, len);
        
    //     if (mem == nullptr) {
    //         return false;
    //     }
    //     if (mem->end - addr < DCACHE_BLOCK_SIZE) {
    //         return false;
    //     }

    //     block.tag = tag;
    //     block.valid = true;
    //     block.raw = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    //     block.dirty = false;
    //     // block.data = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    //     std::memcpy(block.data, block.raw, sizeof(block.data));
    // } 

    // if (hit) {
    //     INFO("DCache hit, addr=" FMT_WORD ", pc=" FMT_WORD, addr, this->pc);
    // }
    
    DCacheBlock *block = this->dcache_hit(addr, len);
    if (block == nullptr) {
        return false;
    }

    void *ptr = block->data + DCACHE_OFF(addr);
    switch (len) {
        case 1: data = *( uint8_t *)ptr; break;
        case 2: data = *(uint16_t *)ptr; break;
        case 4: data = *(uint32_t *)ptr; break;
        case 8: data = *(uint64_t *)ptr; break;
        default: PANIC("Invalid length");
    }

    return true;
}

bool RVCore::dcache_store(word_t addr, word_t data, int len) {
    // if (unlikely(DCACHE_SET(addr) != DCACHE_SET(addr + len))) {
    //     return false;
    // }
    
    // word_t tag = DCACHE_TAG(addr);
    // word_t set = DCACHE_SET(addr);
    // DCacheBlock &block = this->dcache[set];
    // bool hit = block.tag == tag && block.valid;
    // if (block.tag != tag) {
    //     if (block.valid && block.dirty) {
    //         std::memcpy(block.raw, block.data, sizeof(block.data));
    //         INFO("WriteBack " FMT_WORD, block.addr);
    //     } 

    //     auto mem = this->bus->match_memory(addr, len);
        
    //     if (mem == nullptr) {
    //         return false;
    //     }
    //     if (mem->end - addr < DCACHE_BLOCK_SIZE) {
    //         return false;
    //     }

    //     block.tag = tag;
    //     block.valid = true;
    //     block.raw = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    //     // block.data = mem->data + (addr & ~DCACHE_OFF_MASK) - mem->start;
    //     std::memcpy(block.data, block.raw, sizeof(block.data));
    //     #ifdef CONFIG_DEBUG
    //     block.addr = addr & ~DCACHE_OFF_MASK;
    //     #endif
    // } 

    DCacheBlock *block = this->dcache_hit(addr, len);
    if (block == nullptr) {
        return false;
    }
    
    void *ptr = block->data + DCACHE_OFF(addr);
    switch (len) {
        case 1: *( uint8_t *)ptr = data; break;
        case 2: *(uint16_t *)ptr = data; break;
        case 4: *(uint32_t *)ptr = data; break;
        case 8: *(uint64_t *)ptr = data; break;
        default: PANIC("Invalid length");
    }
    block->dirty = true;

    // if (hit) {
    //     INFO("DCache hit, addr=" FMT_WORD ", pc=" FMT_WORD, addr, this->pc);
    // }

    return true;
}

#endif

word_t RVCore::memory_load(word_t addr, int len) {
    if (unlikely(this->privMode != PrivMode::MACHINE)) {
        VMResult result;
        addr = this->vaddr_translate(addr, MemType::LOAD, result);
        switch (result) {
            case VM_OK: break;
            case VM_ACCESS_FAULT: this->trap(TRAP_LOAD_ACCESS_FAULT); return -1;
            case VM_PAGE_FAULT:   this->trap(TRAP_LOAD_PAGE_FAULT);   return -1;
        }

        if (unlikely(!this->csr.pmp_check_r(addr, len))) {
            WARN("Physical memory protection check failed when load, addr=" FMT_WORD ", len=%d", addr, len);
            this->trap(TRAP_LOAD_ACCESS_FAULT);
            return -1;
        }
    }

    word_t data;
    #ifdef CONFIG_DCache
    if (this->dcache_load(addr, len, data)) {
        // #ifdef CONFIG_DEBUG
        // bool valid;
        // word_t ref = this->bus->read(addr, len, valid);
        // SELF_PROTECT(valid, "DCache difftest failed, bus.read in invalid, addr=" FMT_WORD, addr);
        // SELF_PROTECT(ref == data, "DCache difftest failed, pc=" FMT_WORD ", addr=" FMT_WORD ", ref=" FMT_WORD ", dut=" FMT_WORD, this->pc, addr, ref, data);
        // #endif
        return data;
    }
    #endif

    bool valid;
    data = this->bus->read(addr, len, valid);
    if (valid) {
        return data;
    }

    WARN("Read memory failed when load, addr=" FMT_WORD ", len=%d, pc=" FMT_WORD, addr, len, this->pc);
    return -1;
}

bool RVCore::memory_store(word_t addr, word_t data, int len) {
    if (unlikely(this->privMode != PrivMode::MACHINE)) {
        VMResult result;
        addr = this->vaddr_translate(addr, MemType::STORE, result);
        switch (result) {
            case VM_OK: break;
            case VM_ACCESS_FAULT: this->trap(TRAP_STORE_ACCESS_FAULT); return false;
            case VM_PAGE_FAULT:   this->trap(TRAP_STORE_PAGE_FAULT);   return false;
        }

        if (unlikely(!this->csr.pmp_check_w(addr, len))) {
            WARN("Physical memory protection check failed when store, addr=" FMT_WORD ", len=%d", addr, len);
            this->trap(TRAP_STORE_ACCESS_FAULT);
            return false;
        }
    }

    #ifdef CONFIG_DCache
    if (this->dcache_store(addr, data, len)) {
        return true;
    }
    #endif

    return this->bus->write(addr, data, len);
    
    // bool valid = this->bus->write(addr, data, len);
    // if (valid) {
    //     return true;
    // }

    // // ACLINT memory mapped register
    // if (unlikely(addr - ACLINT_BASE <= ACLINT_SIZE)) {
    //     return this->aclint->write(addr - ACLINT_BASE, data, len);
    // }

    // return false;
}

bool RVCore::check_pmp(word_t addr, int len, MemType type) {
    switch (type) {
        case MemType::FETCH: return this->csr.pmp_check_x(addr, len);
        case MemType::LOAD:  return this->csr.pmp_check_r(addr, len);
        case MemType::STORE: return this->csr.pmp_check_w(addr, len);
        default: PANIC("Invalid memory access type");
    }
    return false;
}
