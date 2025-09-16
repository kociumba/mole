#ifndef LOGGER_H
#define LOGGER_H

#include <common.h>
#include <mutex>
#include <queue>

#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <iostream>

#include "../ipc/server.h"

enum class input_source_t { INPUT_NONE, INPUT_CONSOLE, INPUT_IPC };

template <typename... Args>
void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::trace, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::err, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void fatal(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::critical, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void mole_print(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::log(spdlog::level::off, fmt, std::forward<Args>(args)...);
}

bool init_logger();
bool destroy_logger();
void set_console_style();

bool get_input(string& out);

#endif  // LOGGER_H
