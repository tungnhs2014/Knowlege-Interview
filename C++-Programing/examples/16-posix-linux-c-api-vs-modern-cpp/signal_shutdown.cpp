#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;

extern "C" void handle_signal(int) {
    stop_requested = 1;
}

static void install_handler() {
    struct sigaction action {};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGTERM, &action, nullptr) == -1) {
        throw std::system_error(errno, std::generic_category(), "sigaction");
    }
}

int main() {
    try {
        install_handler();

        std::cout << "raising SIGTERM to request shutdown\n";
        if (raise(SIGTERM) != 0) {
            throw std::runtime_error("raise failed");
        }

        while (!stop_requested) {
            pause();
        }

        std::cout << "cleanup runs outside the signal handler\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}

