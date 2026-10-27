#ifndef __KXEMU_DEVICE_MMIO_HPP__
#define __KXEMU_DEVICE_MMIO_HPP__

#include "device/def.hpp"

namespace kxemu::device {

class Bus;

class MMIODev {
public:
    virtual ~MMIODev() {};
        
    virtual void reset() {};
    virtual word_t read(word_t offset, word_t size, bool &valid) = 0;
    virtual bool write(word_t offset, word_t data, word_t size) = 0;

    virtual void connect_to_bus(Bus *bus) {}

    // Wait until no asynchronous work of the device is in flight, so it no longer
    // touches guest memory or drives its IRQ on its own. Call it before
    // destroying the CPU, the interrupt controller or the device.
    virtual void quiesce() {}

    // Called by Bus::add_mmio_map() before connect_to_bus(). Source 0 has no IRQ line.
    void attach_irq(Bus *bus, unsigned int irq) {
        this->irqBus = bus;
        this->irq = irq;
    }
            
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

protected:
    // Drive this device's IRQ line through the bus interrupt controller.
    // Level-triggered and thread-safe; a no-op without an IRQ line.
    void set_irq_level(bool level);

private:
    Bus *irqBus = nullptr;
    unsigned int irq = 0;
};

} // namespace kxemu::device


#endif
