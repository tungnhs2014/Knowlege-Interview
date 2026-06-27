#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <thread>

struct Work {
    int input;
    int output;
};

static void* pthread_worker(void* arg) {
    auto* work = static_cast<Work*>(arg);
    work->output = work->input * 2;
    return nullptr;
}

static void run_pthread_example() {
    Work work{21, 0};
    pthread_t thread{};

    int rc = pthread_create(&thread, nullptr, pthread_worker, &work);
    if (rc != 0) {
        throw std::runtime_error("pthread_create failed");
    }

    rc = pthread_join(thread, nullptr);
    if (rc != 0) {
        throw std::runtime_error("pthread_join failed");
    }

    std::cout << "pthread result: " << work.output << '\n';
}

static void run_std_thread_example() {
    int input = 21;
    int output = 0;

    std::thread thread([input, &output] {
        output = input * 2;
    });

    thread.join();
    std::cout << "std::thread result: " << output << '\n';
}

int main() {
    try {
        run_pthread_example();
        run_std_thread_example();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}

