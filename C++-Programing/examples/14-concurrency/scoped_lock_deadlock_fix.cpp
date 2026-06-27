#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

int left_value = 1;
int right_value = 2;
std::mutex left_mutex;
std::mutex right_mutex;

void safe_swap() {
    std::scoped_lock lock(left_mutex, right_mutex);
    std::swap(left_value, right_value);
}

int main() {
    std::thread a(safe_swap);
    std::thread b(safe_swap);

    a.join();
    b.join();

    std::cout << "left=" << left_value << " right=" << right_value << "\n";
}

