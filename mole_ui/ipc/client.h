#ifndef CLIENT_H
#define CLIENT_H

#include <common.h>

struct ipc_client {
    HANDLE pipe;
    std::thread reader;
    std::atomic<bool> running;

    std::mutex q_mutex;
    std::queue<string> queue;
};

extern ipc_client g_ipc;

bool ipc_client_connect(ipc_client* c, const char* pipe_name = R"(\\.\pipe\mole_ipc)");
void ipc_client_disconnect(ipc_client* c);

bool ipc_client_send(ipc_client* c, const string& msg);
bool ipc_client_poll(ipc_client* c, string& out);

#endif  // CLIENT_H
