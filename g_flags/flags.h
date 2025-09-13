#ifndef FLAGS_H
#define FLAGS_H

#include <common.h>

extern atomic_bool g_main_finished;
extern atomic_bool g_unload;

extern HMODULE g_mole_module;
extern uintptr_t g_mole_base;
extern size_t g_mole_size;

extern atomic_bool g_filter_runtime_leaks;

#endif  // FLAGS_H
