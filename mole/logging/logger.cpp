#include "logger.h"

struct ipc_sink : spdlog::sinks::sink {
    void log(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t buf;
        formatter_->format(msg, buf);
        string s(buf.data(), buf.size());
        s.push_back('\n');
        ipc_broadcast(s);
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

    ipc_start();

    return true;
}

bool destroy_logger() {
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    if (!FreeConsole()) {
        return false;
    }

    ipc_kill();

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

static std::atomic g_active_source{input_source_t::INPUT_NONE};

static std::queue<string> g_console_queue;
static std::mutex g_console_mutex;
static string g_console_partial;

static std::queue<string> g_ipc_queue;
static std::mutex g_ipc_mutex;

void enqueue_ipc_input(const string& line) {
    {
        std::lock_guard lock(g_ipc_mutex);
        g_ipc_queue.push(line);
    }

    auto expected = input_source_t::INPUT_NONE;
    g_active_source.compare_exchange_strong(expected, input_source_t::INPUT_IPC);
}

static void console_process_events() {
    if (g_active_source.load(std::memory_order_acquire) == input_source_t::INPUT_IPC) {
        return;
    }

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD numEvents = 0;
    if (!GetNumberOfConsoleInputEvents(hIn, &numEvents) || numEvents == 0) {
        return;
    }

    std::vector<INPUT_RECORD> buffer(numEvents);
    DWORD eventsRead = 0;
    if (!ReadConsoleInput(hIn, buffer.data(), numEvents, &eventsRead)) {
        return;
    }

    for (DWORD i = 0; i < eventsRead; ++i) {
        auto& [EventType, Event] = buffer[i];
        if (EventType != KEY_EVENT || !Event.KeyEvent.bKeyDown) continue;

        char c = Event.KeyEvent.uChar.AsciiChar;
        WORD vk = Event.KeyEvent.wVirtualKeyCode;

        if (g_active_source.load(std::memory_order_acquire) == input_source_t::INPUT_NONE) {
            bool meaningful =
                (isprint(static_cast<unsigned char>(c)) || vk == VK_BACK || c == '\r' || c == '\n');
            if (meaningful) {
                auto expected = input_source_t::INPUT_NONE;
                g_active_source.compare_exchange_strong(expected, input_source_t::INPUT_CONSOLE);
            }
        }

        if (g_active_source.load(std::memory_order_acquire) != input_source_t::INPUT_CONSOLE) {
            continue;
        }

        if (c == '\r' || c == '\n') {
            std::cout << std::endl;
            string finished = g_console_partial;
            g_console_partial.clear();

            {
                std::lock_guard lock(g_console_mutex);
                g_console_queue.push(finished);
            }

            return;
        }

        if (vk == VK_BACK && !g_console_partial.empty()) {
            g_console_partial.pop_back();
            std::cout << "\b \b";
            std::cout.flush();
        } else if (isprint(static_cast<unsigned char>(c))) {
            g_console_partial.push_back(c);
            std::cout << c;
            std::cout.flush();
        }
    }
}

bool get_input(string& out) {
    console_process_events();  // console input has priority

    auto active = g_active_source.load(std::memory_order_acquire);

    if (active == input_source_t::INPUT_CONSOLE) {
        std::lock_guard lock(g_console_mutex);
        if (!g_console_queue.empty()) {
            out = std::move(g_console_queue.front());
            g_console_queue.pop();

            if (g_console_queue.empty() && g_console_partial.empty()) {
                g_active_source.store(input_source_t::INPUT_NONE, std::memory_order_release);
            }
            return true;
        }

        return false;
    }

    if (active == input_source_t::INPUT_IPC) {
        std::lock_guard lock(g_ipc_mutex);
        if (!g_ipc_queue.empty()) {
            out = std::move(g_ipc_queue.front());
            g_ipc_queue.pop();

            if (g_ipc_queue.empty()) {
                g_active_source.store(input_source_t::INPUT_NONE, std::memory_order_release);
            }
            return true;
        }

        g_active_source.store(input_source_t::INPUT_NONE, std::memory_order_release);
        return false;
    }

    {
        std::lock_guard lock(g_console_mutex);
        if (!g_console_queue.empty()) {
            auto expected = input_source_t::INPUT_NONE;
            if (g_active_source.compare_exchange_strong(expected, input_source_t::INPUT_CONSOLE)) {
                out = std::move(g_console_queue.front());
                g_console_queue.pop();
                if (g_console_queue.empty() && g_console_partial.empty()) {
                    g_active_source.store(input_source_t::INPUT_NONE, std::memory_order_release);
                }

                return true;
            }
        }
    }

    {
        std::lock_guard lock(g_ipc_mutex);
        if (!g_ipc_queue.empty()) {
            auto expected = input_source_t::INPUT_NONE;
            if (g_active_source.compare_exchange_strong(expected, input_source_t::INPUT_IPC)) {
                out = std::move(g_ipc_queue.front());
                g_ipc_queue.pop();
                if (g_ipc_queue.empty()) {
                    g_active_source.store(input_source_t::INPUT_NONE, std::memory_order_release);
                }

                return true;
            }
        }
    }

    return false;
}