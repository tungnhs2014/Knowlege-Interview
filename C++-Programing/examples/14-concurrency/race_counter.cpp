#include <iostream>
#include <thread>

int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter; // Learning-only: intentional data race and undefined behavior.
    }
}

int main() {
    std::thread a(increment);
    std::thread b(increment);

    a.join();
    b.join();

    std::cout << "counter = " << counter << " (expected 200000, but this is UB)\n";
}

