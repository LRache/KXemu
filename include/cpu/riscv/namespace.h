#ifndef __KXEMU_CPU_RISCV_NAMESPACE_H__
#define __KXEMU_CPU_RISCV_NAMESPACE_H__

namespace kxemu::cpu {

    enum PrivMode {
        MACHINE = 3,
        SUPERVISOR = 1,
        USER = 0,   
    };

    class RVCore;

}

#endif
