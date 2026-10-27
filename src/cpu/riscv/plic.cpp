#include "cpu/riscv/plic.hpp"
#include "cpu/riscv/core.hpp"
#include "debug.h"
#include "device/def.hpp"
#include "log.h"

#include <cstdint>

using namespace kxemu::device;

static inline constexpr AddrSpace PLIC_PRIORITY = {0x000000, 0x001000};
static inline constexpr AddrSpace PLIC_PENDING  = {0x001000, 0x000080};
static inline constexpr AddrSpace PLIC_ENABLE   = {0x002000, 0x1f0000};
static inline constexpr AddrSpace PLIC_CONTEXT  = {0x200000, 0x3e00000};

PLIC::PLIC() {
    for (auto &target : this->targetContexts) {
        target.core = nullptr;
        target.output = false; // reset() compares against it
    }
    this->reset();
}

void PLIC::init(std::span<cpu::RVCore *const> cores) {
    Assert(cores.size() <= CONTEXT_COUNT / 2, "PLIC supports at most 16 harts.");
    std::lock_guard<std::mutex> lock(this->mtx);
    for (auto &target : this->targetContexts) {
        target.core = nullptr;
        target.output = false;
    }
    this->activeContexts = cores.size() * 2;
    for (unsigned int i = 0; i < cores.size(); i++) {
        this->targetContexts[i * 2    ].core = cores[i];
        this->targetContexts[i * 2 + 1].core = cores[i];
    }
}

void PLIC::reset() {
    std::lock_guard<std::mutex> lock(this->mtx);
    for (auto &p : this->priority) {
        p = 0;
    }
    for (auto &target : this->targetContexts) {
        target.enable = 0;
        target.threshold = 0;
    }
    this->claimed = 0;
    this->pending = this->level;
    this->update_outputs();
}

unsigned int PLIC::best_source(const TargetContext &target) const {
    uint32_t candidates = this->pending & target.enable;
    if (candidates == 0) return 0;
    uint32_t best = target.threshold;
    unsigned int source = 0;
    for (unsigned int i = 1; i < SOURCE_COUNT; i++) {
        // Ties go to the lowest source ID.
        if ((candidates & (uint32_t{1} << i)) && this->priority[i] > best) {
            best = this->priority[i];
            source = i;
        }
    }
    return source;
}

void PLIC::set_output(unsigned int contextID, bool output) {
    TargetContext &target = this->targetContexts[contextID];
    if (target.output == output || target.core == nullptr) return;
    target.output = output;
    if (contextID % 2 == 0) {
        if (output) target.core->set_external_interrupt_m();
        else        target.core->clear_external_interrupt_m();
    } else {
        if (output) target.core->set_external_interrupt_s();
        else        target.core->clear_external_interrupt_s();
    }
}

void PLIC::update_outputs() {
    // Contexts without a hart have no output to drive.
    for (unsigned int i = 0; i < this->activeContexts; i++) {
        this->set_output(i, this->best_source(this->targetContexts[i]) != 0);
    }
}

void PLIC::set_irq(unsigned int irq, bool level) {
    if (irq == 0 || irq >= SOURCE_COUNT) return;
    const uint32_t bit = uint32_t{1} << irq;
    std::lock_guard<std::mutex> lock(this->mtx);
    if (level) {
        this->level |= bit;
        // The gateway forwards no new request until the claimed one completes.
        if (!(this->claimed & bit)) this->pending |= bit;
    } else {
        this->level &= ~bit;
        this->pending &= ~bit;
    }
    this->update_outputs();
}

word_t PLIC::read(word_t offset, word_t size, bool &valid) {
    if (size != 4) {
        valid = false;
        return 0;
    }

    valid = true;
    std::lock_guard<std::mutex> lock(this->mtx);
    if (PLIC_PRIORITY.in_range(offset)) {
        unsigned int source = offset / 4;
        return source < SOURCE_COUNT ? this->priority[source] : 0;
    } else if (PLIC_PENDING.in_range(offset)) {
        return offset == PLIC_PENDING.BASE ? this->pending : 0;
    } else if (PLIC_ENABLE.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_ENABLE.BASE) / 0x80;
        if (contextID < CONTEXT_COUNT && (offset - PLIC_ENABLE.BASE) % 0x80 == 0) {
            return this->targetContexts[contextID].enable;
        }
        return 0;
    } else if (PLIC_CONTEXT.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_CONTEXT.BASE) / 0x1000;
        if (contextID >= CONTEXT_COUNT) {
            return 0;
        }
        TargetContext &target = this->targetContexts[contextID];
        if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 0) {
            return target.threshold;
        } else if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 4) {
            // Claim
            unsigned int source = this->best_source(target);
            if (source != 0) {
                this->pending &= ~(uint32_t{1} << source);
                this->claimed |=   uint32_t{1} << source;
                this->update_outputs();
            }
            return source;
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

    std::lock_guard<std::mutex> lock(this->mtx);
    if (PLIC_PRIORITY.in_range(offset)) {
        unsigned int source = offset / 4;
        if (source == 0) {
            return true;
        } else if (source < SOURCE_COUNT) {
            this->priority[source] = data;
            this->update_outputs();
            return true;
        } else {
            HINT("Source index out of range: index=%u", source);
            return true;
        }
    } else if (PLIC_PENDING.in_range(offset)) {
        return true; // Read-only
    } else if (PLIC_ENABLE.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_ENABLE.BASE) / 0x80;
        unsigned int index = (offset - PLIC_ENABLE.BASE) % 0x80;
        if (contextID >= CONTEXT_COUNT) {
            return true;
        }
        if (index == 0) { // Only 32 interrupt sources; source 0 is hardwired to 0.
            this->targetContexts[contextID].enable = data & ~uint32_t{1};
            this->update_outputs();
            return true;
        } else {
            HINT("Interrupt enable bits index out of range: index=%u", index);
            return true;
        }
    } else if (PLIC_CONTEXT.in_range(offset)) {
        unsigned int contextID = (offset - PLIC_CONTEXT.BASE) / 0x1000;
        if (contextID >= CONTEXT_COUNT) {
            return false;
        }
        TargetContext &target = this->targetContexts[contextID];
        if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 0) {
            target.threshold = data;
            this->update_outputs();
            return true;
        } else if ((offset - PLIC_CONTEXT.BASE) % 0x1000 == 4) {
            // Complete: ignored for sources not enabled for this context.
            if (data != 0 && data < SOURCE_COUNT && (target.enable & (uint32_t{1} << data))) {
                const uint32_t bit = uint32_t{1} << data;
                this->claimed &= ~bit;
                // A source still asserting raises a new request.
                if (this->level & bit) this->pending |= bit;
                this->update_outputs();
            }
            return true;
        }
        return false;
    } else {
        return false;
    }
}
