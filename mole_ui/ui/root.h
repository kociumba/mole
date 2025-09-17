#ifndef ROOT_H
#define ROOT_H

#include <imgui.h>

#include <common.h>

#include "../commands/commands.h"
#include "../ipc/client.h"
#include "../main.h"

void mole_ui_root();
void add_console_line(const string& line);

#endif  // ROOT_H
