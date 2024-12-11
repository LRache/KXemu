#include "kdb/cmd.h"
#include "kdb/kdb.h"

#include <bits/getopt_core.h>
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
                cmd::elfFileName = optarg;
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
    int r = kdb::run_cmd_mainloop();
    return r;
}