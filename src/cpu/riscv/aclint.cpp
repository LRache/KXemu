#include "cpu/riscv/aclint.h"
#include "cpu/word.h"
#include "log.h"
#include "debug.h"
#include "isa/word.h"

#define MSWI_BASE 0x0000
#define MSWI_SIZE 0x4000
#define MTIMECMP_BASE 0x4000
#define MTIMECMP_SIZE 0x4000
#define MTIME_BASE 0xbff8
#define MTIME_SIZE 0x8
#define SSWI_BASE 0xc000
#define SSWI_SIZE 0x4000

#define IN_RANGE(addr, name) (addr >= name##_BASE && addr < name##_BASE + name##_SIZE)

using namespace kxemu::cpu;

AClint::AClint() {
    this->coreCount = 0;
    this->cores = nullptr;
}

AClint::~AClint() {
    delete[] this->cores;
}

void AClint::init(unsigned int coreCount) {
    SELF_PROTECT(this->cores == nullptr, "CLINT is already initialized.");
    this->coreCount = coreCount;
    this->cores = new CoreInfo[coreCount];
}

void AClint::reset() {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        *this->cores[i].mtimecmp = -1;
    }
}

word_t AClint::read(word_t addr, int size, bool &success) {
    if (IN_RANGE(addr, MSWI)) {
        // MSWI only supports 32-bit reads
        if (size != 4) {
            WARN("Unaligned access to MSWI.");
            success = false;
            return 0;
        }

        unsigned int coreID = (addr - MSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            success = false;
            return 0;
        }
        success = true;
        return *this->cores[coreID].msip;
    } else if (IN_RANGE(addr, SSWI)) {
        // SSWI only supports 32-bit reads
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Unaligned access to SSWI.");
            success = false;
            return 0;
        }

        unsigned int coreID = (addr - SSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            success = false;
            return 0;
        }
        success = true;
        return *this->cores[coreID].ssip;
    } else if (IN_RANGE(addr, MTIMECMP)) {
        // MTIMECMP only supports aligned reads
        unsigned int coreID = (addr - MTIMECMP_BASE) >> 3;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            success = false;
            return 0;
        }

    #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIMECMP.");
            success = false;
            return 0;
        }
        success = true;
        return *this->cores[coreID].mtimecmp;
    #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %d for MTIMECMP", size);
            success = false;
            return 0;
        }
        word_t offset = addr & 0b11;
        success = true;
        if (offset == 0) {
            return *this->cores[coreID].mtimecmp & 0xffffffffUL;
        } else {
            return *this->cores[coreID].mtimecmp >> 32;
        }
    #endif
    } else if (IN_RANGE(addr, MTIME)) {
        PANIC("Do not access MTIME in ACLINT.");
    } else {
        WARN("Invalid address " FMT_WORD " for CLINT", addr);
        success = false;
        return 0;
    }
}

bool AClint::write(word_t addr, word_t value, int size) {
    if (IN_RANGE(addr, MSWI)) {
        // MSWI only supports 32-bit writes
        if (size != 4) {
            WARN("Unaligned access to MSWI.");
            return false;
        }

        // MSWI only supports to write 1
        if (value != 0 && value != 1) {
            WARN("Invalid value " FMT_VARU " for MSWI", value);
            return false;
        }

        unsigned int coreID = (addr - MSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            return false;
        }
        *this->cores[coreID].msip = value;
        if (this->cores[coreID].set_msip != nullptr) {
            (this->cores[coreID].set_msip)(this->cores[coreID].core);
        }
    } else if (IN_RANGE(addr, SSWI)) {
        // SSWI only supports 32-bit writes
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Unaligned access to SSWI.");
            return false;
        }

        // SSWI only supports to write 1
        if (value != 0 && value != 1) {
            WARN("Invalid value " FMT_VARU " for SSWI", value);
            return false;
        }

        unsigned int coreID = (addr - SSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            return false;
        }
        *this->cores[coreID].ssip = value;
        if (this->cores[coreID].set_ssip != nullptr) {
            (this->cores[coreID].set_ssip)(this->cores[coreID].core);
        }
    } else if (IN_RANGE(addr, MTIMECMP)) {
        // MTIMECMP only supports aligned writes
        unsigned int coreID = (addr - MTIMECMP_BASE) >> 3;
        
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
        *this->cores[coreID].mtimecmp = value;
        this->cores[coreID].set_mtimecmp(this->cores[coreID].core);
    #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %d for MTIMECMP", size);
            return false;
        }
        word_t offset = addr & 0b100;
        if (offset == 0) {
            *this->cores[coreID].mtimecmp &= ~0xffffffffUL;
            *this->cores[coreID].mtimecmp |= value;
        } else {
            *this->cores[coreID].mtimecmp &= 0xffffffffUL;
            *this->cores[coreID].mtimecmp |= (uint64_t)value << 32;
            this->cores[coreID].set_mtimecmp(this->cores[coreID].core);
        }
        #endif
    } else if (IN_RANGE(addr, MTIME)) {
        PANIC("Do not access MTIME in ACLINT.");
    } else {
        WARN("Invalid address " FMT_WORD " for CLINT", addr);
        return false;
    }
    return false;
}

void AClint::register_core(unsigned int coreId, const struct CoreInfo &info) {
    SELF_PROTECT(this->cores != nullptr, "CLINT is not initialized.");
    SELF_PROTECT(coreId < this->coreCount, "Core ID out of range %d", coreId);

    this->cores[coreId] = info;
}
