#ifndef __KXEMU_DEVICE_INTC_HPP__
#define __KXEMU_DEVICE_INTC_HPP__

namespace kxemu::device {

// An interrupt controller attached to a Bus. Devices push their IRQ levels to it
// from any thread; without one, the framework injects interrupts into the cores.
class InterruptController {
public:
    virtual ~InterruptController() = default;

    // Level-triggered: drive source `irq` to `level`. Thread-safe; may be called
    // from a device thread, an I/O coroutine or a hart executing an MMIO access.
    virtual void set_irq(unsigned int irq, bool level) = 0;
};

} // namespace kxemu::device

#endif
