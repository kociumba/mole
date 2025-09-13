#ifndef MOLE_H
#define MOLE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <MinHook.h>
#include <dbghelp.h>
#include <psapi.h>

#include <atomic>
#include <mutex>
#include <print>
#include <ranges>
#include <sstream>
#include <unordered_map>

#include "commands/commands.h"
#include "g_flags/flags.h"
#include "hooks/memory.h"
#include "host/host.h"
#include "logging/logger.h"
#include "memory/resolve_symbol.h"
#include "threads/threads.h"
#include "unload/unloader.h"

void print_leaks(const bool& verbose = false);

#endif  // MOLE_H
