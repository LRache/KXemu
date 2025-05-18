#ifndef __KXEMU_CPU_RISCV_CSR_H__
#define __KXEMU_CPU_RISCV_CSR_H__

#include "cpu/riscv/def.h"
#include "cpu/word.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <cstdint>

namespace kxemu::cpu {

class RVCore;

class RVCSR {
private:
    using csr_read_func_t  = std::optional<word_t> (RVCSR::*)(unsigned int addr);
    using csr_write_func_t = bool (RVCSR::*)(unsigned int addr, word_t value);
    using callback_t = std::function<void ()>;
    struct CSR {
        csr_read_func_t readFunc;
        csr_write_func_t writeFunc;
        word_t value;
        word_t resetValue;

        callback_t writeCallback;
    };
    std::unordered_map<unsigned int, CSR> csr;
    void add_csr(CSRAddr addr, csr_read_func_t readFunc = nullptr, csr_write_func_t writeFunc = nullptr, word_t resetValue = 0);

    callback_t update_stimecmp;

    struct PMPCfg {
        uint64_t start;
        uint64_t end;
        bool r;
        bool w;
        bool x;
    } pmpCfgArray[64];
    unsigned int pmpCfgCount;
    void reload_pmpcfg();
    PMPCfg *pmp_check(word_t addr, int len);

    std::function<uint64_t()> get_uptime;

    // mip
    bool write_mip(unsigned int addr, word_t value);

    // mie
    bool write_mie(unsigned int addr, word_t value);

    // pmp
    bool write_pmpcfg (unsigned int addr, word_t value);
    bool write_pmpaddr(unsigned int addr, word_t value);

    // sip
    std::optional<word_t> read_sip(unsigned int addr);
    bool write_sip(unsigned int addr, word_t value);

    // sie
    std::optional<word_t> read_sie(unsigned int addr);
    bool write_sie(unsigned int addr, word_t value);

    // sstatus
    std::optional<word_t> read_sstatus(unsigned int addr);
    bool write_sstatus(unsigned int addr, word_t value);

    // satp
    bool write_satp(unsigned int addr, word_t value);

    // fflags
    std::optional<word_t> read_fflags(unsigned int addr);
    bool write_fflags(unsigned int addr, word_t value);
    
    // frm
    std::optional<word_t> read_frm(unsigned int addr);
    bool write_frm(unsigned int addr, word_t value);

    // time
    bool time_readable();
    std::optional<word_t> read_time (unsigned int addr);
    std::optional<word_t> read_timeh(unsigned int addr);

public:
    RVCSR();

    int privMode;

    void init(unsigned int hartId, std::function<uint64_t()> get_uptime);
    void set_write_callbacks(unsigned int addr, callback_t callback);
    void reset();

    // Read and write CSR with callback functions
    // This is used for CSR read or write instructions
    word_t read_csr(unsigned int addr, bool &valid);
    word_t read_csr(unsigned int addr);
    bool   write_csr(unsigned int addr, word_t value);

    // Get or set csr value without calling 
    // read or write callback functions.
    // This is used for core internal operations
    // such as trap, interrupt, etc.
    word_t get_csr_value(CSRAddr addr) const;
    void   set_csr_value(CSRAddr addr, word_t value);

    word_t *get_csr_ptr(unsigned int addr);
    const word_t *get_csr_ptr_readonly(unsigned int addr) const;
 
    bool pmp_check_r(word_t addr, int len);
    bool pmp_check_w(word_t addr, int len);
    bool pmp_check_x(word_t addr, int len);
}; // class RVCSR

} // namespace kxemu::cpu

#endif
