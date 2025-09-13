#include "unloader.h"

static module_handle_t get_current_module() {
    HMODULE hModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&get_current_module, &hModule
    );
    return reinterpret_cast<module_handle_t>(hModule);
}

bool unload(unload_func func) {
    auto unload_thread = [](LPVOID param) -> DWORD {
        auto* callback = static_cast<unload_func*>(param);

        if (callback) (*callback)();

        while (!g_main_finished) {
            sleep_thread_ms(10);
        }

        HMODULE module = reinterpret_cast<HMODULE>(get_current_module());
        if (!module) {
            delete callback;
            return 0;
        }

        delete callback;
        FreeLibraryAndExitThread(module, 0);
    };

    auto* callback_ptr = new unload_func(func);
    HANDLE thread = CreateThread(nullptr, 0, unload_thread, callback_ptr, 0, nullptr);
    if (!thread) {
        delete callback_ptr;
        return false;
    }

    CloseHandle(thread);
    return true;
}