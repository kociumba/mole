#ifndef HOST_H
#define HOST_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>

using pid = uint32_t;

bool has_window(pid pid, HWND* out_handle);

#endif  // !HOST_H
