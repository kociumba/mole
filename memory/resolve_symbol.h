#ifndef RESOLVE_SYMBOL_H
#define RESOLVE_SYMBOL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dbghelp.h>

#include <format>
#include <string>

using std::string;

std::string addr_to_symbol(void* addr);

#endif  // RESOLVE_SYMBOL_H
