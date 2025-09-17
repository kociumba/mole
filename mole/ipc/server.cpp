#include "server.h"

ipc_server g_ipc;

inline void nng_error(const string& msg, const int& code) {
    error("{}: {}", msg, nng_strerror(code));
}

void input_func(void* arg) {
    auto server = (ipc_server*)arg;

    if (nng_aio_result(server->in_aio) == 0) {
        auto msg = nng_aio_get_msg(server->in_aio);

        std::lock_guard lock(server->in_mutex);
        server->in_bound.emplace((char*)nng_msg_body(msg), nng_msg_len(msg));

        nng_msg_free(msg);
    } else {
        nng_error("recv aio failed", nng_aio_result(server->in_aio));
    }

    if (server->running) {
        nng_recv_aio(server->sock, server->in_aio);
    }
}

void output_func(void* arg) {
    auto server = (ipc_server*)arg;

    int err = nng_aio_result(server->out_aio);
    if (err != 0) {
        nng_error("send aio failed", err);
        if (nng_msg* msg = nng_aio_get_msg(server->out_aio)) nng_msg_free(msg);
    }

    nng_msg* next_msg = nullptr;
    bool has_more = false;

    {
        std::lock_guard lock(server->out_mutex);
        if (!server->out_bound.empty()) {
            string next = std::move(server->out_bound.front());
            server->out_bound.pop();

            int rv = nng_msg_alloc(&next_msg, 0);
            if (rv == 0) {
                rv = nng_msg_append(next_msg, next.data(), next.size());
            }

            if (rv != 0) {
                nng_error("nng_msg_alloc/append failed in output_func", rv);
                if (next_msg) nng_msg_free(next_msg);
                next_msg = nullptr;
            } else {
                has_more = true;
            }
        }

        if (!has_more) {
            server->sending = false;
        }
    }

    if (has_more) {
        nng_aio_set_msg(server->out_aio, next_msg);
        nng_send_aio(server->sock, server->out_aio);
    }
}

bool ipc_start(ipc_server* server) {
    int err = 0;
    if ((err = nng_pair0_open(&server->sock)) != 0) {
        nng_error("nng_pair0_open", err);
    }

    if ((err = nng_listen(server->sock, "ipc://mole", nullptr, 0)) != 0) {
        nng_error("nng_listen", err);
    }

    nng_aio_alloc(&server->in_aio, input_func, server);
    nng_aio_alloc(&server->out_aio, output_func, server);

    server->running = true;
    server->sending = false;

    nng_recv_aio(server->sock, server->in_aio);

    if (err == 0) {
        return true;
    }

    return false;
}

void ipc_kill(ipc_server* server) {
    if (server->running.exchange(false)) {
        nng_aio_cancel(server->in_aio);
        nng_aio_cancel(server->out_aio);

        nng_aio_wait(server->in_aio);
        nng_aio_wait(server->out_aio);

        nng_aio_free(server->in_aio);
        nng_aio_free(server->out_aio);

        nng_close(server->sock);
    }
}

void ipc_broadcast(ipc_server* server, const string& msg) {
    nng_msg* prepared_msg = nullptr;
    bool start_send = false;

    {
        std::lock_guard lock(server->out_mutex);
        server->out_bound.push(msg);

        if (!server->sending && server->out_bound.size() == 1) {
            server->sending = true;

            string next = std::move(server->out_bound.front());
            server->out_bound.pop();

            int err = nng_msg_alloc(&prepared_msg, 0);
            if (err == 0) {
                err = nng_msg_append(prepared_msg, next.data(), next.size());
            }

            if (err != 0) {
                nng_error("nng_msg_alloc/append failed in broadcast", err);
                if (prepared_msg) nng_msg_free(prepared_msg);
                server->sending = false;
            } else {
                start_send = true;
            }
        }
    }

    if (start_send) {
        nng_aio_set_msg(server->out_aio, prepared_msg);
        nng_send_aio(server->sock, server->out_aio);
    }
}

bool ipc_dequeue_command(ipc_server* server, string* out) {
    std::lock_guard lock(server->in_mutex);
    if (server->in_bound.empty()) {
        return false;
    }

    *out = server->in_bound.front();
    server->in_bound.pop();
    return true;
}