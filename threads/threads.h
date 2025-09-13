#ifndef THREADS_H
#define THREADS_H

#include <chrono>
#include <thread>

using thread_t = std::thread;
using thread_func = void (*)(void*);

thread_t create_thread(thread_func func, void* arg);
void sleep_thread_ms(unsigned int ms);

#endif  // THREADS_H
