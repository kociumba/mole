#ifndef UNLOADER_H
#define UNLOADER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../g_flags/flags.h"
#include "../threads/threads.h"

using unload_func = void (*)();
using module_handle_t = uintptr_t;

bool unload(unload_func func = nullptr);

#endif  // UNLOADER_H
