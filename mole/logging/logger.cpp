#include "logger.h"

struct ipc_sink : spdlog::sinks::sink {
    void log(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t buf;
        buf = std::format("[{}] {}", spdlog::level::to_string_view(msg.level), msg.payload);
        string s(buf.data(), buf.size());
        s.push_back('\n');
        ipc_broadcast(&g_ipc, s);
    }
    void flush() override {}
    void set_pattern(const std::string&) override {}
    void set_formatter(std::unique_ptr<spdlog::formatter> f) override { formatter_ = std::move(f); }
    std::unique_ptr<spdlog::formatter> formatter_;
};

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
    auto ipc_sink_ptr = std::make_shared<ipc_sink>();
    auto logger = std::make_shared<spdlog::logger>(
        "mole", spdlog::sinks_init_list{console_sink, ipc_sink_ptr}
    );
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    ipc_start(&g_ipc);

    return true;
}

bool destroy_logger() {
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    if (!FreeConsole()) {
        return false;
    }

    ipc_kill(&g_ipc);

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

static bool console_process_events(string& out) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    if (hIn == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD numEvents = 0;

    if (!GetNumberOfConsoleInputEvents(hIn, &numEvents) || numEvents == 0) {
        return false;
    }

    vector<INPUT_RECORD> buffer(numEvents);

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

bool get_input(string& out) {
    if (console_process_events(out)) return true;        // console input has priority
    if (ipc_dequeue_command(&g_ipc, &out)) return true;  // ipc commands are always full lines
    return false;
}