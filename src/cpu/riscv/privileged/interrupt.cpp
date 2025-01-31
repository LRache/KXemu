#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "log.h"
#include "macro.h"

#include <cstdint>

using namespace kxemu::cpu;

void RVCore::update_mtimecmp() {
    // Remove the previous task first
    if (this->mtimerTaskID != (unsigned int)-1) {
        this->taskTimer->remove_task(this->mtimerTaskID);
    }

    uint64_t uptimecmp = MTIME_TO_UPTIME(this->mtimecmp);
    uint64_t uptime = this->get_uptime();
    uint64_t delay = uptimecmp - uptime;
    this->taskTimer->add_task(delay, [this]() {
        this->set_interrupt(INTERRUPT_TIMER_M);
        this->mtimerTaskID = -1;
    });
}

void RVCore::update_stimecmp() {
    // Remove the previous task first
    if (this->stimerTaskID != (unsigned int)-1) {
        this->taskTimer->remove_task(this->stimerTaskID);
    }

    uint64_t cmp = this->csr.read_csr(CSR_STIMECMP);
#ifdef KXEMU_ISA32
    cmp &= 0xffffffff;
    cmp |= (uint64_t)this->csr.read_csr(CSR_STIMECMPH) << 32;
#endif
    uint64_t uptimecmp = MTIME_TO_UPTIME(cmp);
    uint64_t uptime = this->get_uptime();
    uint64_t delay = uptimecmp - uptime;
    this->taskTimer->add_task(delay, [this]() {
        this->set_interrupt(INTERRUPT_TIMER_S);
        this->stimerTaskID = -1;
        // INFO("STIMER interrupt triggered, mip=" FMT_WORD, this->csr.read_csr(CSR_MIP));
    });
    // INFO("Set STIMECMP, delay=" FMT_VARU, delay);
}

void RVCore::set_interrupt(word_t code) {
    word_t mip = this->csr.read_csr(CSR_MIP);
    mip |= 1 << code;
    this->csr.write_csr(CSR_MIP, mip);
}

void RVCore::clear_interrupt(word_t code) {
    word_t mip = this->csr.read_csr(CSR_MIP);
    mip &= ~(1 << code);
    this->csr.write_csr(CSR_MIP, mip);
}

bool RVCore::check_timer_interrupt() {
    // if (unlikely(this->timerIntrruptNotTriggered && this->uptime >= this->uptimecmp)) {
    //     this->set_interrupt(INTERRUPT_TIMER_M);
    //     this->timerIntrruptNotTriggered = false;
    //     return true;
    // }
    // if (unlikely(this->stimerIntrruptNotTriggered && this->uptime >= this->suptimecmp)) {
    //     this->set_interrupt(INTERRUPT_TIMER_S);
    //     this->stimerIntrruptNotTriggered = false;
    //     return true;
    // }
    return false;
}

void RVCore::set_external_interrupt_m() {
    set_interrupt(INTERRUPT_EXTERNAL_M);
}

void RVCore::set_external_interrupt_s() {
    set_interrupt(INTERRUPT_EXTERNAL_S);
}

void RVCore::clear_external_interrupt_m() {
    clear_interrupt(INTERRUPT_EXTERNAL_M);
}

void RVCore::clear_external_interrupt_s() {
    clear_interrupt(INTERRUPT_EXTERNAL_S);
}

void RVCore::interrupt_m(word_t code) {
    INFO("Machine level interrupt, code=" FMT_WORD, code);
    clear_interrupt(code);
    
    this->csr.write_csr(CSR_MEPC, this->pc);
    this->csr.write_csr(CSR_MCAUSE, code | CAUSE_INTERRUPT_MASK);
    this->csr.write_csr(CSR_MTVAL, 0);

    word_t mstatus = this->csr.read_csr(CSR_MSTATUS);
    mstatus = (mstatus & ~STATUS_MPP_MASK) | (this->privMode << STATUS_MPP_OFF);
    mstatus = (mstatus & ~STATUS_MPIE_MASK) | ((mstatus & STATUS_MIE_MASK) << (STATUS_MPIE_OFF - STATUS_MIE_OFF));
    mstatus = (mstatus & ~STATUS_MIE_MASK);
    this->csr.write_csr(CSR_MSTATUS, mstatus);

    this->privMode = PrivMode::MACHINE;

    word_t mtvec = this->csr.read_csr(CSR_MTVEC);
    word_t vecMode = mtvec & TVEC_MODE_MASK;
    if (vecMode == TVEC_MODE_VECTORED) {
        this->npc = (mtvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = mtvec & ~TVEC_MODE_MASK;
    }
}

void RVCore::interrupt_s(word_t code) {
    // INFO("Interrupt S, code=" FMT_WORD, code);
    clear_interrupt(code);
    
    this->csr.write_csr(CSR_SEPC, this->pc);
    this->csr.write_csr(CSR_SCAUSE, code | CAUSE_INTERRUPT_MASK);
    this->csr.write_csr(CSR_STVAL, 0);

    word_t mstatus = this->csr.read_csr(CSR_MSTATUS);
    mstatus = (mstatus & ~STATUS_SPP_MASK) | (this->privMode << STATUS_SPP_OFF);
    mstatus = (mstatus & ~STATUS_SPIE_MASK) | ((mstatus & STATUS_SIE_MASK) << (STATUS_SPIE_OFF - STATUS_SIE_OFF));
    mstatus = (mstatus & ~STATUS_SIE_MASK);
    this->write_csr(CSR_MSTATUS, mstatus);

    this->privMode = PrivMode::SUPERVISOR;

    word_t stvec = this->csr.read_csr(CSR_STVEC);
    word_t vecMode = stvec & TVEC_MODE_MASK;
    if (vecMode == TVEC_MODE_VECTORED) {
        this->npc = (stvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = stvec & ~TVEC_MODE_MASK;
    }
}

static constexpr word_t INTER_BITS[] = {
    INTERRUPT_SOFTWARE_S,
    INTERRUPT_SOFTWARE_M,
    INTERRUPT_TIMER_S,
    INTERRUPT_TIMER_M,
    INTERRUPT_EXTERNAL_S,
    INTERRUPT_EXTERNAL_M
};

bool RVCore::scan_interrupt() {
    if (likely(this->privMode == PrivMode::MACHINE    && !(*this->mstatus & STATUS_MIE_MASK))) return false;
    if (likely(this->privMode == PrivMode::SUPERVISOR && !(*this->mstatus & STATUS_SIE_MASK))) return false;

    if (*this->mip == 0) return false;

    // Machine level interrupt
    word_t pending;
    if (*this->mstatus & STATUS_MIE_MASK) {
        pending = *this->mip & *this->mie & ~*this->mideleg;
        if (pending) {
            for (unsigned int i = 0; i < sizeof(INTER_BITS) / sizeof(INTER_BITS[0]); i++) {
                if (pending & (1 << INTER_BITS[i])) {
                    interrupt_m(INTER_BITS[i]);
                    return true;
                }
            }
        }
    }
    
    // Supervisor level interrupt
    if (*this->mstatus & STATUS_SIE_MASK) {
        pending = *this->mip & *this->mie & *this->mideleg;
        if (pending) {
            for (unsigned int i = 0; i < sizeof(INTER_BITS) / sizeof(INTER_BITS[0]); i++) {
                if (pending & (1 << INTER_BITS[i])) {
                    interrupt_s(INTER_BITS[i]);
                    return true;
                }
            }
        }
    }

    return false;
}
