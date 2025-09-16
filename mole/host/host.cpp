#include "host.h"

struct FindWindowParams {
    DWORD pid;
    HWND hwnd;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* params = (FindWindowParams*)lParam;

    DWORD window_pid;
    GetWindowThreadProcessId(hwnd, &window_pid);

    if (window_pid == params->pid) {
        params->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

bool has_window(pid pid, HWND* out_handle) {
    FindWindowParams params = {pid, nullptr};
    EnumWindows(EnumWindowsProc, (LPARAM)&params);
    *out_handle = params.hwnd;
    return params.hwnd != nullptr;
}