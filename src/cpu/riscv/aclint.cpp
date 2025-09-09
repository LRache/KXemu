#include "cpu/riscv/core.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/def.hpp"
#include "device/def.hpp"
#include "utils/utils.hpp"
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
}

AClint::~AClint() {
    delete[] this->coreObjects;
}

void AClint::init(RVCore *cores, unsigned int coreCount) {
    Assert(this->coreObjects == nullptr, "CLINT is already initialized.");
    this->coreCount = coreCount;
    this->coreObjects = new CoreObject[coreCount];
    for (unsigned int i = 0; i < coreCount; i++) {
        this->coreObjects[i].core = &cores[i];
        this->coreObjects[i].mtimecmp = -1;
        this->coreObjects[i].mtimerID = -1;
        this->coreObjects[i].stimerID = -1;
        this->coreObjects[i].msip = false;
        this->coreObjects[i].ssip = false;
        this->coreObjects[i].mtip = false;
        this->coreObjects[i].stip = false;
    }
}

void AClint::reset() {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        this->coreObjects[i].mtimecmp = -1;
        if (this->coreObjects[i].mtimerID != (unsigned int)-1) {
            this->taskTimer.remove_task(this->coreObjects[i].mtimerID);
            this->coreObjects[i].mtimerID = -1;
        }
        if (this->coreObjects[i].stimerID != (unsigned int)-1) {
            this->taskTimer.remove_task(this->coreObjects[i].stimerID);
            this->coreObjects[i].stimerID = -1;
        }
        this->coreObjects[i].msip = false;
        this->coreObjects[i].ssip = false;
        this->coreObjects[i].mtip = false;
        this->coreObjects[i].stip = false;
    }
    this->timerRunning = false;
}

word_t AClint::read(word_t addr, word_t size, bool &valid) {
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
        return this->coreObjects[coreID].mtimecmp;
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIMECMP", size);
            valid = false;
            return -1;
        }

        word_t offset = addr & 0b11;
        valid = true;
        if (offset == 0) {
            return this->coreObjects[coreID].mtimecmp & 0xffffffffUL;
        } else {
            return this->coreObjects[coreID].mtimecmp >> 32;
        }
        #endif
    } else if (MTIME.in_range(addr)) {
        #ifdef KXEMU_ISA64
        if (size != 8 || (addr & 0x7) != 0) {
            WARN("Unaligned access to MTIME. size=%lu, addr=" FMT_WORD64, size, addr);
            return -1;
        }

        valid = true;
        return cpu::realtime_to_mtime(this->get_uptime());
        
        #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIME", size);
            return -1;
        }
        
        valid = true;
        word_t offset = addr - MTIME.BASE;
        word_t mtime = cpu::realtime_to_mtime(this->get_uptime());
        if (offset == 0) {
            return mtime & 0xffffffffUL;
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
        
        this->coreObjects[coreID].mtimecmp = value;
        this->update_core_mtimecmp(coreID);
    #else
        if (size != 4 || (addr & 0x3) != 0) {
            WARN("Invalid size %lu for MTIMECMP", size);
            return false;
        }
        word_t offset = addr & 0b100;
        if (offset == 0) {
            this->coreObjects[coreID].mtimecmp &= ~0xffffffffUL;
            this->coreObjects[coreID].mtimecmp |= value;
        } else {
            this->coreObjects[coreID].mtimecmp &= 0xffffffffUL;
            this->coreObjects[coreID].mtimecmp |= (uint64_t)value << 32;
            this->update_core_mtimecmp(coreID);
        }
        #endif
    } else if (MTIME.in_range(addr)) {
        return false;
    } else {
        WARN("Invalid address " FMT_WORD64 " for CLINT", addr);
        return false;
    }
    
    return true;
}

void AClint::update() {
    std::lock_guard<std::mutex> lock(this->mtx);
    for (unsigned int i = 0; i < this->coreCount; i++) {
        CoreObject &coreObj = this->coreObjects[i];
        if (coreObj.mtip) {
            coreObj.core->set_timer_interrupt_m();
            coreObj.mtip = false;
        }
        if (coreObj.stip) {
            coreObj.core->set_timer_interrupt_s();
            coreObj.stip = false;
        }
    }
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
    uint64_t uptime = utils::get_current_time() - this->bootTime;
    return uptime;
}

void AClint::register_stimer(unsigned int coreID, uint64_t stimecmp) {
    if (coreID >= this->coreCount) {
        PANIC("Invalid core ID %d", coreID);
        return ;
    }

    CoreObject *coreObj = &this->coreObjects[coreID];
    
    if (coreObj->stimerID != (unsigned int)-1) {
        this->taskTimer.remove_task(coreObj->stimerID);
    }

    uint64_t uptimecmp = cpu::mtime_to_realtime(stimecmp);
    uint64_t uptime = this->get_uptime();
    if (uptimecmp <= uptime) {
        // If the stimecmp is already passed, we can just set the interrupt immediately
        coreObj->stip = true;
        coreObj->stimerID = -1;
        return ;
    }
    uint64_t delay = uptimecmp - uptime;
    this->coreObjects[coreID].stimerID = this->taskTimer.add_task(delay, [this, coreObj]() {
        std::lock_guard<std::mutex> lock(this->mtx);
        coreObj->stip = true;
        coreObj->stimerID = -1;
    });
}

void AClint::update_core_mtimecmp(unsigned int coreID) {
    if (coreID >= this->coreCount) {
        PANIC("Invalid core ID %d", coreID);
        return ;
    }

    CoreObject *coreObj = &this->coreObjects[coreID];

    if (coreObj->mtimerID != (unsigned int)-1) {
        this->taskTimer.remove_task(coreObj->mtimerID);
    }

    coreObj->core->clear_timer_interrupt_m();

    uint64_t mtimecmp = coreObj->mtimecmp;
    uint64_t uptimecmp = cpu::mtime_to_realtime(mtimecmp);
    uint64_t uptime = this->get_uptime();
    uint64_t delay = uptimecmp - uptime;
    coreObj->mtimerID = this->taskTimer.add_task(delay, [this, coreObj]() {
        std::lock_guard<std::mutex> lock(this->mtx);
        coreObj->mtip = true;
        coreObj->mtimerID = -1;
    });
}

const char *AClint::get_type_name() const {
    return "AClint";
}
