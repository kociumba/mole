#ifndef MEMORY_H
#define MEMORY_H

#include <common.h>

#include <MinHook.h>
#include <psapi.h>

#include <ranges>
#include <shared_mutex>
#include "../g_flags/flags.h"

constexpr size_t EVENT_BUFFER_SIZE = 1 << 16;
constexpr size_t MAX_FRAMES = 64;

typedef void*(__cdecl* malloc_t)(size_t);
typedef void(__cdecl* free_t)(void*);

extern malloc_t real_malloc;
extern free_t real_free;

struct AllocEvent {
    enum class Type { MALLOC, FREE } type;
    void* ptr;
    size_t size;
    USHORT frame_count;
    void* stack[MAX_FRAMES];
    DWORD thread_id = 0;
    ULONGLONG timestamp = 0;
    ULONG backtrace_hash = 0;
};

struct ModuleInfoCache {
    uintptr_t base;
    size_t size;
    string name;
};

ModuleInfoCache get_module_for_address(void* addr);
inline bool address_in_module(void* addr, const ModuleInfoCache& mi) {
    if (mi.base == 0) return false;
    uintptr_t p = reinterpret_cast<uintptr_t>(addr);
    return p >= mi.base && p < mi.base + mi.size;
}

extern unordered_map<uintptr_t, ModuleInfoCache> g_module_cache;
extern std::shared_mutex g_module_cache_mutex;

extern AllocEvent g_events[EVENT_BUFFER_SIZE];
extern atomic<size_t> g_write_index;
extern atomic<size_t> g_read_index;
extern atomic<size_t> g_dropped;

extern vector<std::pair<uintptr_t, size_t>> g_runtime_modules;

inline void push_event(AllocEvent ev) {
    if (g_write_index.load(std::memory_order_acquire) -
            g_read_index.load(std::memory_order_acquire) >=
        EVENT_BUFFER_SIZE) {
        ++g_dropped;
        return;
    }

    size_t idx = g_write_index.fetch_add(1, std::memory_order_relaxed);
    g_events[idx % EVENT_BUFFER_SIZE] = ev;
}

inline bool is_mole_address(void* addr) {
    uintptr_t p = reinterpret_cast<uintptr_t>(addr);
    return (p >= g_mole_base && p < g_mole_base + g_mole_size);
}

void init_runtime_modules();

void* __cdecl mole_malloc(size_t size);
void __cdecl mole_free(void* ptr);

#endif  // MEMORY_H
