#ifndef __KXEMU_CPU_RISCV_CPU_HPP__
#define __KXEMU_CPU_RISCV_CPU_HPP__

#include "cpu/cpu.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/plic.hpp"
#include "cpu/riscv/core.hpp"

#include <mutex>
#include <thread>

namespace kxemu::cpu {

class RVCPU : public CPU<word_t> {
private:    
    RVCore *cores;
    unsigned int coreCount;
    std::thread *coreThread;
    void core_thread_worker(unsigned int coreID, const word_t *breakpoints, unsigned int n);
    
    device::AClint aclint;
    device::PLIC plic;
    std::mutex deviceMtx;
    
public:
    RVCPU();
    ~RVCPU() override;

    void init(device::Bus *bus, int flags, unsigned int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    void run(bool blocked=false, const word_t *breakpoints=nullptr, unsigned int n=0) override;
    void join() override;
    bool is_running() override;

    unsigned int core_count() override;
    RVCore *get_core(unsigned int coreID) override;
};

} // namespace kxemu::cpu

#endif
