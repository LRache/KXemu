#include "kdb/cmd.hpp"
#include "kdb/kdb.hpp"
#include "macro.h"
#include "word.h"

#include <bitset>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>

using namespace kxemu;
using namespace kxemu::kdb;

/*
    arg: /<n/f/c> addr
    n: unit count
    f: format
        x: hexadecimal
        d: decimal
        u: unsinged decimal
        o: octal
        t: binary
        s: string
        c: char
        f: floating point
        i: instruction
    c: count of bytes in one unit
        b: 1 byte
        h: 2 bytes
        w: 4 bytes
        g: 8 bytes
        when format=x, d, o, t, c
*/

enum Format{
    HEX,  // x
    DEC,  // d
    UDEC, // u
    OCT,  // o
    BIN,  // t
    FLOAT,// f
    CHAR, // c
    STR,  // s
    INST, // i
};

typedef void (*func_t)(uint8_t *, unsigned int);

static void show_mem_value_helper(unsigned int count, unsigned int size, word_t addr, func_t f) {
    bool valid;
    word_t vaddr = addr;
    word_t paddr = kdb::cpu->get_core(0)->vaddr_translate(addr, valid);
    if (!valid) {
        paddr = addr;
    }

    uint8_t *mem = (uint8_t *)kdb::bus->get_ptr(paddr);
    if (mem == nullptr) {
        std::cout << "Cannot access memory at address " << FMT_FG_BLUE << FMT_STREAM_WORD(vaddr) << FMT_FG_RESET << "." << std::endl;
        return;
    }
    
    word_t memSize = kdb::bus->get_ptr_length(paddr);
    for (unsigned int i = 0; i < count; i++) {
        if (vaddr != paddr) {
            std::cout << "(paddr=" << FMT_STREAM_WORD(paddr) << ") ";
        }
        std::cout << FMT_FG_BLUE << FMT_STREAM_WORD(vaddr) << FMT_FG_RESET << ": ";
        if (memSize < size) {
            std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(vaddr) << "." << std::endl;
            break;
        }
        f(mem, size);
        memSize -= size;
        mem += size;
        vaddr += size;
        paddr += size;
    }
}

static void show_hex(uint8_t *mem, unsigned int size) {
    std::cout << "0x" << std::hex << std::setw(size * 2) << std::setfill('0');
    switch (size) {
        case 1: std::cout << *(uint8_t  *)mem; break;
        case 2: std::cout << *(uint16_t *)mem; break;
        case 4: std::cout << *(uint32_t *)mem; break;
        case 8: std::cout << *(uint64_t *)mem; break;
    }
    std::cout << std::dec << std::endl;
}

static void show_dec(uint8_t *mem, unsigned int size) {
    std::cout << std::dec;
    switch (size) {
        case 1: std::cout << (int64_t)*(int8_t  *)mem; break;
        case 2: std::cout << (int64_t)*(int16_t *)mem; break;
        case 4: std::cout << (int64_t)*(int32_t *)mem; break;
        case 8: std::cout << (int64_t)*(int64_t *)mem; break;
    }
    std::cout << std::endl;
}

static void show_udec(uint8_t *mem, unsigned int size) {
    std::cout << std::dec;
    switch (size) {
        case 1: std::cout << *(uint8_t  *)mem; break;
        case 2: std::cout << *(uint16_t *)mem; break;
        case 4: std::cout << *(uint32_t *)mem; break;
        case 8: std::cout << *(uint64_t *)mem; break;
    }
    std::cout << std::endl;
}

static void show_oct(uint8_t *mem, unsigned int size) {
    std::cout << "0o" << std::oct;
    switch (size) {
        case 1: std::cout << *(uint8_t  *)mem; break;
        case 2: std::cout << *(uint16_t *)mem; break;
        case 4: std::cout << *(uint32_t *)mem; break;
        case 8: std::cout << *(uint64_t *)mem; break;
    }
    std::cout << std::endl;
}

static void show_bin(uint8_t *mem, unsigned int size) {
    std::cout << "0b";
    switch (size) {
        case 1: std::cout << std::bitset<sizeof(uint8_t ) * 8>(*(uint8_t *)mem); break;
        case 2: std::cout << std::bitset<sizeof(uint16_t) * 8>(*(uint8_t *)mem); break;
        case 4: std::cout << std::bitset<sizeof(uint32_t) * 8>(*(uint8_t *)mem); break;
        case 8: std::cout << std::bitset<sizeof(uint64_t) * 8>(*(uint8_t *)mem); break;
    }
    std::cout << std::endl;
}

static void show_char(uint8_t *mem, unsigned int) {
    std::cout << *(char *)mem << std::endl;
}

static void show_float(uint8_t *mem, unsigned int size) {
    switch (size) {
        case 4: std::cout << *(float *)mem; break;
        case 8: std::cout << *(double *)mem; break;
    }
    std::cout << std::endl;
}

static void show_inst(unsigned int count, unsigned int, word_t addr) {
    bool valid;
    word_t vaddr = addr;
    word_t paddr = kdb::cpu->get_core(0)->vaddr_translate(addr, valid);
    if (!valid) {
        paddr = addr;
    }
    
    uint8_t *mem = (uint8_t *)kdb::bus->get_ptr(paddr);
    if (mem == nullptr) {
        std::cout << "Cannot access memory at address " << FMT_FG_BLUE << FMT_STREAM_WORD(vaddr) << FMT_FG_RESET << "." << std::endl;
        return;
    }
    
    word_t memSize = kdb::bus->get_ptr_length(paddr);

    for (unsigned int i = 0; i < count; i++) {
        if (vaddr != paddr) {
            std::cout << "(paddr=" << FMT_STREAM_WORD(paddr) << ") ";
        }
        std::cout << FMT_FG_BLUE << FMT_STREAM_WORD(vaddr) << FMT_FG_RESET << ": ";
        if (memSize == 0) {
            std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(vaddr) << "." << std::endl;
            break;
        }

        uint64_t instLen;
        auto disasmStr = isa::disassemble(mem, memSize, addr, instLen);

        if (instLen == 0) {
            std::cout << "Unsupport to disassemble at " << FMT_STREAM_WORD(vaddr) << std::endl;
            break;
        }

        word_t symbolOffset = 0;
        auto symbolName = kdb::addr_match_symbol(vaddr, symbolOffset);
        if (symbolName != std::nullopt) {
            std::cout << "<" << FMT_FG_YELLOW << symbolName.value() << FMT_FG_RESET << "+" << symbolOffset << "> ";
        }

        std::cout << "0x" << std::hex;
        for (unsigned int j = instLen; j > 0; j--) {
            std::cout << std::setw(2) << std::setfill('0') << (uint64_t)mem[j];
        }
        std::cout << disasmStr << std::endl;
        memSize -= instLen;
        mem += instLen;
        vaddr += instLen;
        paddr += instLen;
    }
}

static void show_string(unsigned int count, word_t addr) {
    bool valid;
    word_t vaddr = addr;
    word_t paddr = kdb::cpu->get_core(0)->vaddr_translate(addr, valid);
    if (!valid) {
        std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(addr) << "." << std::endl;
        return;
    }
    addr = paddr;

    uint8_t *mem = (uint8_t *)kdb::bus->get_ptr(addr);
    word_t memSize = kdb::bus->get_ptr_length(addr);

    for (unsigned int i = 0; i < count; i++) {
        if (memSize == 0) {
            std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(addr) << "." << std::endl;
            break;
        }

        if (vaddr != paddr) {
            std::cout << "(paddr=" << FMT_STREAM_WORD(paddr) << ") ";
        }
        std::cout  << FMT_STREAM_WORD(vaddr) << ": ";
        while (memSize > 0 && *mem != '\0') {
            std::cout << *mem++;
            memSize--;
            vaddr++;
            paddr++;
        }
        std::cout << std::endl;
    }
}

int cmd::show_mem(const args_t &args) {
    if (args.size() < 2) {
        std::cout << "Usage: x /<n/f/c> addr" << std::endl;
        return cmd::InvalidArgs;
    }

    const std::string arg = args[1];
    std::string addrArg;
    word_t addr;
    int format = HEX;
    unsigned int count = 1;
    unsigned int size = 1;
    if (arg[0] == '/') {
        unsigned int n = 0;
        for (std::size_t i = 1; i < arg.length(); i++) {
            char c = arg[i];
            if (std::isdigit(c)) {
                n = n * 10 + c - '0';
            } else {
                switch (c) {
                    case 'x': case 'X': format = HEX;   break;
                    case 'd': case 'D': format = DEC;   break;
                    case 'u': case 'U': format = UDEC;  break;
                    case 'o': case 'O': format = OCT;   break;
                    case 't': case 'T': format = BIN;   break;
                    case 's': case 'S': format = STR;   break;
                    case 'c': case 'C': format = CHAR;  break;
                    case 'f': case 'F': format = FLOAT; break;
                    case 'i': case 'I': format = INST;  break;
                    case 'b': case 'B': size = 1; break;
                    case 'h': case 'H': size = 2; break;
                    case 'w': case 'W': size = 4; break;
                    case 'g': case 'G': size = 8; break;
                }
            } 
        }
        if (n != 0) {
            count = n;
        }
        if (args.size() < 3) {
            return cmd::InvalidArgs;
        }
        addrArg = args[2];
    } else {
        addrArg = args[1];
    }

    bool s;
    addr = kdb::string_to_addr(addrArg, s);
    if (!s) {
        return cmd::InvalidArgs;
    }

    switch (format) {
        case HEX: show_mem_value_helper(count, size, addr, show_hex); break;
        case DEC: show_mem_value_helper(count, size, addr, show_dec); break;
        case UDEC: show_mem_value_helper(count, size, addr, show_udec); break;
        case OCT: show_mem_value_helper(count, size, addr, show_oct); break;
        case BIN: show_mem_value_helper(count, size, addr, show_bin); break;
        case CHAR: show_mem_value_helper(count, 1, addr, show_char); break;
        case FLOAT: show_mem_value_helper(count, size, addr, show_float); break;
        case INST: show_inst(count, size, addr); break;
        case STR: show_string(count, addr); break;
    }

    return cmd::Success; 
}
