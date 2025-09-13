#ifndef RESOLVE_SYMBOL_H
#define RESOLVE_SYMBOL_H

#include <common.h>

#include <dbghelp.h>

using std::string;

std::string addr_to_symbol(void* addr);

#endif  // RESOLVE_SYMBOL_H
