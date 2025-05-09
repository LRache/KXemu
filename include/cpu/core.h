#ifndef __KXEMU_CPU_CORE_H__
#define __KXEMU_CPU_CORE_H__

#include <optional>
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

    virtual void run(const word_t *breakpoints = nullptr, unsigned int n = 0) = 0;
    
    virtual word_t get_pc() = 0;
    virtual void   set_pc(word_t pc) = 0;
    virtual word_t get_gpr(unsigned int idx) = 0;
    virtual void   set_gpr(unsigned int idx, word_t value) = 0;
    virtual std::optional<word_t> get_register(const std::string &name) = 0;
    virtual bool   set_register(const std::string &name, word_t value) = 0;

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
