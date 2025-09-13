#include "threads.h"

thread_t create_thread(thread_func func, void* arg) {
    auto thread_wrapper = [func, arg]() { func(arg); };

    try {
        return std::thread(thread_wrapper);
    } catch (...) {
        return {};
    }
}

void sleep_thread_ms(unsigned int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
