#include "client.h"

ipc_client g_ipc = {};

inline void nng_error(const string& msg, const int& code) {
    add_console_line(std::format("ERROR: {}: {}", msg, nng_strerror(code)));
}

void input_func(void* arg) {
    auto client = (ipc_client*)arg;

    if (nng_aio_result(client->in_aio) == 0) {
        auto msg = nng_aio_get_msg(client->in_aio);

        std::lock_guard lock(client->in_mutex);
        client->in_bound.emplace((char*)nng_msg_body(msg), nng_msg_len(msg));

        nng_msg_free(msg);
    } else {
        nng_error("recv aio failed", nng_aio_result(client->in_aio));
    }

    if (client->running) {
        nng_recv_aio(client->sock, client->in_aio);
    }
}

void output_func(void* arg) {
    auto client = (ipc_client*)arg;

    int err = nng_aio_result(client->out_aio);
    if (err != 0) {
        nng_error("send aio failed", err);
        if (nng_msg* msg = nng_aio_get_msg(client->out_aio)) nng_msg_free(msg);
    }

    nng_msg* next_msg = nullptr;
    bool has_more = false;

    {
        std::lock_guard lock(client->out_mutex);
        if (!client->out_bound.empty()) {
            string next = std::move(client->out_bound.front());
            client->out_bound.pop();

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
            client->sending = false;
        }
    }

    if (has_more) {
        nng_aio_set_msg(client->out_aio, next_msg);
        nng_send_aio(client->sock, client->out_aio);
    }
}

bool ipc_connect(ipc_client* client) {
    int err = 0;
    if ((err = nng_pair0_open(&client->sock)) != 0) {
        nng_error("nng_pair0_open", err);
        return false;
    }

    if ((err = nng_dial(client->sock, "ipc://mole", nullptr, 0)) != 0) {
        nng_error("nng_dial", err);
        nng_close(client->sock);
        return false;
    }

    nng_aio_alloc(&client->in_aio, input_func, client);
    nng_aio_alloc(&client->out_aio, output_func, client);

    client->running = true;
    client->sending = false;

    nng_recv_aio(client->sock, client->in_aio);

    return true;
}

void ipc_disconnect(ipc_client* client) {
    if (client->running.exchange(false)) {
        nng_aio_cancel(client->in_aio);
        nng_aio_cancel(client->out_aio);

        nng_aio_wait(client->in_aio);
        nng_aio_wait(client->out_aio);

        nng_aio_free(client->in_aio);
        nng_aio_free(client->out_aio);

        nng_close(client->sock);
    }
}

void ipc_send(ipc_client* client, const string& msg) {
    nng_msg* prepared_msg = nullptr;
    bool start_send = false;

    {
        std::lock_guard lock(client->out_mutex);
        client->out_bound.push(msg);

        if (!client->sending && client->out_bound.size() == 1) {
            client->sending = true;

            string next = std::move(client->out_bound.front());
            client->out_bound.pop();

            int err = nng_msg_alloc(&prepared_msg, 0);
            if (err == 0) {
                err = nng_msg_append(prepared_msg, next.data(), next.size());
            }

            if (err != 0) {
                nng_error("nng_msg_alloc/append failed in broadcast", err);
                if (prepared_msg) nng_msg_free(prepared_msg);
                client->sending = false;
            } else {
                start_send = true;
            }
        }
    }

    if (start_send) {
        nng_aio_set_msg(client->out_aio, prepared_msg);
        nng_send_aio(client->sock, client->out_aio);
    }
}

bool ipc_dequeue_incoming(ipc_client* client, string* out) {
    std::lock_guard lock(client->in_mutex);
    if (client->in_bound.empty()) {
        return false;
    }

    *out = client->in_bound.front();
    client->in_bound.pop();
    return true;
}
