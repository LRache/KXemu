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
    INFO("Continuing execution on core %u", currentCore);
    kdb::run_cpu();
    INFO("Continuing execution on core %u", currentCore);
    return check_action();
}

static gdb_action_t stepi(void *) {
    INFO("Stepping core %u", currentCore);
    kdb::step_core(currentCore);
    INFO("Core %u stepped", currentCore);
    return check_action();
}

static int read_reg(void *args, int regno, size_t *value) {
    if ((unsigned int)regno > kxemu::isa::get_gpr_count()) {
        return EFAULT;
    }

    if ((unsigned int)regno == kxemu::isa::get_gpr_count()) {
        *value = kdb::cpu->get_core(currentCore)->get_pc();
    } else {
        *value = kdb::cpu->get_core(currentCore)->get_gpr(regno);
    }
    
    return 0;
}

static int write_reg(void *args, int regno, size_t value) {
    if ((unsigned int)regno > kxemu::isa::get_gpr_count()) {
        return EFAULT;
    }
    if ((unsigned int)regno == kxemu::isa::get_gpr_count()) {
        kdb::cpu->get_core(currentCore)->set_pc(value);
    } else {
        kdb::cpu->get_core(currentCore)->set_gpr(regno, value);
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
    
    #ifdef KXEMU_ISA64
    constexpr int xlen = 8;
    #else
    constexpr int xlen = 4;
    #endif

    static target_ops ops = {
        cont,
        stepi,
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
            xlen
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
