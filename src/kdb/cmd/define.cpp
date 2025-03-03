#include "kdb/cmd.h"

#include <iostream>
#include <string>
#include <unordered_map>

using namespace kxemu::kdb;

std::unordered_map<std::string, std::string> cmd::defines;

void cmd::add_define(const std::string &define) {
    auto pos = define.find('=');
    if (pos == std::string::npos) {
        return;
    }
    auto key = define.substr(0, pos);
    auto value = define.substr(pos + 1);
    defines[key] = value;
}

void cmd::replace_define(args_t &args) {
    for (auto &arg : args) {
        if (arg.empty()) {
            continue;
        }

        if (arg[0] == '{' && arg[arg.size() - 1] == '}') {
            std::string vname = arg.substr(1, arg.size() - 2);
            auto it = defines.find(vname);
            if (it != defines.end()) {
                arg = it->second;
            } else {
                std::cerr << "warning: Name not found: " << arg << std::endl;
            }
        }
    }
}
