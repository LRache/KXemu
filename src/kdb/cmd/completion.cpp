#include "kdb/cmd.h"
#include "utils/utils.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <readline/readline.h>
#include <vector>

struct Node{
    explicit Node(const std::unordered_map<std::string, std::vector<Node>> &children) : children(children) , func(nullptr){};
    explicit Node(const cmd::completion_func_t &func) : func(func) {}
    explicit Node(const std::string &name) {
        children.insert({name, {}});
    }
    Node() = default;
    
    std::unordered_map<std::string, std::vector<Node>> children;
    cmd::completion_func_t func;
};

static const Node tree = Node({
    {"log", {Node({{"off", {Node("DEBUG"), Node("WARN")}}, {"on", {Node("DEBUG"), Node("WARN")}}})}},
});

static char **cmd_completion(const char *text, int start, int end) {
    const cmd::args_t args = utils::string_split(rl_line_buffer, ' ');
    const std::size_t argsLength = args.size();
    if (argsLength == 0) return NULL;

    std::size_t index = 0;
    const Node *node = &tree;
    while (index < argsLength - 1) {
        auto iter = node->children.find(args[index]);
        if (iter == node->children.end()) {
            break;
        }
        auto item = std::find(iter->second.begin(), iter->second.end(), args[index]);
        if (item == iter->second.end()) {
            break;
        }
        node = &(*item);
        index ++;
    }
    if (index != argsLength - 1) return NULL; // not matched
    
    // std::string last = args[argsLength - 1];
    // std::vector<std::string> possible;
    // for (auto x : node->children) {
    //     if (x.first.substr(0, last.size()) == last) {
    //         possible.push_back(x.first);
    //     }
    // }
    // char **r = (char **)malloc(sizeof(char *) * possible.size());
    // for (std::size_t i = 0; i < possible.size(); i++) {
    //     r[i] = strdup(possible[i].c_str());
    // }

    // return r;
    return NULL;
}

void cmd::init_completion() {
    rl_attempted_completion_function = cmd_completion;
}
