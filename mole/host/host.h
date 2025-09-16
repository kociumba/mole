#ifndef HOST_H
#define HOST_H

#include <common.h>

#include <cstdint>

using pid = uint32_t;

bool has_window(pid pid, HWND* out_handle);

#endif  // !HOST_H
