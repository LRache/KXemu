#include "kdb/kdb.h"
#include "log.h"
#include "config/config.h"

#include <cstdint>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <vector>

#ifdef ISA64
    #define Elf_Ehdr Elf64_Ehdr
    #define Elf_Phdr Elf64_Phdr
    #define Elf_Shdr Elf64_Shdr
    #define Elf_Sym  Elf64_Sym
    #define Elf_Word Elf64_Word
    #define Elf_Addr Elf64_Addr
    #define Elf_Off  Elf64_Off
    #define EXPECTED_CLASS ELFCLASS64
#else
    #define Elf_Ehdr Elf32_Ehdr
    #define Elf_Phdr Elf32_Phdr
    #define Elf_Shdr Elf32_Shdr
    #define Elf_Sym  Elf32_Sym
    #define Elf_Word Elf32_Word
    #define Elf_Addr Elf32_Addr
    #define Elf_Off  Elf32_Off
    #define EXPECTED_CLASS ELFCLASS32
#endif

#ifdef ISA
    #if ISA == riscv32
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

using namespace kxemu;

std::map<kdb::word_t, std::string> kdb::symbolTable;

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

static bool load_program(const Elf_Phdr &phdr, std::fstream &f) {
    if (phdr.p_type != PT_LOAD) {
        return true;
    }

    Elf_Addr start  = phdr.p_vaddr;
    Elf_Word filesz = phdr.p_filesz;
    Elf_Word memsze = phdr.p_memsz;

    if (memsze == 0) return true;

    f.seekg(phdr.p_offset, std::ios::beg);
    kdb::bus->memset(start, memsze, 0); // clear memory
    kdb::bus->load_from_stream(f, start, filesz); // copy ELF file to memory
    
    return true;
}

static bool load_symbol_table(const Elf_Shdr &symtabShdr, const Elf_Shdr &strtabShdr, std::fstream &f) {
    Elf_Off  symtabStart = symtabShdr.sh_offset;
    Elf_Word symtabSize  = symtabShdr.sh_size;
    Elf_Off  strtabStart = strtabShdr.sh_offset;

    unsigned int symbolCount = symtabSize / sizeof(Elf_Sym);

    kdb::symbolTable.clear();
    Elf_Off offset = symtabStart;
    for (unsigned int i = 0; i < symbolCount; i++) {
        Elf_Sym sym;
        f.seekg(offset, std::ios::beg);
        f.read((char *)&sym, sizeof(sym));
        CHECK_READ_SUCCESS(sizeof(sym));
        offset = f.tellg();

        if (sym.st_name == 0) {
            continue;
        }

        char name[256];
        f.seekg(strtabStart + sym.st_name, std::ios::beg);
        f.getline(name, 256, '\0');
        kdb::symbolTable[sym.st_value] = name;
    }
    return true;
}

// ELF file format see https://www.man7.org/linux/man-pages/man5/elf.5.html
// Load elf file from local disk to memory and build symbol table for debug.
kdb::word_t kdb::load_elf(const std::string &filename) {
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
    std::vector<Elf_Phdr> phdrArray;
    f.seekg(ehdr.e_phoff, std::ios::beg);
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        Elf_Phdr phdr;
        f.read((char *)&phdr, sizeof(phdr));
        CHECK_READ_SUCCESS(sizeof(phdr));
        phdrArray.push_back(phdr);
    }
    DEBUG("Load ELF: find %d programs", ehdr.e_phnum);

    for (auto phdr: phdrArray) {
        load_program(phdr, f);
    }

    // read section header to load symbol table
    Elf_Shdr symtabShdr = {}, strtabShdr = {};
    bool symtabFound = false;
    bool strtabFound = false;
    uint16_t shstrndx = ehdr.e_shstrndx;
    f.seekg(ehdr.e_shoff, std::ios::beg);
    for (uint16_t i = 0; i < ehdr.e_shnum; i++) {
        Elf_Shdr shdr;
        f.read((char *)&shdr, sizeof(shdr));
        if (shdr.sh_type == SHT_SYMTAB) {  // find .symtab
            symtabShdr = shdr;
            symtabFound = true;
        } else if (shdr.sh_type == SHT_STRTAB && i != shstrndx) { // find .strtab
            strtabShdr = shdr;
            strtabFound = true;
        }
    }
    
    if (symtabFound && strtabFound) {
        load_symbol_table(symtabShdr, strtabShdr, f);
    } else {
        std::cout << ".symtab or .strtab not found, skip loading symbol table" << std::endl;
    }

    f.close();
    
    return ehdr.e_entry;
}

std::optional<std::string> kdb::addr_match_symbol(word_t addr, word_t &offset) {
    if (kdb::symbolTable.empty()) {
        return std::nullopt;
    }

    auto iter = kdb::symbolTable.upper_bound(addr);
    if (iter == kdb::symbolTable.end()) {
        iter = iter--;
    } else if (iter->first != addr) {
        if (iter != kdb::symbolTable.begin()) {
            iter--;
        } else {
            return std::nullopt;
        }
    }

    if (addr - iter->first < 0x100) {
        offset = addr - iter->first;
        return iter->second;
    } else {
        return std::nullopt;
    }
}
