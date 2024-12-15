#include "kdb/cmd.h"

#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <readline/readline.h>
#include <vector>

struct Node{
    Node(const std::unordered_map<std::string, std::vector<Node>> &children) : children(children) , func(nullptr){};
    Node(const cmd::completion_func_t &func) : func(func) {}
    Node(const std::string name) : name(name) {}
    Node() = default;
    
    std::unordered_map<std::string, std::vector<Node>> children;
    cmd::completion_func_t func;
    std::string name; // end
};

static const Node node = Node{{{"1", {}}}};

static const
std::unordered_map<std::string, Node> tree = {
    {
        "log", {
            {
                {"off", {"DEBUG", "INFO"}},
                {"on",  {"DEBUG", "INFO"}}
            }
        }
    }
};

static char *cmd_completion_generator(const char *text, int state) {
    return nullptr;
};


static char **cmd_completion(const char *text, int start, int end) {
    std::cout << rl_line_buffer << std::endl;
    return rl_completion_matches(text, cmd_completion_generator);
}

void cmd::init_completion() {
    rl_attempted_completion_function = cmd_completion;
    // char s[] = " ";
    // rl_completer_word_break_characters = s; // Tell readline do not split words by space, but it disables the filename completion
}
