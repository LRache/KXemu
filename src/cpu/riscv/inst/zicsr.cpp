#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"

#include "./local-decoder.h"

#define REQUIRE_WRITABLE do {if (IS_CSR_READ_ONLY(CSR)) {do_invalid_inst(); return;}} while(0);
#define CHECK_SUCCESS do{if (!s) {do_invalid_inst(); return;}} while(0);

using namespace kxemu::cpu;

void RVCore::do_csrrw(const DecodeInfo &decodeInfo) {    
    REQUIRE_WRITABLE;
    
    bool s;
    word_t value = 0;
    if (!rd_is_x0) {
        value = this->read_csr(CSR, s);
        CHECK_SUCCESS;
    }

    s = this->write_csr(CSR, SRC1);
    CHECK_SUCCESS;

    DEST = value;
}

void RVCore::do_csrrs(const DecodeInfo &decodeInfo) {
    bool s;
    word_t value = this->read_csr(CSR, s);
    CHECK_SUCCESS;

    if (!rs1_is_x0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(CSR, value | SRC1);
        CHECK_SUCCESS;
    }
    
    DEST = value;
}

void RVCore::do_csrrc(const DecodeInfo &decodeInfo) {
    bool s;
    
    word_t value = this->read_csr(CSR, s);
    CHECK_SUCCESS;
    
    if (!rs1_is_x0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(CSR, value & (~SRC1));
        CHECK_SUCCESS;
    }

    DEST = value;
}

void RVCore::do_csrrwi(const DecodeInfo &decodeInfo) {  
    REQUIRE_WRITABLE;
    
    bool s;
    if (!rd_is_x0) {
        word_t value = this->read_csr(CSR, s);
        CHECK_SUCCESS;
        DEST = value;
    }

    s = this->write_csr(CSR, IMM);
    CHECK_SUCCESS;
}

void RVCore::do_csrrsi(const DecodeInfo &decodeInfo) {
    bool s;
    word_t value = this->read_csr(CSR, s);
    CHECK_SUCCESS;

    if (IMM != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(CSR, value | IMM);
        CHECK_SUCCESS;
    }

    DEST = value;
}

void RVCore::do_csrrci(const DecodeInfo &decodeInfo) {
    bool s;
    word_t value = this->read_csr(CSR, s);
    CHECK_SUCCESS;

    if (IMM != 0) {
        REQUIRE_WRITABLE;
        s = this->write_csr(CSR, value & (~IMM));
        CHECK_SUCCESS;
    }

    DEST = value;
}
