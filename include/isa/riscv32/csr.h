#ifndef __ISA_RISCV32_CSR_H__
#define __ISA_RISCV32_CSR_H__

#include "isa/word.h"
#include <unordered_map>

class RV32CSR {
private:
    using csr_read_fun_t  = word_t (RV32CSR::*)(word_t addr, word_t value);
    using csr_write_fun_t = word_t (RV32CSR::*)(word_t addr, word_t oldValue);
    struct CSR {
        csr_read_fun_t  readFunc;
        csr_write_fun_t writeFunc;
        word_t value;
    };
    std::unordered_map<word_t, CSR> csr;

    void add_csr(word_t addr, word_t init = 0, csr_read_fun_t = nullptr, csr_write_fun_t = nullptr);

public:
    void init();

    word_t read_csr (word_t addr, bool &success);
    void   write_csr(word_t addr, word_t value, bool &success);
};

#endif
