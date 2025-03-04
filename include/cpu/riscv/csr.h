#ifndef __KXEMU_CPU_RISCV_CSR_H__
#define __KXEMU_CPU_RISCV_CSR_H__

#include "cpu/word.h"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace kxemu::cpu {

class RVCore;

class RVCSR {
private:
    using csr_rw_func_t = word_t (RVCSR::*)(unsigned int addr, word_t value, bool &valid);
    using callback_t = std::function<void ()>;
    struct CSR {
        csr_rw_func_t  readFunc;
        csr_rw_func_t writeFunc;
        word_t value;
        word_t resetValue;

        callback_t writeCallback;
    };
    std::unordered_map<unsigned int, CSR> csr;
    void add_csr(unsigned int addr, word_t resetValue = 0, csr_rw_func_t readFunc = nullptr, csr_rw_func_t writeFunc = nullptr);

    callback_t update_stimecmp;

    struct PMPCfg {
        word_t start;
        word_t end;
        bool r;
        bool w;
        bool x;
    } pmpCfgArray[64];
    unsigned int pmpCfgCount;
    void reload_pmpcfg();
    PMPCfg *pmp_check(word_t addr, int len);

    std::function<uint64_t()> get_uptime;

    // misa
    word_t  read_misa(unsigned int addr, word_t value, bool &valid);
    word_t write_misa(unsigned int addr, word_t value, bool &valid);

    // mip
    word_t  read_mip(unsigned int addr, word_t value, bool &valid);
    word_t write_mip(unsigned int addr, word_t value, bool &valid);

    // mie
    word_t  read_mie(unsigned int addr, word_t value, bool &valid);
    word_t write_mie(unsigned int addr, word_t value, bool &valid);

    // pmp
    word_t write_pmpcfg (unsigned int addr, word_t value, bool &valid);
    word_t write_pmpaddr(unsigned int addr, word_t value, bool &valid);

    // sip
    word_t  read_sip(unsigned int addr, word_t value, bool &valid);
    word_t write_sip(unsigned int addr, word_t value, bool &valid);

    // stimecmp
    word_t write_stimecmp(unsigned int addr, word_t value, bool &valid);

    // sie
    word_t  read_sie(unsigned int addr, word_t value, bool &valid);
    word_t write_sie(unsigned int addr, word_t value, bool &valid);

    // sstatus
    word_t  read_sstatus(unsigned int addr, word_t value, bool &valid);
    word_t write_sstatus(unsigned int addr, word_t value, bool &valid);

    // satp
    word_t  read_satp(unsigned int addr, word_t value, bool &valid);
    word_t write_satp(unsigned int addr, word_t value, bool &valid);

    // time
    word_t  read_time(unsigned int addr, word_t value, bool &valid);

public:
    RVCSR();

    int privMode;

    void init(unsigned int hartId, std::function<uint64_t()> get_uptime);
    void set_write_callbacks(unsigned int addr, callback_t callback);
    void reset();

    word_t read_csr(unsigned int addr, bool &valid);
    word_t read_csr(unsigned int addr);
    bool   write_csr(unsigned int addr, word_t value);

    word_t *get_csr_ptr(unsigned int addr);
    const word_t *get_csr_ptr_readonly(unsigned int addr) const;

    bool pmp_check_r(word_t addr, int len);
    bool pmp_check_w(word_t addr, int len);
    bool pmp_check_x(word_t addr, int len);
}; // class RVCSR

} // namespace kxemu::cpu

#endif
