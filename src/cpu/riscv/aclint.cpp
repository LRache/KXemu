#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "utils/utils.h"
#include "log.h"
#include "debug.h"
#include "word.h"

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

void AClint::init(RVCore *cores[], unsigned int coreCount) {
    SELF_PROTECT(this->cores == nullptr, "CLINT is already initialized.");
    this->coreCount = coreCount;
    this->cores = new CoreObject[coreCount];
    for (unsigned int i = 0; i < coreCount; i++) {
        this->cores[i].core = cores[i];
        this->cores[i].mtimecmp = -1;
        this->cores[i].mtimerID = -1;
        this->cores[i].stimerID = -1;
    }
}

void AClint::reset() {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        this->cores[i].mtimecmp = -1;
        if (this->cores[i].mtimerID != (unsigned int)-1) {
            this->taskTimer.remove_task(this->cores[i].mtimerID);
            this->cores[i].mtimerID = -1;
        }
    }
    this->timerRunning = false;
}

word_t AClint::read(word_t addr, word_t size, bool &valid) {
    if (IN_RANGE(addr, MSWI)) {
        // MSWI only supports 32-bit reads
        if (size != 4) {
            WARN("Unaligned access to MSWI.");
            valid = false;
            return 0;
        }

        unsigned int coreID = (addr - MSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return 0;
        }
        
        valid = true; 
        return this->cores[coreID].msip;
    
    } else if (IN_RANGE(addr, SSWI)) {
        // SSWI only supports 32-bit reads
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Unaligned access to SSWI.");
            valid = false;
            return 0;
        }

        unsigned int coreID = (addr - SSWI_BASE) >> 2;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return 0;
        }
        
        valid = true;
        return this->cores[coreID].ssip;
    
    } else if (IN_RANGE(addr, MTIMECMP)) {
        // MTIMECMP only supports aligned reads
        unsigned int coreID = (addr - MTIMECMP_BASE) >> 3;
        
        // CoreID is out of range
        if (coreID >= this->coreCount) {
            WARN("Invalid core ID %d", coreID);
            valid = false;
            return 0;
        }

        #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIMECMP.");
            valid = false;
            return 0;
        }
        
        valid = true;
        return this->cores[coreID].mtimecmp;
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %d for MTIMECMP", size);
            valid = false;
            return 0;
        }

        word_t offset = addr & 0b11;
        valid = true;
        if (offset == 0) {
            return this->cores[coreID].mtimecmp & 0xffffffffUL;
        } else {
            return this->cores[coreID].mtimecmp >> 32;
        }
        #endif
    } else if (IN_RANGE(addr, MTIME)) {
        #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIME.");
            return false;
        }

        return UPTIME_TO_MTIME(this->get_uptime());
        
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %d for MTIME", size);
            return false;
        }
        
        this->mtime = UPTIME_TO_MTIME(this->get_uptime());
        
        word_t offset = addr & 0b11;
        if (offset == 0) {
            return this->mtime & 0xffffffffUL;
        } else {
            return this->mtime >> 32;
        }
        #endif
    } else {
        WARN("Invalid address " FMT_WORD " for CLINT", addr);
        valid = false;
        return 0;
    }
}

bool AClint::write(word_t addr, word_t value, word_t size) {
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
        
        this->cores[coreID].msip = value;
        if (value) {
            this->cores[coreID].core->set_software_interrupt_m();
        } else {
            this->cores[coreID].core->clear_software_interrupt_m();
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
        
        this->cores[coreID].ssip = value;
        if (value) {
            this->cores[coreID].core->set_software_interrupt_s();
        } else {
            this->cores[coreID].core->clear_software_interrupt_s();
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
        
        this->cores[coreID].mtimecmp = value;
        this->update_core_mtimecmp(coreID);
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
        return false;
    } else {
        WARN("Invalid address " FMT_WORD " for CLINT", addr);
        return false;
    }
    return false;
}

void AClint::start_timer() {
    if (this->timerRunning) {
        PANIC("Timer is already running.");
        return ;
    }
    this->timerRunning = true;
    this->bootTime = utils::get_current_time();
    this->taskTimer.start_timer();
}

void AClint::stop_timer() {
    if (!this->timerRunning) {
        PANIC("Timer is not running.");
        return ;
    }
    this->timerRunning = false;
    this->taskTimer.stop_timer();
}

uint64_t AClint::get_uptime() {
    if (!this->timerRunning) {
        return 0;
    }
    return utils::get_current_time() - this->bootTime;
}

void AClint::register_stimer(unsigned int coreID, uint64_t stimecmp) {
    if (coreID >= this->coreCount) {
        PANIC("Invalid core ID %d", coreID);
        return ;
    }

    const CoreObject &coreObj = this->cores[coreID];
    
    if (coreObj.stimerID != (unsigned int)-1) {
        this->taskTimer.remove_task(coreObj.stimerID);
    }

    coreObj.core->clear_timer_interrupt_s();

    uint64_t uptimecmp = MTIME_TO_UPTIME(stimecmp);
    uint64_t uptime = this->get_uptime();
    uint64_t delay = uptimecmp - uptime;
    this->cores[coreID].stimerID = this->taskTimer.add_task(delay, [this, coreID]() {
        this->cores[coreID].core->set_timer_interrupt_s();
        this->cores[coreID].stimerID = -1;
    });
}

void AClint::update_core_mtimecmp(unsigned int coreID) {
    if (coreID >= this->coreCount) {
        PANIC("Invalid core ID %d", coreID);
        return ;
    }

    const CoreObject &coreObj = this->cores[coreID];

    if (coreObj.mtimerID != (unsigned int)-1) {
        this->taskTimer.remove_task(coreObj.mtimerID);
    }

    coreObj.core->clear_timer_interrupt_m();

    uint64_t mtimecmp = coreObj.mtimecmp;
    uint64_t uptimecmp = MTIME_TO_UPTIME(mtimecmp);
    uint64_t uptime = this->get_uptime();
    uint64_t delay = uptimecmp - uptime;
    this->cores[coreID].mtimerID = this->taskTimer.add_task(delay, [this, coreID]() {
        this->cores[coreID].core->set_timer_interrupt_m();
        this->cores[coreID].mtimerID = -1;
    });
}

const char *AClint::get_type_name() const {
    return "RISC-V AClint";
}
