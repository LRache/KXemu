#ifndef __KXEMU_UTILS_UTILS_H__
#define __KXEMU_UTILS_UTILS_H__

#include "isa/word.h"

#include <string>
#include <vector>

namespace kxemu::utils {
    word_t string_to_word(const std::string &s);
    word_t string_to_word(const std::string &s, bool &success);
    std::vector<std::string> string_split(const std::string &s, const char delim);
}

#endif // __UTILS_UTILS_H__
