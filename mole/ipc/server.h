#ifndef SERVER_H
#define SERVER_H

#include <common.h>
#include <mutex>
#include <queue>

#include "../commands/commands.h"
#include "../threads/threads.h"

struct ipc_client {
    HANDLE pipe;
    bool active;
};

extern bool g_ipc_running;
extern HANDLE g_ipc_thread;
extern std::vector<ipc_client> g_ipc_clients;
extern std::mutex g_ipc_mutex;

bool ipc_start();
void ipc_kill();

void ipc_broadcast(const string& msg);

bool ipc_dequeue_command(string& out);

#endif  // SERVER_H