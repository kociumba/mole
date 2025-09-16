#include "server.h"

bool g_ipc_running = false;
HANDLE g_ipc_thread = nullptr;
vector<ipc_client> g_ipc_clients;
std::mutex g_ipc_mutex;

static std::mutex g_cmd_mutex;
static std::queue<string> g_cmd_queue;

static DWORD WINAPI ipc_accept_loop(LPVOID);

bool ipc_start() {
    g_ipc_running = true;
    g_ipc_thread = CreateThread(nullptr, 0, ipc_accept_loop, nullptr, 0, nullptr);
    return g_ipc_thread != nullptr;
}

void ipc_kill() {
    g_ipc_running = false;
    if (g_ipc_thread) {
        WaitForSingleObject(g_ipc_thread, 1000);
        CloseHandle(g_ipc_thread);
        g_ipc_thread = nullptr;
    }

    std::lock_guard lock(g_ipc_mutex);
    for (auto& c : g_ipc_clients) {
        if (c.pipe != INVALID_HANDLE_VALUE) CloseHandle(c.pipe);
    }

    g_ipc_clients.clear();
}

void ipc_broadcast(const string& msg) {
    std::lock_guard lock(g_ipc_mutex);
    for (auto it = g_ipc_clients.begin(); it != g_ipc_clients.end();) {
        DWORD written = 0;
        BOOL ok = WriteFile(it->pipe, msg.c_str(), (DWORD)msg.size(), &written, nullptr);
        if (!ok) {
            CloseHandle(it->pipe);
            it = g_ipc_clients.erase(it);
        } else {
            ++it;
        }
    }
}

bool ipc_dequeue_command(string& out) {
    std::lock_guard lock(g_cmd_mutex);
    if (g_cmd_queue.empty()) return false;
    out = g_cmd_queue.front();
    g_cmd_queue.pop();
    return true;
}

static DWORD WINAPI ipc_accept_loop(LPVOID) {
    while (g_ipc_running) {
        HANDLE pipe = CreateNamedPipeA(
            R"(\\.\pipe\mole_ipc)", PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 4096,
            4096, 0, nullptr
        );
        if (pipe == INVALID_HANDLE_VALUE) {
            sleep_thread_ms(1000);
            continue;
        }

        BOOL ok = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!ok) {
            CloseHandle(pipe);
            continue;
        }

        {
            std::lock_guard lock(g_ipc_mutex);
            g_ipc_clients.push_back({pipe, true});
        }

        thread_t([pipe] {
            string acc;
            char buf[512];
            DWORD read = 0;
            while (ReadFile(pipe, buf, sizeof(buf), &read, nullptr) && read > 0) {
                acc.append(buf, buf + read);
                size_t pos = 0;
                while ((pos = acc.find('\n')) != string::npos) {
                    string line = acc.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    enqueue_ipc_input(line);
                    acc.erase(0, pos + 1);
                }
            }

            if (!acc.empty()) {
                enqueue_ipc_input(acc);
            }
        }).detach();
    }

    return 0;
}