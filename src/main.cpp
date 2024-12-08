#include "kdb/kdb.h"

int main() {
    kdb::init();
    kdb::cmd_init();
    kdb::run_cmd();
    return 0;
}