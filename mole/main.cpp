#include "main.h"

static thread_t g_thread;
static std::unordered_map<void*, AllocEvent> g_allocs;
static std::mutex g_allocs_mutex;

void print_leaks(const bool& verbose) {
    std::lock_guard lock(g_allocs_mutex);
    if (g_dropped != 0) warn("dropped {} malloc/free events", g_dropped.load());

    struct LeakInfo {
        size_t count = 0;
        size_t total_size = 0;
        DWORD thread_id = 0;
        ULONGLONG timestamp = 0;
        void* stack[MAX_FRAMES] = {0};
        USHORT frame_count = 0;
    };
    std::unordered_map<ULONG, LeakInfo> leak_groups;

    size_t total_leaked = 0, total_count = 0;
    for (const auto& ev : g_allocs | std::views::values) {
        if (should_ignore(ev)) continue;

        auto& group = leak_groups[ev.backtrace_hash];
        group.count++;
        group.total_size += ev.size;
        group.thread_id = ev.thread_id;
        group.timestamp = ev.timestamp;
        group.frame_count = ev.frame_count;
        std::copy_n(ev.stack, ev.frame_count, group.stack);
        total_leaked += ev.size;
        total_count++;
    }

    info("Total leaks: {} allocations, {} bytes", total_count, total_leaked);

    for (const auto& group : leak_groups | std::views::values) {
        std::string caller = (group.frame_count > 0) ? addr_to_symbol(group.stack[0]) : "<unknown>";
        warn(
            "Leak group: {}x, {}b, thread({}), caller={}", group.count, group.total_size,
            group.thread_id, caller
        );

        if (verbose && group.frame_count > 0) {
            std::stringstream stack_trace;
            for (USHORT i = 0; i < group.frame_count; ++i) {
                stack_trace << std::format(
                    "    #{} {}{}", i, addr_to_symbol(group.stack[i]),
                    i < group.frame_count - 1 ? "\n" : ""
                );
            }
            mole_print("stack trace:\n{}", stack_trace.str());
        }
    }
}

void _main(void* _) {
    set_console_style();
    info("loaded mole on thread {}", GetCurrentThreadId());

    string line;

    while (!g_unload) {
        size_t r = g_read_index.load(std::memory_order_relaxed);
        size_t w = g_write_index.load(std::memory_order_acquire);

        while (r < w) {
            auto& ev = g_events[r % EVENT_BUFFER_SIZE];

            std::lock_guard lock(g_allocs_mutex);
            if (ev.type == AllocEvent::Type::MALLOC) {
                g_allocs[ev.ptr] = ev;
            } else if (ev.type == AllocEvent::Type::FREE) {
                g_allocs.erase(ev.ptr);
            }

            ++r;
        }
        g_read_index.store(r, std::memory_order_relaxed);

        if (get_input(line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                if (const auto* cmd = find_command(line)) {
                    cmd->callback();
                } else {
                    info(
                        "'{}' is not a command, use 'help' or 'h' to see a list of available "
                        "commands",
                        line
                    );
                }
            }
        }
    }

    print_leaks();

    g_main_finished = true;
}

bool library_init() {
    init_logger();
    init_runtime_modules();

    SymSetOptions(
        SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_CASE_INSENSITIVE
    );
    if (!SymInitialize(
            GetCurrentProcess(), "SRV*C:\\sym*https://msdl.microsoft.com/download/symbols", TRUE
        )) {
        warn("SymInitialize failed: {}", GetLastError());
    }

    g_thread = create_thread(_main, nullptr);
    if (!g_thread.joinable()) {
        return false;
    }

    if (MH_Initialize() != MH_OK) return false;

    void* malloc_addr = (void*)GetProcAddress(GetModuleHandleA("ucrtbase.dll"), "malloc");
    void* malloc_addr_wcrt = (void*)GetProcAddress(GetModuleHandleA("wcrtbase.dll"), "malloc");
    void* free_addr = (void*)GetProcAddress(GetModuleHandleA("ucrtbase.dll"), "free");
    void* free_addr_wcrt = (void*)GetProcAddress(GetModuleHandleA("wcrtbase.dll"), "free");

    if (malloc_addr) {
        MH_CreateHook(malloc_addr, &mole_malloc, reinterpret_cast<LPVOID*>(&real_malloc));
        MH_EnableHook(malloc_addr);
    }
    if (malloc_addr_wcrt) {
        MH_CreateHook(malloc_addr_wcrt, &mole_malloc, reinterpret_cast<LPVOID*>(&real_malloc));
        MH_EnableHook(malloc_addr_wcrt);
    }
    if (free_addr) {
        MH_CreateHook(free_addr, &mole_free, reinterpret_cast<LPVOID*>(&real_free));
        MH_EnableHook(free_addr);
    }
    if (free_addr_wcrt) {
        MH_CreateHook(free_addr_wcrt, &mole_free, reinterpret_cast<LPVOID*>(&real_free));
        MH_EnableHook(free_addr_wcrt);
    }

    return true;
}

void library_shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);

    SymCleanup(GetCurrentProcess());

    if (g_thread.joinable()) {
        g_unload = true;
        int timeout = 0;
        while (!g_main_finished) {
            sleep_thread_ms(10);
            timeout += 10;
            if (timeout > 300) {
                g_main_finished = true;
                g_thread.detach();
            }
        }
        if (timeout < 300) g_thread.detach();
    }

    MH_Uninitialize();
    destroy_logger();
}

HMODULE g_mole_module = nullptr;
uintptr_t g_mole_base = 0;
size_t g_mole_size = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    MODULEINFO mi{};

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            g_mole_module = hModule;

            if (GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi))) {
                g_mole_base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
                g_mole_size = mi.SizeOfImage;
            }

            if (!library_init()) return FALSE;
            break;
        case DLL_PROCESS_DETACH:
            library_shutdown();
            break;
    }
    return TRUE;
}