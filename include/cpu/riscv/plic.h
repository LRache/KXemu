#ifndef __KXEMU_CPU_RISCV_PLIC_H__
#define __KXEMU_CPU_RISCV_PLIC_H__

#include "device/mmio.h"

namespace kxemu::device {

class PLIC : public device::MMIOMap {
private:
    Bus *bus;
public:
    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;
    void connect_to_bus(Bus *bus) override;

    bool scan_interrupt();
};

} // namespace kxemu::cpu

#endif
