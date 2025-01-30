#include "kdb/cmd.h"
#include "utils/utils.h"

#include <string>
#include <unordered_map>
#include <readline/readline.h>
#include <vector>

using namespace kxemu;
using namespace kxemu::kdb;

struct Node {
    explicit Node(const std::unordered_map<std::string, Node*> &children) : children(children) , func(nullptr){};
    explicit Node(const cmd::completion_func_t &func) : func(func) {}
    explicit Node(const std::string &name) {
        children.insert({name, {}});
    }
    Node() = default;
    
    std::unordered_map<std::string, Node*> children;
    cmd::completion_func_t func;
};

static const Node *tree = new Node({
    {
        "log", 
        new Node({
            {"on",  new Node({{"DEBUG", nullptr}, {"INFO", nullptr}, {"WARN", nullptr}, {"PANIC", nullptr}})},
            {"off", new Node({{"DEBUG", nullptr}, {"INFO", nullptr}, {"WARN", nullptr}, {"PANIC", nullptr}})}
        })
    },
    {"info", new Node({
        {"gpr", nullptr}
    })},
    {"load", new Node({
        {"elf", nullptr}
    })},
    {"mem", new Node({
        {"create", nullptr},
        {"img", nullptr},
        {"elf", nullptr},
        {"map", nullptr},
        {"save", nullptr}
    })},
    {"step", nullptr},
    {"exit", nullptr},
    {"run" , nullptr},
    {"symbol", nullptr},
    {"breakpoint", nullptr},
    {"reset", nullptr},
    {"uart", new Node({
        {"add", nullptr}
    })}
});

static char *command_completion_generator(const char *, int state) {
    static std::vector<std::string> matches;
    static std::size_t current = 0;
    
    if (state == 0) {
        const std::string line = rl_line_buffer;
        const cmd::args_t args = utils::string_split(rl_line_buffer, ' ');
        const std::size_t argsLength = args.size();

        if (argsLength == 0) return NULL;

        const Node *node = tree;
        std::size_t index = 0;
        while (index < argsLength - 1) {
            auto iter = node->children.find(args[index]);
            if (iter == node->children.end()) {
                return NULL; // Unknown command
            }
            if (iter->second == nullptr) {
                return NULL; // End of a command
            }
            node = iter->second;
            index ++;
        }
        // Init matches
        current = 0;
        matches.clear();

        if (line.back() == ' ') {
            auto iter = node->children.find(args[argsLength - 1]);
            if (iter == node->children.end()) {
                return NULL; // Unknown command
            }
            node = iter->second;
            if (node == nullptr) return NULL; // End of a command
            
            for (const auto &pair : node->children) {
                matches.push_back(pair.first);
            }
            
        } else {
            std::string prefix = args[argsLength - 1];
            for (const auto &pair : node->children) {
                if (pair.first.rfind(prefix, 0) == 0 && pair.first != prefix) {
                    matches.push_back(pair.first);
                }
            }
        }
    }

    if (current < matches.size()) {
        return strdup(matches[current++].c_str());
    } else {
        return NULL;
    }
}

static char **cmd_completion(const char *text, int, int) {
    return rl_completion_matches(text, command_completion_generator);
}

void cmd::init_completion() {
    rl_attempted_completion_function = cmd_completion;
}
