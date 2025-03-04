#ifndef __KXEMU_ISA_WORD_H__
#define __KXEMU_ISA_WORD_H__

// DO NOT include this file in the other header files

#include "config/config.h"
#include <cinttypes>
#include <iomanip>

#define FMT_WORD32 "0x%08"  PRIx32
#define FMT_WORD64 "0x%016" PRIx64
#define FMT_HEX32  "0x%" PRIx32
#define FMT_HEX64  "0x%" PRIx64
#define FMT_VARU32 "%" PRIu32
#define FMT_VARU64 "%" PRIu64

#ifdef KXEMU_ISA64
    #define FMT_WORD FMT_WORD64
    #define FMT_VARU FMT_VARU64
    #define WORD_BITS 64
    #define WORD_WIDTH 16
#else
    #define FMT_WORD FMT_WORD32
    #define FMT_VARU FMT_VARU32
    #define WORD_BITS 32
    #define WORD_WIDTH 8
#endif // KXEMU_ISA64

#define FMT_STREAM_WORD(word)  "0x" << std::hex << std::setfill('0') << word << std::dec
#define FMT_STREAM_WORD_SPACE(word)  std::setw(WORD_WIDTH + 2) << std::setfill(' ') << std::showbase << std::hex << word << std::dec << std::noshowbase

#endif