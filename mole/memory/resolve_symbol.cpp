#include "resolve_symbol.h"

std::string addr_to_symbol(void* addr) {
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    HANDLE proc = GetCurrentProcess();

    if (SymFromAddr(proc, (DWORD64)addr, &displacement, sym)) {
        std::string sym_name = std::format("{}+0x{:x}", sym->Name, displacement);

        DWORD line_displacement = 0;
        IMAGEHLP_LINE64 line_info;
        line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(proc, (DWORD64)addr, &line_displacement, &line_info)) {
            return std::format("{} ({}:{})", sym_name, line_info.FileName, line_info.LineNumber);
        }

        return sym_name;
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi))) {
        char module_name[MAX_PATH];
        if (GetModuleFileNameA((HMODULE)mbi.AllocationBase, module_name, MAX_PATH)) {
            return std::format(
                "{}+0x{:x}", module_name, (uintptr_t)addr - (uintptr_t)mbi.AllocationBase
            );
        }
    }

    return std::format("{}", addr);
}
