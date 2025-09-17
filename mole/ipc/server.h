#ifndef SERVER_H
#define SERVER_H

#include <common.h>
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>
#include <mutex>
#include <queue>

#include "../logging/logger.h"
#include "../threads/threads.h"

struct ipc_server {
    std::atomic_bool running = false;
    std::atomic_bool sending = false;
    nng_socket sock;
    nng_aio *in_aio, *out_aio;
    std::mutex in_mutex, out_mutex;
    std::queue<string> in_bound;
    std::queue<string> out_bound;
};

extern ipc_server g_ipc;

bool ipc_start(ipc_server* server);
void ipc_kill(ipc_server* server);

void ipc_broadcast(ipc_server* server, const string& msg);
bool ipc_dequeue_command(ipc_server* server, string* out);

#endif  // SERVER_H