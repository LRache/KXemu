/*
 * main.cpp
 * Copyright (C) 2024 Zhenrui Liu, Hangzhou Dianzi University
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "autogen/git_info.hpp"
#include "kdb/cmd.hpp"
#include "kdb/kdb.hpp"

#include <getopt.h>
#include <iostream>
#include <vector>

using namespace kxemu;

static std::vector<std::string> sourceFiles;
static unsigned int coreCount = 1;

static void parse_args(int argc, char **argv) {
    static struct option options[] = {
        {"source", required_argument, 0, 's'},
        {"def"   , required_argument, 0, 'd'},
        {"cores" , required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    int o;
    while ((o = getopt_long(argc, argv, "s:", options, NULL)) != -1) {
        switch (o) {
        case 's':
            sourceFiles.push_back(optarg);
            break;
        case 'd':
            kdb::cmd::add_define(optarg);
            break;
        case 'c':
            coreCount = std::stoi(optarg);
            break;
        default:
            break;
        }
    }
}

static void output_info() {
    std::cout << "KXemu build at " << __TIMESTAMP__;
#ifdef __clang__
    std::cout << " (Clang " << __clang_major__ << "." << __clang_minor__ << "."
              << __clang_patchlevel__ << ")" << std::endl;
#elif defined(__GNUC__)
    std::cout << " (GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "."
              << __GNUC_PATCHLEVEL__ << ")" << std::endl;
#elif defined(_MSC_VER)
    std::cout << " (MSVC " << _MSC_VER << ")" << std::endl;
#endif

    std::cout << "Git: " << kxemu::git::get_version_string() << std::endl;
    std::cout << "Commit: " << kxemu::git::commit_hash << std::endl;
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    output_info();

    kdb::init(coreCount);
    for (auto sourceFileName : sourceFiles) {
        kdb::cmd::run_source_file(sourceFileName);
    }

    kdb::cmd::init();
    int r = kdb::cmd::mainloop();
    kdb::deinit();

    return r;
}