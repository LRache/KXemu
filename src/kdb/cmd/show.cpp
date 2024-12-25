#include "isa/word.h"
#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/disasm.h"
#include "utils/utils.h"

#include <bitset>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <vector>

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
    uint8_t *mem = kdb::memory->get_ptr(addr);
    word_t memSize = kdb::memory->get_ptr_length(addr);
    for (unsigned int i = 0; i < count; i++) {
        std::cout << FMT_STREAM_WORD(addr) << ": ";
        if (memSize < size) {
            std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(addr) << "." << std::endl;
            break;
        }
        f(mem, size);
        memSize -= size;
        mem += size;
        addr += size;
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

static void show_char(uint8_t *mem, unsigned int size) {
    std::cout << *(char *)mem << std::endl;
}

static void show_float(uint8_t *mem, unsigned int size) {
    switch (size) {
        case 4: std::cout << *(float *)mem; break;
        case 8: std::cout << *(double *)mem; break;
    }
    std::cout << std::endl;
}

static void show_inst(unsigned int count, unsigned int size, word_t addr) {
    uint8_t *mem = kdb::memory->get_ptr(addr);
    word_t memSize = kdb::memory->get_ptr_length(addr);

    for (unsigned int i = 0; i < count; i++) {
        std::cout << FMT_STREAM_WORD(addr) << ": ";
        if (memSize == 0) {
            std::cout << "Cannot access memory at address " << FMT_STREAM_WORD(addr) << "." << std::endl;
            break;
        }

        unsigned int instLen;
        auto disasmStr = disasm::disassemble(mem, memSize, addr, instLen);

        if (instLen == 0) {
            std::cout << "Unsupport to disassemble at " << FMT_STREAM_WORD(addr) << std::endl;
            break;
        }

        std::cout << std::hex;
        for (unsigned int j = 0; j < instLen; j++) {
            std::cout << std::setw(2) << std::setfill('0') << (uint64_t)mem[j] << ' ';
        }
        std::cout << disasmStr << std::endl;
        memSize -= instLen;
        mem += instLen;
        addr += instLen;
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
    }

    return cmd::Success; 
}
