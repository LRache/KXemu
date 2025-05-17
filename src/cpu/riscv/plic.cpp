#include "cpu/riscv/plic.h"
#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "device/bus.h"
#include "device/def.h"
#include "device/mmio.h"
#include "log.h"

#include <cstdint>

using namespace kxemu::device;

static inline constexpr AddrSpace PLIC_PRIORITY = {0x000000, 0x001000};
static inline constexpr AddrSpace PLIC_PENDING  = {0x001000, 0x000080};
static inline constexpr AddrSpace PLIC_ENABLE   = {0x002000, 0x1f0000};
static inline constexpr AddrSpace PLIC_CONTEXT  = {0x200000, 0x100000};

void PLIC::init(cpu::RVCore *cores, unsigned int coreCount) {
    for (unsigned int i = 0; i < 32; i++) {
        this->interruptSources[i].priority = 0;
        this->interruptSources[i].pending = false;
        for (unsigned int j = 0; j < 32; j++) {
            this->interruptSources[i].enable[j] = false;
        }
    }

    for (unsigned int i = 0; i < 32; i++) {
        this->targetContexts[i].threshold = 0;
        this->targetContexts[i].claim = 0;
        this->targetContexts[i].core = nullptr;
    }

    for (unsigned int i = 0; i < coreCount; i++) {
        this->targetContexts[i * 2    ].core = &cores[i];
        this->targetContexts[i * 2 + 1].core = &cores[i];
    }
}

void PLIC::reset() {}

word_t PLIC::read(word_t offset, word_t size, bool &valid) {
    if (size != 4) {
        valid = false;
        return 0;
    }

    valid = true;
    if (PLIC_PRIORITY.in_range(offset)) {
        unsigned int source = offset / 4;
        if (source < 32) {
            return this->interruptSources[source].priority;
        } else {
            return 0;
        }
    } else if (PLIC_PENDING.in_range(offset)) {
        unsigned int t = (offset - PLIC_PENDING.BASE) / 4;
        if (t < 4) {
            uint32_t pending = 0;
            for (unsigned int i = 0; i < 32; i++) {
                if (this->interruptSources[t * 8 + i].pending) {
                    pending |= 1 << i;
                }
            }
            return pending;
        } else {
            return 0;
        }
    } else if (PLIC_ENABLE.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_ENABLE.BASE) / 0x80;
        if ((offset - PLIC_ENABLE.BASE) % 0x80 == 0) { // Only 32 interrupt sources
            uint32_t enable = 0;
            for (unsigned int i = 0; i < 32; i++) {
                if (this->interruptSources[i].enable[contextID]) {
                    enable |= 1 << i;
                }
            }
            return enable;
        } else {
            return 0;
        }
    } else if (PLIC_CONTEXT.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_CONTEXT.BASE) / 0x1000;
        if (contextID >= 32) {
            return 0;
        }
        if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 0) {
            return this->targetContexts[contextID].threshold;
        } else if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 4) {
            return this->targetContexts[contextID].claim;
        }
        return 0;
    } else {
        valid = false;
        return 0;
    }
}

bool PLIC::write(word_t offset, word_t data, word_t size) {
    if (size != 4) {
        WARN("Unaligned write to PLIC");
        return false;
    }

    if (PLIC_PRIORITY.in_range(offset)) {
        unsigned int source = offset / 4;
        if (source < 32) {
            this->interruptSources[source].priority = data;
            return true;
        } else {
            HINT("Source index out of range: index=%u", source);
            return true;
        }
    } else if (PLIC_ENABLE.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_ENABLE.BASE) / 0x80;
        unsigned int index = (offset - PLIC_ENABLE.BASE) % 0x80;
        if (index == 0) { // Only 32 interrupt sources
            for (unsigned int i = 0; i < 32; i++) {
                this->interruptSources[i].enable[contextID] = (data & (1 << i)) != 0;
            }
            return true;
        } else {
            HINT("Interrupt enable bits index out of range: index=%u", index);
            return true;
        }
    } else if (PLIC_CONTEXT.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_CONTEXT.BASE) / 0x1000;
        if (contextID >= 32) {
            return false;
        }
        if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 0) {
            this->targetContexts[contextID].threshold = data;
            return true;
        } else if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 4) {
            if (this->targetContexts[contextID].claim == data) {
                this->targetContexts[contextID].claim = 0;
                if (contextID % 2 == 0) {
                    this->targetContexts[contextID].core->clear_external_interrupt_m();
                } else {
                    this->targetContexts[contextID].core->clear_external_interrupt_s();
                }
            }
            return true;
        }
        return false;
    } else {
        return false;
    }
}

void PLIC::connect_to_bus(Bus *bus) {
    this->bus = bus;
}

void PLIC::scan_and_set_interrupt(unsigned int hartid, int privMode) {
    unsigned int contextID;
    if (privMode == cpu::PrivMode::MACHINE) {
        contextID = hartid * 2;
    } else {
        contextID = hartid * 2 + 1;
    }
    
    TargetContext &target = this->targetContexts[contextID];
    if (target.claim != 0) {
        return;
    }
    
    uint32_t priority = target.threshold;
    uint32_t claim = 0;
    MMIODev *sourceDev = nullptr;
    
    for (auto &map : this->bus->mmioMaps) {
        auto &dev = map->dev;
        if (dev->interrupt_pending()) {
            auto &source = this->interruptSources[map->id];
            
            source.pending = true;
            sourceDev = dev;
            
            if (source.enable[contextID] && source.priority >= priority) {
                priority = source.priority;
                claim = map->id;
            }
        }
    }

    if (claim != 0) {
        target.claim = claim;
        sourceDev->clear_interrupt();
        if (privMode == cpu::PrivMode::MACHINE) {
            target.core->set_external_interrupt_m();
        } else {
            target.core->set_external_interrupt_s();
        }
    }
}
