#include "kdb/kdb.h"

#include <getopt.h>

void parse_args(int argc, char **argv) {
    static struct option options[] = {
        {"source", required_argument, 0, 's'},
        {"elf"   , required_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    int o;
    while((o = getopt_long(argc, argv, "s:", options, NULL)) != -1) {
        switch (o) {
            case 's':
                kdb::run_source_file(optarg);
                break;
            case 'e':
                kdb::load_elf(optarg);
                break;
            default:
                break;
        }
    }
}

int main(int argc, char **argv) {
    kdb::init();
    kdb::cmd_init();
    parse_args(argc, argv);
    kdb::run_cmd_mainloop();
    return 0;
}