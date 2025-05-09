#ifndef __KXEMU_CPU_RISCV_CSR_FIELD_H__
#define __KXEMU_CPU_RISCV_CSR_FIELD_H__


#include "cpu/word.h"
#include <cstdint>

namespace kxemu::cpu::csr {

    class CSRField {
    protected:
        word_t value;
    public:
        CSRField() = delete;
        CSRField(word_t value) : value(value) {}
        operator word_t() { return this->value; }
    };

    class MStatus : public CSRField {
    public:
        using CSRField::CSRField;
        
        bool   sie()  const { return this->value & (1 << 1); }
        bool   mie()  const { return this->value & (1 << 3); }
        bool   spie() const { return this->value & (1 << 5); }
        bool   mpie() const { return this->value & (1 << 7); }
        word_t spp()  const { return (this->value >>  8) & 0x1; }
        word_t mpp()  const { return (this->value >> 11) & 0x3; }
        bool   sum()  const { return this->value & (1 << 18); }

        void set_sie (bool sie ) { this->value = (this->value & ~ (1 << 1)) | (sie << 1); }
        void set_mie (bool mie ) { this->value = (this->value & ~ (1 << 3)) | (mie << 3); }
        void set_spie(bool spie) { this->value = (this->value & ~ (1 << 5)) | (spie << 5); }
        void set_mpie(bool mpie) { this->value = (this->value & ~ (1 << 7)) | (mpie << 7); }
        void set_spp (word_t spp) { this->value = (this->value & ~ (1 <<  8)) | (spp <<  8); }
        void set_mpp (word_t mpp) { this->value = (this->value & ~ (3 << 11)) | (mpp << 11); }
        void set_mprv(word_t mprv) { this->value = (this->value & ~ (1 << 17)) | (mprv << 17);}

        static constexpr word_t SSTATUS_MASK = (
            (1ULL <<  1) | // SIE
            (1ULL <<  5) | // SPIE
            (1ULL <<  8) | // SPP
            (3ULL << 13) | // FS
            (3ULL << 15) | // XS
            (1ULL << 18) | // SUM
            (1ULL << 19) | // MXR
            (1ULL << 23) | // SPELP
            (1ULL << 24) | // SDT
        #ifdef KXEMU_ISA64
            (3ULL << 32) | // UXL
            (1ULL << 63)   // SD
        #else
            (1ULL << 31)   // SD
        #endif
        );

        word_t sstatus() { return this->value & SSTATUS_MASK; }
        void set_sstatus(word_t sstatus) {
            this->value &= ~SSTATUS_MASK;
            this->value |= sstatus & SSTATUS_MASK;
        }
    };

    class MCounteren : public CSRField {
    public:
        using CSRField::CSRField;

        bool cy() const { return this->value & (1 << 0); }
        bool tm() const { return this->value & (1 << 1); }
        bool ir() const { return this->value & (1 << 2); }
    };

    class MEnvConfig : public CSRField {
    public:
        using CSRField::CSRField;

        #ifdef KXEMU_ISA64
        bool stce() const { return this->value & (1ULL << 63); }
        #endif
    };

    #ifdef KXEMU_ISA32
    class MEnvConfigH : public CSRField {
    public:
        using CSRField::CSRField;

        bool stce() const { return this->value & (1 << 31); }
    };
    #endif

    class PMPCfgItem {
    private:
        uint8_t value;
    public:
        PMPCfgItem() = delete;
        PMPCfgItem(uint8_t value) : value(value) {}

        bool r() const { return this->value & (1 << 0); }
        bool w() const { return this->value & (1 << 1); }
        bool x() const { return this->value & (1 << 2); }
        unsigned int a() const { return (this->value >> 3) & 0x3; }
    };

    class TrapVec : public CSRField {
    public:
        using CSRField::CSRField;

        word_t vec() { return this->value & ~0x3; }
        word_t mode() { return this->value & 0x1; }
    };
    

    class Satp : public CSRField {
    public:
        using CSRField::CSRField;

        #ifdef KXEMU_ISA64
        word_t ppn()  { return this->value & ((1ULL << 44) - 1); }
        word_t asid() { return (this->value >> 44) & 0x1ff; }
        word_t mode() { return (this->value >> 60) & 0xf; }

        void set_asid(word_t asid) { this->value = (this->value & ~(0x1ffULL << 44)) | ((asid & 0x1ff) << 44); }
        void set_mode(word_t mode) { this->value = (this->value & ~(0xfULL << 60)) | ((mode & 0xf) << 60); }
        #else
        word_t ppn()  { return this->value & ((1ULL << 22) - 1); }
        word_t asid() { return (this->value >> 22) & 0x1ff; }
        word_t mode() { return (this->value >> 31) & 0x1; }
        
        void set_asid(word_t asid) { this->value = (this->value & ~(0x1ff << 22)) | ((asid & 0x1ff) << 22); }
        void set_mode(word_t mode) { this->value = (this->value & ~0x80000000) | ((mode & 0x1) << 31); }
        #endif
    };

    class FCSR : public CSRField {
    public:
        using CSRField::CSRField;

        word_t fflags() { return this->value & 0x1f; }
        word_t frm()   { return (this->value >> 5) & 0x7; }

        void set_fflags(word_t fflags) { this->value = (this->value & ~0x1f) | (fflags & 0x1f);}
        void set_frm   (word_t frm   ) { this->value = (this->value & ~(0x7 << 5)) | ((frm & 0x7) << 5); }
    };
}

#endif
