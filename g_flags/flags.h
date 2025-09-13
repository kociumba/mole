#ifndef FLAGS_H
#define FLAGS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>

extern std::atomic_bool g_main_finished;
extern std::atomic_bool g_unload;

extern HMODULE g_mole_module;
extern uintptr_t g_mole_base;
extern size_t g_mole_size;

extern std::atomic_bool g_filter_runtime_leaks;

#endif  // FLAGS_H
