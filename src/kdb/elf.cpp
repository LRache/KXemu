#include "common.h"
#include "kdb/kdb.h"
#include "isa/isa.h"

#include <cstddef>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <ios>
#include <iostream>

#ifdef ISA64
    #define Elf_Ehdr Elf64_Ehdr
    #define Elf_Phdr Elf64_Phdr 
    #define EXPECTED_CLASS ELFCLASS64
#else
    #define Elf_Ehdr Elf32_Ehdr
    #define Elf_Phdr Elf32_Phdr
    #define EXPECTED_CLASS ELFCLASS32
#endif

#ifdef ISA
    #if ISA == RISCV32
        #define EXPECTED_ISA EM_RISCV
    #endif
#endif

#define CHECK_READ_SUCCESS(expectedSize) \
do { \
    if (f.gcount() != expectedSize) { \
        std::cout << "An error occurred when read from file." << std::endl; \
        f.close(); \
        return false; \
    } \
} while (0) 

static bool check_is_valid_elf(const Elf_Ehdr &ehdr) {
    // check elf magic number
    if (!(
        ehdr.e_ident[EI_MAG0] == 0x7f ||
        ehdr.e_ident[EI_MAG1] == 'E'  ||
        ehdr.e_ident[EI_MAG2] == 'L'  ||
        ehdr.e_ident[EI_MAG3] == 'F'
    )) {
        return false;
    }

    // check n-bits
    if (ehdr.e_ident[EI_CLASS] != EXPECTED_CLASS) {
        return false;
    }

    // check isa
    if (ehdr.e_machine != EXPECTED_ISA) {
        return false;
    }

    return true;
}

static bool load_program(std::fstream &f, const Elf_Phdr &phdr) {
    return true;
}

word_t kdb::load_elf(const std::string &filename) {
    std::fstream f;
    f.open(filename, std::ios_base::in);
    if (!f.is_open()) {
        return 0;
    }

    Elf_Ehdr ehdr;
    f.read((char *)&ehdr, sizeof(ehdr));
    CHECK_READ_SUCCESS(sizeof(ehdr));

    if (!check_is_valid_elf(ehdr)) {
        std::cout << "BAD elf header" << std::endl;
        return 0;
    }

    // read program header
    f.seekp(ehdr.e_phoff, std::ios::beg); // seek to program header array
    for (unsigned int i = 0; i < ehdr.e_phentsize; i++) {
        Elf_Phdr phdr;
        f
    }
    f.close();
    
    return ehdr.e_entry;
}
