#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

volatile std::sig_atomic_t stop_requested = 0;

extern "C" void handle_signal(int) {
    stop_requested = 1;
}

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "Running. Press Ctrl+C or send SIGTERM to stop.\n";

    while (!stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "." << std::flush;
    }

    std::cout << "\nCleanup happens outside the signal handler.\n";
}

