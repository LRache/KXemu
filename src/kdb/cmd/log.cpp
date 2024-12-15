#include "kdb/cmd.h"
#include "log.h"

#include <iostream>

static const struct {
    std::string name;
    int level;
} pairs[] = {
    {"DEBUG", DEBUG},
    {"debug", DEBUG},
    {"INFO",  INFO },
    {"info",  INFO },
    {"WARN",  WARN },
    {"warn",  WARN },
    {"PANIC", PANIC},
    {"panic", PANIC},
};

static const cmd::cmd_map_t cmdMap = {
    {"on" , cmd::log},
    {"off", cmd::log},
};

static int cmd_log_on(const std::string &logLevel) {
    if (logLevel.empty()) {
        logFlag = DEBUG | INFO | WARN | PANIC;
        return 0;
    }

    for (auto &pair : pairs) {
        if (pair.name == logLevel) {
            logFlag |= pair.level;
            std::cout << "Turn on the log level " << logLevel << "." << std::endl;
            return 0;
        }
    }
    std::cout << "Unkonwn log level: " << logLevel << std::endl;
    return 1;
}

static int cmd_log_off(const std::string &logLevel) {
    if (logLevel.empty()) {
        logFlag = 0;
        return 0;
    }
    
    for (auto &pair : pairs) {
        if (pair.name == logLevel) {
            logFlag &= ~pair.level;
            std::cout << "Turn off the log level " << logLevel << "." << std::endl;
            return 0;
        }
    }
    std::cout << "Unkonwn log level: " << logLevel << std::endl;
    return 1;
}
 
int cmd::log(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        std::cout << "Usage: log on|off [DEBUG|INFO|WARN|PANIC]" << std::endl;
        return 1;
    }
    
    std::string arg;
    if (args.size() < 3) {
        arg = "";
    } else {
        arg = args[2];
    }

    if (args[1] == "on") {
        return cmd_log_on(arg);
    } else if (args[1] == "off") {
        return cmd_log_off(arg);
    } else {
        std::cout << "Usage: log on|off [DEBUG|INFO|WARN|PANIC]" << std::endl;
        return CmdNotFound;
    }
}


