#include "device/syscon.hpp"
#include "log.h"

using namespace kxemu;
using namespace kxemu::device;

// QEMU's sifive_test read handler returns 0 for the whole 0x1000 window and
// allows 2- and 4-byte accesses. OpenSBI and Linux regmap read the register
// before updating it, so a valid zero read is required.
word_t Syscon::read(word_t, word_t size, bool &valid) {
    if (size != 2 && size != 4) {
        valid = false;
        return 0;
    }

    valid = true;
    return 0;
}

bool Syscon::write(word_t offset, word_t data, word_t size) {
    if (size != 2 && size != 4) {
        return false;
    }

    if (offset != 0) {
        WARN("syscon: ignored write at offset 0x%llx", (unsigned long long)offset);
        return true;
    }

    const word_t status = data & 0xffff;
    const unsigned int code = (unsigned int)((data >> 16) & 0xffff);

    switch (status) {
        case FINISHER_PASS:
        case FINISHER_FAIL:
            if (!this->terminalFired.exchange(true) && this->handler) {
                const Action action = status == FINISHER_PASS
                    ? Action::Poweroff
                    : Action::Fail;
                this->handler(action, code);
            }
            return true;
        case FINISHER_RESET:
            if (this->handler) {
                this->handler(Action::Reset, code);
            }
            return true;
        default:
            // QEMU logs a guest error and keeps running for unknown values.
            WARN("syscon: ignored unknown finisher value 0x%llx", (unsigned long long)data);
            return true;
    }
}
