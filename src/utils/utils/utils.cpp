#include "utils/utils.h"
#include <vector>

word_t utils::string_to_word(const std::string &s) {
    word_t result;
    if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        result = std::stoul(s, nullptr, 16);
    } else {
        result = std::stoul(s);
    }
    return result;
}

std::vector<std::string> utils::string_split(const std::string &s, const char delim) {
    std::stringstream ss(s);
    std::vector<std::string> result;
    std::string t;
    while (std::getline(ss, t, ' ')) {
        result.push_back(t);
    }
    return result;
}
