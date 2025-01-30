#ifndef __KXEMU_CPU_CORE_H__
#define __KXEMU_CPU_CORE_H__

#include <string>

namespace kxemu::cpu {

template <typename word_t>
class Core {
public:
    virtual bool is_error() = 0;
    virtual bool is_break() = 0;
    virtual bool is_running() = 0;
    virtual bool is_halt() = 0;
    virtual ~Core() = default;

    virtual void reset(word_t entry) = 0;
    virtual void step() = 0;

    virtual void run() = 0;
    virtual void run(word_t *breakpoints, unsigned int n) {
        (void)breakpoints;
        (void)n;
        return run();
    }
    
    virtual word_t get_pc() = 0;
    virtual word_t get_gpr(int idx) = 0;
    virtual word_t get_register(const std::string &name, bool &success) = 0;

    virtual word_t get_halt_pc()   = 0;
    virtual word_t get_halt_code() = 0;

    // The interface for virtual address translation for KDB
    virtual word_t vaddr_translate(word_t vaddr, bool &valid) {
        valid = true;
        return vaddr;
    }
};

} // namespace kxemu::cpu

#endif
