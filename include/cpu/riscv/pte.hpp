#ifndef __KXEMU_CPU_RISCV_PTE_HPP__
#define __KXEMU_CPU_RISCV_PTE_HPP__

#include "cpu/word.hpp"

namespace kxemu::cpu {
    
class PTEFlag {
public:
    uint8_t flag;
        
    PTEFlag() : flag(0) {}
    PTEFlag(uint8_t flag) : flag(flag) {}
            
    bool v() const { return this->flag & (1 << 0); }
    bool r() const { return this->flag & (1 << 1); }
    bool w() const { return this->flag & (1 << 2); }
    bool x() const { return this->flag & (1 << 3); }
    bool u() const { return this->flag & (1 << 4); }
    bool g() const { return this->flag & (1 << 5); }
    bool a() const { return this->flag & (1 << 6); }
    bool d() const { return this->flag & (1 << 7); }

    void set_a() { this->flag |= (1 << 6); }
    void set_d() { this->flag |= (1 << 7); }
        
    operator uint8_t() const {
        return this->flag;
    }
        
    PTEFlag operator&(int mask) const {
        return this->flag & mask;
    }
};
        
class PTE {
private:
    word_t pte;
public:
    PTE() : pte(0) {}
    PTE(word_t pte) : pte(pte) {}
            
    PTEFlag flag() const { return this->pte & 0xff; }
    word_t ppn() const { return this->pte >> 10; }

    operator word_t() const {
        return this->pte;
    }
};

}

#endif
