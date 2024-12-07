#include "isa/isa.h"
#include "kdb/kdb.h"

int main() {
    kdb::init(ISA::RISCV32);
    kdb::run_cpu();
    return 0;
}