#include "client.h"

ipc_client g_ipc = {};

static void ipc_client_reader(ipc_client* c) {
    char buf[512];
    DWORD read = 0;
    std::string acc;

    while (c->running) {
        BOOL ok = ReadFile(c->pipe, buf, sizeof(buf), &read, nullptr);
        if (!ok || read == 0) {
            c->running = false;
            break;
        }
        acc.append(buf, buf + read);
        size_t pos = 0;
        while ((pos = acc.find('\n')) != string::npos) {
            string line = acc.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            {
                std::lock_guard lock(c->q_mutex);
                c->queue.push(line);
            }
            acc.erase(0, pos + 1);
        }
    }
}

bool ipc_client_connect(ipc_client* c, const char* pipe_name) {
    c->pipe =
        CreateFileA(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (c->pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    c->running = true;
    c->reader = std::thread(ipc_client_reader, c);
    return true;
}

void ipc_client_disconnect(ipc_client* c) {
    c->running = false;
    if (c->pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(c->pipe);
        c->pipe = INVALID_HANDLE_VALUE;
    }
    if (c->reader.joinable()) {
        c->reader.join();
    }
}

bool ipc_client_send(ipc_client* c, const string& msg) {
    if (c->pipe == INVALID_HANDLE_VALUE) return false;
    std::string line = msg;
    if (line.empty() || line.back() != '\n') line.push_back('\n');
    DWORD written = 0;
    BOOL ok = WriteFile(c->pipe, line.c_str(), (DWORD)line.size(), &written, nullptr);
    return ok && written == line.size();
}

bool ipc_client_poll(ipc_client* c, string& out) {
    std::lock_guard lock(c->q_mutex);
    if (c->queue.empty()) return false;
    out = std::move(c->queue.front());
    c->queue.pop();
    return true;
}
