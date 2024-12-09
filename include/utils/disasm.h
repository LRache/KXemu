#ifndef __UTILS_DISASM_H__
#define __UTILS_DISASM_H__

#include "common.h"
#include <string>

namespace disasm {
    void init(std::string isaName);
    std::string disassemble(const uint8_t *data, const size_t length, word_t pc, unsigned int &instLen);
}

#endif
