#include "utils/utils.h"
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace kxemu;

uint64_t utils::string_to_unsigned(const std::string &s) {
    uint64_t result;
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        result = std::stoul(s, nullptr, 16);
    } else {
        result = std::stoul(s);
    }
    return result;
}

uint64_t utils::string_to_unsigned(const std::string &s, bool &success) {
    uint64_t result;
    try {
        if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        result = std::stoul(s, nullptr, 16);
        } else {
            result = std::stoul(s);
        }
    } catch (std::exception &) {
        success = false;
        return 0;
    };
    success = true;
    return result;
}

std::vector<std::string> utils::string_split(const std::string &s, const char delim) {
    std::stringstream ss(s);
    std::vector<std::string> result;
    std::string t;

    while (std::getline(ss, t, delim)) {
        if (!t.empty() && t.find_first_not_of(' ') != std::string::npos) {
            t.erase(t.begin(), std::find_if(t.begin(), t.end(), [](unsigned char c) { return !std::isspace(c); }));
            t.erase(std::find_if(t.rbegin(), t.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), t.end());
            result.push_back(t);
        }
    }
    return result;
}

utils::timepoint_t utils::get_current_timepoint() {
    return std::chrono::high_resolution_clock::now();
}
