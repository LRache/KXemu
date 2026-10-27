#include "cpu/riscv/core.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/def.hpp"
#include "device/def.hpp"
#include "log.h"
#include "debug.h"

#include <mutex>

using namespace kxemu::device;
using kxemu::cpu::RVCore;

static inline constexpr AddrSpace MSWI     = {0x0000, 0x4000};
static inline constexpr AddrSpace MTIMECMP = {0x4000, 0x4000};
static inline constexpr AddrSpace MTIME    = {0xbff8, 0x0008};
static inline constexpr AddrSpace SSWI     = {0xc000, 0x4000};

AClint::AClint() {
    this->coreCount = 0;
    this->coreObjects = nullptr;
    this->timer = nullptr;
}

AClint::~AClint() {
    delete[] this->coreObjects;
}

void AClint::init(std::span<RVCore *const> cores, kxemu::cpu::PlatformTimer *timer) {
    Assert(this->coreObjects == nullptr, "CLINT is already initialized.");
    this->coreCount = cores.size();
    this->timer = timer;
    this->coreObjects = new CoreObject[this->coreCount];
    for (unsigned int i = 0; i < this->coreCount; i++) {
        CoreObject &coreObj = this->coreObjects[i];
        coreObj.core = cores[i];
        coreObj.msip = false;
        coreObj.ssip = false;
        coreObj.mtimer.init(timer, [core = coreObj.core](bool asserted) {
            if (asserted) {
                core->set_timer_interrupt_m();
            } else {
                core->clear_timer_interrupt_m();
            }
        });
    }
}

void AClint::reset() {
    std::lock_guard<std::mutex> lock(this->mtx);
    for (unsigned int i = 0; i < this->coreCount; i++) {
        this->coreObjects[i].mtimer.reset();
        this->coreObjects[i].msip = false;
        this->coreObjects[i].ssip = false;
    }
}

word_t AClint::read(word_t addr, word_t size, bool &valid) {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (MSWI.in_range(addr)) {
        // MSWI only supports 32-bit reads
        if (size != 4) {
            WARN("Unaligned access to MSWI.");
            valid = false;
            return -1;
        }

        unsigned int coreID = (addr - MSWI.BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return -1;
        }
        
        valid = true; 
        return this->coreObjects[coreID].msip;
    
    } else if (SSWI.in_range(addr)) {
        // SSWI only supports 32-bit reads
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Unaligned access to SSWI.");
            valid = false;
            return -1;
        }

        unsigned int coreID = (addr - SSWI.BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return -1;
        }
        
        valid = true;
        return this->coreObjects[coreID].ssip;
    
    } else if (MTIMECMP.in_range(addr)) {
        // MTIMECMP only supports aligned reads
        unsigned int coreID = (addr - MTIMECMP.BASE) >> 3;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return -1;
        }

        #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIMECMP.");
            valid = false;
            return -1;
        }
        
        valid = true;
        return this->coreObjects[coreID].mtimer.value();
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIMECMP", size);
            valid = false;
            return -1;
        }

        const uint64_t mtimecmp = this->coreObjects[coreID].mtimer.value();
        valid = true;
        if ((addr & 0b100) == 0) {
            return mtimecmp & 0xffffffffULL;
        } else {
            return mtimecmp >> 32;
        }
        #endif
    } else if (MTIME.in_range(addr)) {
        #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIME. size=%lu, addr=" FMT_WORD64, size, addr);
            valid = false;
            return -1;
        }

        valid = true;
        return this->timer->mtime();
        
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIME", size);
            valid = false;
            return -1;
        }
        
        valid = true;
        word_t offset = addr - MTIME.BASE;
        const uint64_t mtime = this->timer->mtime();
        if (offset == 0) {
            return mtime & 0xffffffffULL;
        } else {
            return mtime >> 32;
        }
        #endif
    } else {
        WARN("Invalid address " FMT_WORD64 " for CLINT", addr);
        valid = false;
        return -1;
    }
}

bool AClint::write(word_t addr, word_t value, word_t size) {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (MSWI.in_range(addr)) {
        // MSWI only supports 32-bit writes
        if (size != 4) {
            WARN("Unaligned access to MSWI.");
            return false;
        }

        // MSWI only supports to write 1
        if (value != 0 && value != 1) {
            WARN("Invalid value " FMT_VARU64 " for MSWI", value);
            return false;
        }

        unsigned int coreID = (addr - MSWI.BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            return false;
        }
        
        value = value & 1;
        this->coreObjects[coreID].msip = value;
        if (value) {
            this->coreObjects[coreID].core->set_software_interrupt_m();
        } else {
            this->coreObjects[coreID].core->clear_software_interrupt_m();
        }

    } else if (SSWI.in_range(addr)) {
        // SSWI only supports 32-bit writes
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Unaligned access to SSWI.");
            return false;
        }

        // SSWI only supports to write 1
        if (value != 0 && value != 1) {
            WARN("Invalid value " FMT_VARU64 " for SSWI", value);
            return false;
        }

        unsigned int coreID = (addr - SSWI.BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            return false;
        }
        
        this->coreObjects[coreID].ssip = value;
        if (value) {
            this->coreObjects[coreID].core->set_software_interrupt_s();
        } else {
            this->coreObjects[coreID].core->clear_software_interrupt_s();
        }
    
    } else if (MTIMECMP.in_range(addr)) {
        // MTIMECMP only supports aligned writes
        unsigned int coreID = (addr - MTIMECMP.BASE) >> 3;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            return false;
        }

    #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIMECMP.");
            return false;
        }
        
        this->coreObjects[coreID].mtimer.set(value);
    #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIMECMP", size);
            return false;
        }
        // Each half write takes effect, as the comparator is level-triggered.
        uint64_t mtimecmp = this->coreObjects[coreID].mtimer.value();
        if ((addr & 0b100) == 0) {
            mtimecmp = (mtimecmp & ~0xffffffffULL) | uint32_t(value);
        } else {
            mtimecmp = (mtimecmp & 0xffffffffULL) | (uint64_t(value) << 32);
        }
        this->coreObjects[coreID].mtimer.set(mtimecmp);
        #endif
    } else if (MTIME.in_range(addr)) {
        return false;
    } else {
        WARN("Invalid address " FMT_WORD64 " for CLINT", addr);
        return false;
    }
    
    return true;
}

const char *AClint::get_type_name() const {
    return "AClint";
}
