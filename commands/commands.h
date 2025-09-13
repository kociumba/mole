#ifndef COMMANDS_H
#define COMMANDS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <format>
#include <functional>
#include <print>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../g_flags/flags.h"
#include "../logging/logger.h"
#include "../main.h"
#include "../unload/unloader.h"

using std::function;
using std::string;
using std::unordered_map;
using std::vector;

struct Command {
    string desc;
    vector<string> aliases;
    function<void()> callback;
};

extern unordered_map<string, Command> commands;

const Command* find_command(const string& input);
void print_help();

#endif  // !COMMANDS_H
