#ifndef COMMANDS_H
#define COMMANDS_H

#include <common.h>

#include "../g_flags/flags.h"
#include "../logging/logger.h"
#include "../main.h"
#include "../unload/unloader.h"

struct Command {
    string desc;
    vector<string> aliases;
    function<void()> callback;
};

extern unordered_map<string, Command> commands;

const Command* find_command(const string& input);
void print_help();

#endif  // !COMMANDS_H
