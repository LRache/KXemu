#include "kdb/kdb.h"

int main() {
    kdb::init();
    kdb::run_cpu();
    return 0;
}