#ifndef __KXEMU_UTILS_UTILS_H__
#define __KXEMU_UTILS_UTILS_H__

#include <cstdint>
#include <string>
#include <vector>

namespace kxemu::utils {
    uint64_t string_to_unsigned(const std::string &s);
    uint64_t string_to_unsigned(const std::string &s, bool &success);
    std::vector<std::string> string_split(const std::string &s, const char delim);
}

#endif // __UTILS_UTILS_H__
