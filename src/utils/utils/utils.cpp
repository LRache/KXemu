#include "utils/utils.h"
#include <string>
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

word_t utils::string_to_word(const std::string &s, bool &success) {
    word_t result;
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
    while (std::getline(ss, t, ' ')) {
        if (t.empty() || t.find_first_not_of(' ') == std::string::npos) continue;
        result.push_back(t);
    }
    return result;
}
