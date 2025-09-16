#ifndef ROOT_H
#define ROOT_H

#include <imgui.h>

#include <common.h>

#include "../commands/commands.h"
#include "../ipc/client.h"
#include "../main.h"

extern vector<string> output_log;
extern vector<string> command_history;
extern char input_buffer[256];
extern int history_index;
extern bool scroll_to_bottom;
extern vector<Command> commands;

void mole_ui_root();

#endif  // ROOT_H
