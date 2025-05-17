#ifndef __KXEMU_CPU_RISCV_CSR_FIELD_H__
#define __KXEMU_CPU_RISCV_CSR_FIELD_H__


#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include <cstdint>

namespace kxemu::cpu::csr {

    class CSRField {
    protected:
        word_t value;
    public:
        CSRField() : value(0) {}
        CSRField(word_t value) : value(value) {}
        operator word_t() { return this->value; }
    };

    class PMPCfgItem {
    private:
        uint8_t value;
    public:
        PMPCfgItem() = delete;
        PMPCfgItem(uint8_t value) : value(value) {}

        enum A {
            OFF   = 0, // Null region (Disabled)
            TOR   = 1, // Top of Range
            NA4   = 2, // Naturally aligned 4-byte
            NAPOT = 3, // Naturally aligned power-of-two
        };

        bool r() const { return this->value & (1 << 0); }
        bool w() const { return this->value & (1 << 1); }
        bool x() const { return this->value & (1 << 2); }
        unsigned int a() const { return (this->value >> 3) & 0x3; }

        operator uint8_t() const { return this->value; }
    };

    class PMPCfg : public CSRField {
    public:
        using CSRField::CSRField;

        PMPCfgItem item(unsigned int index) const {
            return PMPCfgItem((this->value >> (index * 8)) & 0xff);
        }
    };

    class PMPAddr : public CSRField {
    public:
        PMPAddr(word_t value) {
        #ifdef KXEMU_ISA64
            this->value = value & ((1ULL << 54) - 1);
        #else
            this->value = value;
        #endif
        }
        
        word_t address() const { return this->value << 2; }
    };

    class MISA : public CSRField {
    public:
        enum MISAFlag {
            A = (1 <<  0), // Atomic Extension
            B = (1 <<  1), // B Extesnion
            C = (1 <<  2), // Compressed Extension
            D = (1 <<  3), // Double-precision Extension
            E = (1 <<  4), // RV32E/RV64E Base ISA
            F = (1 <<  5), // Single-precision Extension
            H = (1 <<  7), // Hypervisor Extension
            I = (1 <<  8), // RV32I/RV64I/RV128I Base ISA
            M = (1 << 12), // Integer Mutiply/Divide Extension
            Q = (1 << 16), // Quad-precision floating-point Extension
            S = (1 << 18), // Supervisor mode Implemented
            U = (1 << 20), // User mode Implemented
            V = (1 << 21), // Vector Extension
        };

        enum MXLEN {
            MXLEN32  = 1,
            MXLEN64  = 2,
            MXLEN128 = 3,
        };

        void set_mxlen(MXLEN mxlen) {
            #ifdef KXEMU_ISA64
            this->value = (this->value & ~(3ULL << 62)) | ((word_t)mxlen << 62);
            #else
            this->value = (this->value & ~(3ULL << 30)) | ((word_t)mxlen << 30);
            #endif
        }

        void set_flag(MISAFlag flag) {
            this->value |= flag;
        }

        MISA() {
            set_flag(MISAFlag::A);
            set_flag(MISAFlag::C);
            set_flag(MISAFlag::D);
            set_flag(MISAFlag::F);
            set_flag(MISAFlag::I);
            set_flag(MISAFlag::M);
            set_flag(MISAFlag::S);
            set_flag(MISAFlag::U);
            #ifdef KXEMU_ISA64
            set_mxlen(MXLEN::MXLEN64);
            #else
            set_mxlen(MXLEN::MXLEN32);
            #endif
        }
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

    class MIP : public CSRField {
    private:
        static constexpr word_t RW_MASK = (
            (1 << InterruptCode::TIMER_S   ) | 
            (1 << InterruptCode::SOFTWARE_S)
        );
        static constexpr word_t INTER_MASK_S = (
            (1 << InterruptCode::SOFTWARE_S) |
            (1 << InterruptCode::TIMER_S  )  |
            (1 << InterruptCode::EXTERNAL_S)
        );
    public:
        using CSRField::CSRField;

        void set  (InterruptCode code) { this->value |=  (1 << code); }
        void clear(InterruptCode code) { this->value &= ~(1 << code); }

        void set_mip(word_t value) { 
            // Bits in MIP register
            // NAME   | MIP
            // MEIP   | ReadOnly
            // MTIP   | ReadOnly
            // MSIP   | ReadOnly
            // STIP   | ReadWrite
            // SSIP   | ReadWrite
            // SEIP   | ReadOnly
            // LCOFIP | ReadOnly
            this->value = (this->value & ~RW_MASK) | (value & RW_MASK);
        }

        word_t sip() const { return this->value & INTER_MASK_S; }

        void set_sip(word_t value) {
            constexpr word_t mask = (1 << InterruptCode::SOFTWARE_S);
            this->value = (this->value & ~mask) | (value & mask);
        }
        void   set_pending(InterruptCode code) { this->value |=  (1 << code); }
        void clear_pending(InterruptCode code) { this->value &= ~(1 << code); }
    };

    class MIE : public CSRField {
    private:
        static constexpr word_t INTER_MASK = (
            (1 << InterruptCode::SOFTWARE_S) |
            (1 << InterruptCode::SOFTWARE_M) |
            (1 << InterruptCode::TIMER_S  )  |
            (1 << InterruptCode::TIMER_M  )  |
            (1 << InterruptCode::EXTERNAL_S) |
            (1 << InterruptCode::EXTERNAL_M) |
            (1 << InterruptCode::COUNTER   )
        );

        static constexpr word_t INTER_MASK_S = (
            (1 << InterruptCode::SOFTWARE_S) |
            (1 << InterruptCode::TIMER_S  )  |
            (1 << InterruptCode::EXTERNAL_S)
        );
    public:
        MIE(word_t value) { this->value = value & INTER_MASK; }

        word_t sie() const { return this->value & INTER_MASK_S; }

        void set_sie(word_t value) { this->value = (this->value & ~INTER_MASK_S) | (value & INTER_MASK_S); }
    };

    class TrapVec : public CSRField {
    public:
        using CSRField::CSRField;

        enum Mode {
            DIRECT   = 0,
            VECTORED = 1,
        };

        word_t vec()  { return this->value & ~0x3; }
        word_t mode() { return this->value &  0x1; }
    };

    class MCause : public CSRField {
    public:
        using CSRField::CSRField;

        MCause(InterruptCode code) {
            this->set_interrupt(code);
        }

        void set_interrupt(InterruptCode code) {
        #ifdef KXEMU_ISA64
            this->value = (1ULL << 63) | (word_t)code;
        #else
            this->value = (1 << 31) | (word_t)code;
        #endif
        }

        void set_exception(TrapCode code) {
            this->value = (word_t)code;
        }
    };

    class Satp : public CSRField {
    public:
        enum Mode {
            BARE = 0,
            SV32 = 1,
            SV39 = 8,
            SV48 = 9,
            SV57 = 10,
            SV64 = 11,
        };
    private:
        #ifdef KXEMU_ISA64
        static constexpr Mode validMode[] = {
            Mode::BARE,
            Mode::SV39,
            Mode::SV48,
            Mode::SV57
        };
        #endif
    public:
        Satp(word_t value) {
            this->value = value;
            #ifdef KXEMU_ISA64
            // Check if the mode is valid
            bool flag = true;
            for (unsigned int i = 0; i < sizeof(validMode) / sizeof(Mode); i++) {
                if (this->mode() == validMode[i]) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                // Set mode to Bare
                this->set_mode(Mode::BARE);
            }
            #endif
        }

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

        enum FRM {
            RNE = 0, // Round to Nearest, ties to Even
            RTZ = 1, // Round towards Zero
            RDN = 2, // Round Down
            RUP = 3, // Round Up
            RMM = 4, // Round to Min Magnitude
            DYN = 7, // Dynamic Rounding Mode
        };

        word_t fflags() { return this->value & 0x1f; }
        word_t frm()   { return (this->value >> 5) & 0x7; }

        void set_fflags(word_t fflags) { this->value = (this->value & ~0x1f) | (fflags & 0x1f);}
        void set_frm   (word_t frm   ) { this->value = (this->value & ~(0x7 << 5)) | ((frm & 0x7) << 5); }
    };
}

#endif
