#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {

int mutex_counter = 0;
std::mutex counter_mutex;

std::atomic<int> atomic_counter{0};

void increment_with_mutex(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(counter_mutex);
        ++mutex_counter;
    }
}

void increment_with_atomic(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        atomic_counter.fetch_add(1);
    }
}

} // namespace

int main() {
    constexpr int thread_count = 4;
    constexpr int iterations = 50000;

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(increment_with_mutex, iterations);
        threads.emplace_back(increment_with_atomic, iterations);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "mutex_counter  = " << mutex_counter << "\n";
    std::cout << "atomic_counter = " << atomic_counter.load() << "\n";
}

