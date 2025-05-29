#include "isa/isa.h"
#include "kdb/kdb.h"
#include "log.h"

#include <cstddef>
#include <cstdio>
#include <cstring>

extern "C"  {
    #include "gdbstub.h"
}

using namespace kxemu;

static unsigned int currentCore = 0;

static gdb_action_t check_action() {
    gdb_action_t act = ACT_RESUME;
    for (unsigned int i = 0; i < kdb::cpu->core_count(); i++) {
        if (kdb::cpu->get_core(i)->is_halt() || kdb::cpu->get_core(i)->is_error()) {
            act = ACT_SHUTDOWN;
            break;
        }
    }
    return act;
}

static gdb_action_t cont(void *) {
    kdb::run_cpu();
    return check_action();
}

static gdb_action_t stepi(void *) {
    kdb::step_core(currentCore);
    return check_action();
}

static int read_reg(void *args, int regno, void *value) {
    if ((unsigned int)regno > kxemu::isa::get_gpr_count()) {
        return EFAULT;
    }

    if ((unsigned int)regno == kxemu::isa::get_gpr_count()) {
        *(size_t *)value = kdb::cpu->get_core(currentCore)->get_pc();
    } else {
        *(size_t *)value = kdb::cpu->get_core(currentCore)->get_gpr(regno);
    }
    
    return 0;
}

static int write_reg(void *args, int regno, void *value) {
    if ((unsigned int)regno > kxemu::isa::get_gpr_count()) {
        return EFAULT;
    }
    if ((unsigned int)regno == kxemu::isa::get_gpr_count()) {
        kdb::cpu->get_core(currentCore)->set_pc(*(size_t *)value);
    } else {
        kdb::cpu->get_core(currentCore)->set_gpr(regno, *(size_t *)value);
    }
    return 0;
}

static int read_mem(void *, size_t addr, size_t len, void *val) {
    bool valid;
    kdb::word_t paddr = kdb::cpu->get_core(currentCore)->vaddr_translate(addr, valid);
    if (!valid) {
        paddr = addr;
    }
    bool s = kdb::bus->memcpy(paddr, len, val);
    if (s) {
        return 0;
    } else {
        return EFAULT;
    }
}

static int write_mem(void *, size_t addr, size_t len, void *val) {
    bool valid;
    kdb::word_t paddr = kdb::cpu->get_core(currentCore)->vaddr_translate(addr, valid);
    if (!valid) {
        paddr = addr;
    }
    bool s = kdb::bus->load_from_memory(val, paddr, len);
    if (s) {
        return 0;
    } else {
        return EFAULT;
    }
}

static bool set_bp(void *, size_t addr, bp_type_t) {
    kdb::add_breakpoint(addr);
    return true;
}

static bool del_bp(void *, size_t addr, bp_type_t) {
    return kdb::remove_breakpoint(addr);
}

static void on_interrupt(void *) {
    INFO("Interrupted by GDB");
}

static void set_cpu(void *, int cpuid) {
    currentCore = cpuid;
}

static int get_cpu(void *) {
    return currentCore;
}

static size_t get_reg_bytes(int regno) {
    if ((unsigned int)regno > kxemu::isa::get_gpr_count()) {
        return -1; // Invalid register number
    }
    
    #ifdef KXEMU_ISA64
    return 8; // 64-bit registers
    #else
    return 4; // 32-bit registers
    #endif
}

static gdbstub_t gdbstub;

static bool gdb_init(const std::string &addr) {
    char s[32];
    std::strcpy(s, addr.c_str());
    char targetDesc[96];
    snprintf(
        targetDesc, sizeof(targetDesc), 
        "<target version=\"1.0\"><architecture>%s</architecture></target>", 
        kxemu::isa::get_gdb_target_desc()
    );

    static target_ops ops = {
        cont,
        stepi,
        get_reg_bytes,
        read_reg,
        write_reg,
        read_mem,
        write_mem,
        set_bp,
        del_bp,
        on_interrupt,
        set_cpu,
        get_cpu,
    };
    
    bool v = gdbstub_init(
        &gdbstub, 
        &ops, 
        {
            targetDesc, 
            (int)kdb::cpu->core_count(),
            (int)kxemu::isa::get_gpr_count() + 1, 
        }, 
        s
    );
    if (!v) {
        return false;
    }

    return true;
}

bool kdb::run_gdb(const std::string &addr) {
    if (!gdb_init(addr)) {
        return false;
    }
    gdbstub_run(&gdbstub, nullptr);
    gdbstub_close(&gdbstub);
    return true;
}
