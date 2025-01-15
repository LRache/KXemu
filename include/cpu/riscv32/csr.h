#ifndef __KXEMU_CPU_RISCV32_CSR_H__
#define __KXEMU_CPU_RISCV32_CSR_H__

#include "isa/word.h"
#include <unordered_map>

namespace kxemu::cpu {

class RV32CSR {
private:
    using csr_read_fun_t  = word_t (RV32CSR::*)(unsigned int addr, word_t value);
    using csr_write_fun_t = word_t (RV32CSR::*)(unsigned int addr, word_t oldValue, bool &valid);
    struct CSR {
        csr_read_fun_t  readFunc;
        csr_write_fun_t writeFunc;
        word_t value;
    };
    std::unordered_map<unsigned int, CSR> csr;

    void add_csr(word_t addr, word_t init = 0, csr_read_fun_t = nullptr, csr_write_fun_t = nullptr);

    struct PMPCfg {
        unsigned int index;
        unsigned int a;
        word_t start;
        word_t end;
        bool r;
        bool w;
        bool x;
    } pmpCfgArray[64];
    unsigned int pmpCfgCount;
    void reload_pmpcfg();
    PMPCfg *pmp_check(word_t addr, int len);

public:
    void init(unsigned int hartId);
    void reset();

    word_t get_csr(unsigned int addr, bool &success);
    void   set_csr(unsigned int addr, word_t value, bool &success);

    word_t *get_csr_ptr(unsigned int addr);
    const word_t *get_csr_ptr_readonly(unsigned int addr) const;

    // misa
    word_t  read_misa(unsigned int addr, word_t value);
    word_t write_misa(unsigned int addr, word_t value, bool &valid);

    // pmp
    word_t write_pmpcfg (unsigned int addr, word_t value, bool &valid);
    word_t write_pmpaddr(unsigned int addr, word_t value, bool &valid);
    bool pmp_check_r(word_t addr, int len);
    bool pmp_check_w(word_t addr, int len);
    bool pmp_check_x(word_t addr, int len);

    // sip
    word_t  read_sip(unsigned int addr, word_t value);
    word_t write_sip(unsigned int addr, word_t value, bool &valid);

    // sstatus
    word_t  read_sstatus(unsigned int addr, word_t value);
    word_t write_sstatus(unsigned int addr, word_t value, bool &valid);
};

} // namespace kxemu::cpu

#endif
