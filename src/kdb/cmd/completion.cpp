#include "kdb/cmd.h"

#include <readline/readline.h>

static const char *allCommands[] = {
    "log off debug",
    "log on debug",
    NULL
};

static char *cmd_completion_generator(const char *text, int state) {
    static int index, len;
    const char *name;

    if (!state) {
        index = 0;
        len = strlen(text);
    }

    while ((name = allCommands[index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return nullptr;
};


static char **cmd_completion(const char *text, int start, int end) {
    return rl_completion_matches(text, cmd_completion_generator);
}

void cmd::init_completion() {
    rl_attempted_completion_function = cmd_completion;
    rl_completer_word_break_characters = ""; // Tell readline do not split words by space, but it disables the filename completion
}
