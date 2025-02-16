#ifndef __KXEMU_DEVICE_MMIO_H__
#define __KXEMU_DEVICE_MMIO_H__

#include "device/def.h"

namespace kxemu::device {

class MMIOMap {
public:
    virtual ~MMIOMap() {};
        
    virtual void reset() {};
    virtual word_t read(word_t offset, word_t size, bool &valid) = 0;
    virtual bool write(word_t offset, word_t data, word_t size) = 0;
    virtual void update() {};
            
    virtual word_t do_atomic(word_t addr, word_t data, int size, AMO amo, bool &valid) {
        valid = false;
        return 0;
    }
           
    virtual uint8_t *get_ptr(word_t offset) {
        return nullptr;
    }
            
    virtual const char *get_type_name() const {
        return "MMIO";
    }
};

} // namespace kxemu::device


#endif
