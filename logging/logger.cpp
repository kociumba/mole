#include "logger.h"

bool init_logger() {
    if (!AllocConsole()) {
        return false;
    }

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(
                hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_INPUT |
                          ENABLE_LINE_INPUT | ENABLE_WINDOW_INPUT
            );
        }
    }

    std::ios::sync_with_stdio(true);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // console_sink->set_color_mode(spdlog::color_mode::always);
    auto logger = std::make_shared<spdlog::logger>("mole", console_sink);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    return true;
}

bool destroy_logger() {
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    if (!FreeConsole()) {
        return false;
    }

    spdlog::shutdown();

    return true;
}

void set_console_style() {
    if (auto handle = GetConsoleWindow()) {
        SetConsoleTitle("mole");

        auto curr_style = GetWindowLong(handle, GWL_STYLE);
        auto style = curr_style & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_POPUP;

        SetWindowLong(handle, GWL_STYLE, style);

        SetWindowPos(
            handle, nullptr, 0, 0, 1280, 720, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED
        );

        RedrawWindow(handle, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
    }
}

bool get_input(string& out) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD numEvents = 0;
    if (!GetNumberOfConsoleInputEvents(hIn, &numEvents) || numEvents == 0) {
        return false;
    }

    std::vector<INPUT_RECORD> buffer(numEvents);
    DWORD eventsRead = 0;
    if (!ReadConsoleInput(hIn, buffer.data(), numEvents, &eventsRead)) {
        return false;
    }

    static string line;

    for (DWORD i = 0; i < eventsRead; i++) {
        if (buffer[i].EventType == KEY_EVENT && buffer[i].Event.KeyEvent.bKeyDown) {
            char c = buffer[i].Event.KeyEvent.uChar.AsciiChar;
            WORD vk = buffer[i].Event.KeyEvent.wVirtualKeyCode;

            if (c == '\r' || c == '\n') {
                std::cout << std::endl;
                out = line;
                line.clear();
                return true;
            }

            if (vk == VK_BACK && !line.empty()) {
                line.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            } else if (isprint(c)) {
                line.push_back(c);
                std::cout << c;
                std::cout.flush();
            }
        }
    }

    return false;
}