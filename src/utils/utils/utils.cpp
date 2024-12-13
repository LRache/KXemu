#include "utils/utils.h"

word_t utils::string_to_word(const std::string &s) {
    word_t result;
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        result = std::stoul(s, nullptr, 16);
    } else {
        result = std::stoul(s);
    }
    return result;
}
