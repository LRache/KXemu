#include "cpu/riscv/core.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/word.hpp"
#include "log.h"

#include "./local-decoder.h"

using namespace kxemu::cpu;

void RVCore::execute_csr(const DecodeInfo &decodeInfo, word_t operand, RVCSR::ModifyOp op, bool readOld) {
    const auto result = this->modify_csr(CSR, operand, op, readOld);
    if (!result.valid) {
        // Guests commonly probe CSRs, so this is a debug trace rather than a warning.
        DEBUG("Illegal access to CSR 0x%03x (op %d) at pc " FMT_WORD, CSR, static_cast<int>(op), this->pc);
        do_invalid_inst();
        return;
    }
    if (!rd_is_x0) DEST = result.oldValue;
}

void RVCore::do_csrrw(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, SRC1, RVCSR::ModifyOp::Write, !rd_is_x0);
}

void RVCore::do_csrrs(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, SRC1, rs1_is_x0 ? RVCSR::ModifyOp::Read : RVCSR::ModifyOp::Set);
}

void RVCore::do_csrrc(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, SRC1, rs1_is_x0 ? RVCSR::ModifyOp::Read : RVCSR::ModifyOp::Clear);
}

void RVCore::do_csrrwi(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, IMM, RVCSR::ModifyOp::Write, !rd_is_x0);
}

void RVCore::do_csrrsi(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, IMM, IMM == 0 ? RVCSR::ModifyOp::Read : RVCSR::ModifyOp::Set);
}

void RVCore::do_csrrci(const DecodeInfo &decodeInfo) {
    this->execute_csr(decodeInfo, IMM, IMM == 0 ? RVCSR::ModifyOp::Read : RVCSR::ModifyOp::Clear);
}
