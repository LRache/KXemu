#ifndef __KXEMU_CPU_RISCV_CLINT_H__
#define __KXEMU_CPU_RISCV_CLINT_H__

#include "cpu/word.h"

#include <functional>
#include <cstdint>

namespace kxemu::cpu {

// This is an implementation of the Advanced Core Local Interruptor (ACLINT) for RISC-V.
// See https://github.com/riscv/riscv-aclint/releases/download/v1.0-rc4/riscv-aclint-1.0-rc4.pdf
// offset          | Device   | Description
// 0x0000 - 0x3fff | MSWI     | Machine-mode Software Interrupt Device
// 0x4000 - 0x7ff0 | MTIMECMP |
// 0xbff8 - 0xbfff | MTIME    |
// 0xc000 - 0xffff | SSWI     | Supervisor-mode Software Interrupt Device
class AClint {
public:
    using callback_t = std::function<void(void *)>;
    struct CoreInfo {
        void *core;         // Pointer to the core object
        
        // The following fields are allocated in the core object
        uint64_t *mtimecmp; // Machine-mode Timecmp Register
        bool *msip;         // Machine-mode Software Interrupt Device
        bool *ssip;         // Supervisor-mode Software Interrupt Device

        callback_t set_mtimecmp; // Machine-mode Timecmp Register
        callback_t set_msip; // Machine-mode Software Interrupt Device
        callback_t set_ssip; // Supervisor-mode Software Interrupt Device
    };

    AClint();
    ~AClint();

    void init(unsigned int coreCount);
    void reset();

    word_t read(word_t addr, int size, bool &success);
    bool write(word_t addr, word_t value, int size);

    void register_core(unsigned int coreId, const CoreInfo &info);
private:
    unsigned int coreCount;

    CoreInfo *cores;
};

} // namespace kxemu::cpu

#endif
