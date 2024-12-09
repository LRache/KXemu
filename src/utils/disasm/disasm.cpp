#include "utils/disasm.h"
#include "common.h"
#include "log.h"

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include <cstddef>
#include <cstdint>

using namespace llvm;

static MCDisassembler *disassembler;
static MCInstPrinter *gIP = nullptr;
static MCSubtargetInfo *gSTI = nullptr;

void disasm::init(std::string isaName) {
    InitializeAllTargetInfos();
    InitializeAllTargetMCs();
    InitializeAllDisassemblers();

    std::string targetTriple = isaName + "-unknown-elf";
    std::string error;
    const Target *target = TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        PANIC("Failed to lookup target: %s", error.c_str());
    }

    MCTargetOptions MCOptions;
    gSTI = target->createMCSubtargetInfo(targetTriple, "", "");
    auto gMII = target->createMCInstrInfo();
    auto gMRI = target->createMCRegInfo(targetTriple);
    auto asmInfo = target->createMCAsmInfo(*gMRI, targetTriple, MCOptions);
    auto llvmTripleTwine = Twine(targetTriple);
    auto llvmtriple = llvm::Triple(llvmTripleTwine);
    auto ctx = new llvm::MCContext(llvmtriple, asmInfo, gMRI, nullptr);

    disassembler = target->createMCDisassembler(*gSTI, *ctx);
    gIP = target->createMCInstPrinter(llvm::Triple(targetTriple),
        asmInfo->getAssemblerDialect(), *asmInfo, *gMII, *gMRI);
    gIP->setPrintImmHex(true);
    gIP->setPrintBranchImmAsAddress(true);
    gIP->applyTargetSpecificCLOption("no-aliases");
}

std::string disasm::disassemble(const uint8_t *code, const size_t length, word_t pc, unsigned int &instLen) {
    MCInst inst;
    ArrayRef<uint8_t> arr(code, length);
    uint64_t dummy_size = 0;
    disassembler->getInstruction(inst, dummy_size, arr, pc, nulls());

    std::string s;
    raw_string_ostream os(s);
    gIP->printInst(&inst, pc, "", *gSTI, os);
    
    instLen = dummy_size;
    std::replace(s.begin(), s.end(), '\t', ' ');
    return s;
}
