#ifndef CLIENT_H
#define CLIENT_H

#include <common.h>
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>
#include <format>

#include <../ui/root.h>

struct ipc_client {
    std::atomic_bool running = false;
    std::atomic_bool sending = false;
    nng_socket sock;
    nng_aio *in_aio, *out_aio;
    std::mutex in_mutex, out_mutex;
    std::queue<string> in_bound;
    std::queue<string> out_bound;
};

extern ipc_client g_ipc;

bool ipc_connect(ipc_client* client);
void ipc_disconnect(ipc_client* client);

void ipc_send(ipc_client* client, const string& msg);
bool ipc_dequeue_incoming(ipc_client* client, string* out);

#endif  // CLIENT_H
