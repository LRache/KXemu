#ifndef __KXEMU_ISA_WORD_H__
#define __KXEMU_ISA_WORD_H__

#include <iomanip>
#include <cinttypes>

#define FMT_WORD_64 "0x%016" PRIx64
#define FMT_WORD_32 "0x%08" PRIx32
#define FMT_VARU_32 "%" PRIu32
#define FMT_VARU_64 "%" PRIu64

#ifdef ISA_64
    #define FMT_WORD FMT_WORD_64
    #define FMT_VARU FMT_VARU_64
    #define WORD_SIZE 64
    #define WORD_WIDTH 16
#else
    #define FMT_WORD FMT_WORD_32
    #define FMT_VARU FMT_VARU_32
    #define WORD_SIZE 32
    #define WORD_WIDTH 8
#endif // ISA_64

#define FMT_STREAM_WORD(word)  "0x" << std::hex << std::setw(WORD_WIDTH) << std::setfill('0') << word << std::dec

#endif
