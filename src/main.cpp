#include "kdb/cmd.h"
#include "kdb/kdb.h"

#include <bits/getopt_core.h>
#include <getopt.h>
#include <vector>

void parse_args(int argc, char **argv) {
    static struct option options[] = {
        {"source", required_argument, 0, 's'},
        {"elf"   , required_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    std::vector<std::string> sourceFiles;

    int o;
    while((o = getopt_long(argc, argv, "s:", options, NULL)) != -1) {
        switch (o) {
            case 's':
                sourceFiles.push_back(optarg);
                break;
            case 'e':
                cmd::elfFileName = optarg;
                break;
            default:
                break;
        }
    }

    for (auto sourceFileName: sourceFiles) {
        kdb::run_source_file(sourceFileName);
    }
}

int main(int argc, char **argv) {
    kdb::init();
    kdb::cmd_init();
    parse_args(argc, argv);
    int r = kdb::run_cmd_mainloop();
    return r;
}