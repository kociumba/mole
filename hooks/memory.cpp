#include "memory.h"

malloc_t real_malloc = nullptr;
free_t real_free = nullptr;

AllocEvent g_events[EVENT_BUFFER_SIZE];
atomic<size_t> g_write_index{0};
atomic<size_t> g_read_index{0};
atomic<size_t> g_dropped{0};

std::unordered_map<uintptr_t, ModuleInfoCache> g_module_cache;
std::shared_mutex g_module_cache_mutex;

// implemented from flags.h
std::atomic_bool g_filter_runtime_leaks = true;

vector<std::pair<uintptr_t, size_t>> g_runtime_modules;

void init_runtime_modules() {
    const std::vector<string> runtime_libs = {
        "ucrtbase.dll", "msvcrt.dll", "kernel32.dll", "ntdll.dll"
    };

    for (const auto& lib : runtime_libs) {
        HMODULE hModule = GetModuleHandleA(lib.c_str());
        if (hModule) {
            MODULEINFO mi{};
            if (GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi))) {
                g_runtime_modules.emplace_back(
                    reinterpret_cast<uintptr_t>(mi.lpBaseOfDll), mi.SizeOfImage
                );
            }
        }
    }
}

ModuleInfoCache get_module_for_address(void* addr) {
    uintptr_t p = reinterpret_cast<uintptr_t>(addr);

    std::shared_lock read_lock(g_module_cache_mutex);
    for (const auto& [base, mi] : g_module_cache) {
        if (p >= mi.base && p < mi.base + mi.size) return mi;
    }

    HMODULE hMod = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(addr), &hMod
        )) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) && mbi.AllocationBase) {
            hMod = reinterpret_cast<HMODULE>(mbi.AllocationBase);
        }
    }

    ModuleInfoCache mi{};
    if (hMod) {
        MODULEINFO modinfo{};
        if (GetModuleInformation(GetCurrentProcess(), hMod, &modinfo, sizeof(modinfo))) {
            mi.base = reinterpret_cast<uintptr_t>(modinfo.lpBaseOfDll);
            mi.size = modinfo.SizeOfImage;

            char buf[MAX_PATH] = {0};
            if (GetModuleFileNameA(hMod, buf, MAX_PATH)) mi.name = buf;
        }
    } else {
        mi.base = 0;
        mi.size = 0;
        mi.name = "<unknown>";
    }

    std::unique_lock write_lock(g_module_cache_mutex);
    if (mi.base != 0) g_module_cache[mi.base] = mi;

    return mi;
}

bool is_addr_runtime(void* addr) {
    if (!g_filter_runtime_leaks) return false;
    ModuleInfoCache mi = get_module_for_address(addr);
    if (mi.base == 0) return false;
    if (!mi.name.empty()) {
        std::string lower;
        lower.resize(mi.name.size());
        std::transform(mi.name.begin(), mi.name.end(), lower.begin(), ::tolower);

        if (lower.find("ucrtbase.dll") != std::string::npos) return true;
        if (lower.find("msvcrt.dll") != std::string::npos) return true;
        if (lower.find("kernel32.dll") != std::string::npos) return true;
        if (lower.find("ntdll.dll") != std::string::npos) return true;
    }

    uintptr_t p = reinterpret_cast<uintptr_t>(addr);
    for (const auto& [base, size] : g_runtime_modules) {
        if (p >= base && p < base + size) return true;
    }

    return false;
}

bool should_ignore(const AllocEvent& ev) {
    if (ev.frame_count == 0) return false;

    int last_runtime_idx = -1;
    for (USHORT i = 0; i < ev.frame_count; ++i) {
        if (is_mole_address(ev.stack[i])) {
            return true;
        }
        if (is_addr_runtime(ev.stack[i])) {
            last_runtime_idx = static_cast<int>(i);
        } else {
            continue;
        }
    }

    if (last_runtime_idx == -1) {
        return false;
    }

    if (last_runtime_idx == static_cast<int>(ev.frame_count) - 1) {
        return false;
    }

    void* candidate = ev.stack[last_runtime_idx + 1];
    if (is_mole_address(candidate)) return true;
    if (is_addr_runtime(candidate)) return true;

    return false;
}

void* __cdecl mole_malloc(size_t size) {
    void* p = real_malloc(size);
    if (p) {
        AllocEvent ev{};
        ev.type = AllocEvent::Type::MALLOC;
        ev.ptr = p;
        ev.size = size;
        ev.thread_id = GetCurrentThreadId();
        ev.timestamp = GetTickCount64();
        ev.frame_count = RtlCaptureStackBackTrace(2, MAX_FRAMES, ev.stack, &ev.backtrace_hash);

        if (!should_ignore(ev)) {
            push_event(ev);
        }
    }

    return p;
}

void __cdecl mole_free(void* ptr) {
    if (ptr) {
        AllocEvent ev{};
        ev.type = AllocEvent::Type::FREE;
        ev.ptr = ptr;
        ev.thread_id = GetCurrentThreadId();
        ev.timestamp = GetTickCount64();
        ev.frame_count = RtlCaptureStackBackTrace(2, MAX_FRAMES, ev.stack, &ev.backtrace_hash);

        if (!should_ignore(ev)) {
            push_event(ev);
        }
    }

    real_free(ptr);
}
