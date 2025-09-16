#ifndef COMMANDS_H
#define COMMANDS_H

#include <common.h>

struct Command {
    string desc;
    vector<string> aliases;
};

vector<Command> parse_help_output(const string& help_text);

#endif  // COMMANDS_H
