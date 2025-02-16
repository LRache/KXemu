#ifndef __KXEMU_UTILS_UTILS_H__
#define __KXEMU_UTILS_UTILS_H__

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace kxemu::utils {
    uint64_t string_to_unsigned(const std::string &s);
    uint64_t string_to_unsigned(const std::string &s, bool &success);
    std::vector<std::string> string_split(const std::string &s, const char delim);

    using timepoint_t = std::chrono::time_point<std::chrono::high_resolution_clock>;
    timepoint_t get_current_timepoint();
    uint64_t timepoint_to_ns(const timepoint_t &tp);
    uint64_t get_current_time();
}

#endif // __UTILS_UTILS_H__
