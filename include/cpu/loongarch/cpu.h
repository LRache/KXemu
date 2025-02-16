#ifndef __KXEMU_CPU_LOONGARCH_CPU_H__
#define __KXEMU_CPU_LOONGARCH_CPU_H__

#include "cpu/cpu.h"
#include "cpu/word.h"
#include "cpu/loongarch/core.h"

namespace kxemu::cpu {

class LACPU : public CPU<word_t> {
private:
    LACore *cores;
    unsigned int coreCount;

public:
    LACPU();
    ~LACPU() override;

    void init(device::Bus *bus, int flags, unsigned int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    void run(bool blocked=false, const word_t *breakpoints=nullptr, unsigned int n=0) override;
    void join() override;
    bool is_running() override;

    unsigned int core_count() override;
    LACore *get_core(unsigned int coreID) override;
};

}

#endif
