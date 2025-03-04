#include "utils/utils.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace kxemu;

static uint64_t s_to_u(const std::string &s, bool &success) {
    uint64_t result = 0;
    for (auto &c : s) {
        if (c < '0' || c > '9') {
            success = false;
            return -1;
        }
        result = result * 10 + (c - '0');
    }
    success = true;
    return result;
}

static uint64_t s_to_u_hex(const std::string &s, bool &success) {
    uint64_t result = 0;
    for (auto &c : s) {
        if (c >= '0' && c <= '9') {
            result = result * 16 + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result = result * 16 + (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result = result * 16 + (c - 'A' + 10);
        } else {
            success = false;
            return -1;
        }
    }
    success = true;
    return result;
}

std::optional<uint64_t> utils::string_to_unsigned(const std::string &s) {
    bool success;
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        uint64_t r = s_to_u_hex(s.substr(2), success);
        return success ? std::optional<uint64_t>(r) : std::nullopt;
    } else {
        uint64_t r = s_to_u(s, success);
        return success ? std::optional<uint64_t>(r) : std::nullopt;
    }
}

uint64_t utils::string_to_unsigned(const std::string &s, bool &success) {
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        return s_to_u_hex(s.substr(2), success);
    } else {
        return s_to_u(s, success);
    }
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

uint64_t utils::timepoint_to_ns(const timepoint_t &tp) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

uint64_t utils::get_current_time() {
    return timepoint_to_ns(get_current_timepoint());
}
