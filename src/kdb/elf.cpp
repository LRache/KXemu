#include "isa/isa.h"
#include "kdb/kdb.h"
#include "config/config.h"
#include "word.h"

#include <cerrno>
#include <cstddef>
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

#ifdef KXEMU_ISA64
    using Elf_Ehdr = Elf64_Ehdr;
    using Elf_Phdr = Elf64_Phdr;
    using Elf_Shdr = Elf64_Shdr;
    using Elf_Sym  = Elf64_Sym;
    using Elf_Word = Elf64_Word;
    using Elf_Addr = Elf64_Addr;
    using Elf_Off  = Elf64_Off;
    static const int EXPECTED_CLASS = ELFCLASS64;
#else
    using Elf_Ehdr = Elf32_Ehdr;
    using Elf_Phdr = Elf32_Phdr;
    using Elf_Shdr = Elf32_Shdr;
    using Elf_Sym  = Elf32_Sym;
    using Elf_Word = Elf32_Word;
    using Elf_Addr = Elf32_Addr;
    using Elf_Off  = Elf32_Off;
    static const int EXPECTED_CLASS = ELFCLASS32;
#endif

using namespace kxemu;

std::map<kdb::word_t, std::string> kdb::symbolTable;

static bool check_read_success(std::fstream &f, long expectedSize) {
    if (f.gcount() != expectedSize) {
        std::cerr << "An error occurred when read from file." << std::endl;
        f.close();
        return false;
    }
    return true;
}

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
    if (ehdr.e_machine != isa::get_elf_expected_machine()) {
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
    
    // clear memory
    if (!kdb::bus->memset(start, memsze, 0)) {
        std::cerr << "Faliled to clear memory, start=" <<  FMT_STREAM_WORD(start) << ", memsize=" << FMT_STREAM_WORD(memsze) << std::endl;
        return false; 
    }
    
    // copy ELF file to memory
    if (!kdb::bus->load_from_stream(f, start, filesz)) {
        std::cerr << "Failed to load ELF file to memory, start=" <<  FMT_STREAM_WORD(start) << ", filesize=" << FMT_STREAM_WORD(filesz) << std::endl;
        return false; // clear memory
    }
    
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
        if (!check_read_success(f, sizeof(sym))) {
            return false;
        }
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

// For ELF file format, see https://www.man7.org/linux/man-pages/man5/elf.5.html
// Load elf file from local disk to memory and build symbol table for debug.
std::optional<kdb::word_t> kdb::load_elf(const std::string &filename) {
    std::fstream f;
    f.open(filename, std::ios_base::in);
    if (!f.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return std::nullopt;
    }

    Elf_Ehdr ehdr;
    f.read((char *)&ehdr, sizeof(ehdr));
    if (!check_read_success(f, sizeof(ehdr))) {
        std::cout << "An error occurred when read elf header." << std::endl;
        return std::nullopt;
    }

    if (!check_is_valid_elf(ehdr)) {
        std::cout << "BAD elf header" << std::endl;
        return std::nullopt;
    }

    // read program header
    std::vector<Elf_Phdr> phdrArray;
    f.seekg(ehdr.e_phoff, std::ios::beg);
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        Elf_Phdr phdr;
        f.read((char *)&phdr, sizeof(phdr));
        if (!check_read_success(f, sizeof(phdr))) {
            std::cout << "An error occurred when read program header." << std::endl;
            return std::nullopt;
        }
        phdrArray.push_back(phdr);
    }

    for (auto phdr: phdrArray) {
        if (!load_program(phdr, f)) {
            std::cout << "An error occurred when load program header." << std::endl;
            return std::nullopt;
        }
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

bool kdb::load_symbol(const std::string &filename) {
    std::fstream f;
    f.open(filename, std::ios_base::in);
    if (!f.is_open()) {
        std::cerr << "Failed to open file \"" << filename << "\": " << std::strerror(errno) << std::endl;
        return false;
    }

    Elf_Ehdr ehdr;
    f.read((char *)&ehdr, sizeof(ehdr));
    if (!check_read_success(f, sizeof(ehdr))) {
        std::cerr << "Failed to read file \"" << filename << "\": " << std::strerror(errno) << std::endl;
        return false;
    }

    if (!check_is_valid_elf(ehdr)) {
        std::cerr << "BAD elf header" << std::endl;
        return false;
    }

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
        size_t before = kdb::symbolTable.size();
        load_symbol_table(symtabShdr, strtabShdr, f);
        size_t after = kdb::symbolTable.size();
        std::cout << "Loaded " << (after - before) << " symbols from \"" << filename << "\"" << std::endl;
    } else {
        std::cerr << "No symbol table was founded in \"" << filename << "\"" << std::endl;
    }

    return true;
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

    offset = addr - iter->first;
    return iter->second;
}
