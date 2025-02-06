#ifndef __KXEMU_DEVICE_MEMORY_H__
#define __KXEMU_DEVICE_MEMORY_H__

#include "device/bus.h"

namespace kxemu::device {

class Memory : public MemoryMap {
private:
    uint64_t length;
public:
    Memory(uint64_t length);
    ~Memory();
    
    word_t read(word_t addr, int size) override;
    bool write(word_t addr, word_t data, int size) override;
    uint8_t *get_ptr(word_t addr) override;
    const char *get_type_name() const override;

    uint8_t *data;

    word_t do_atomic(word_t addr, word_t data, int size, AMO amo, bool &valid) override;
};

} // namespace kxemu::device

#endif
