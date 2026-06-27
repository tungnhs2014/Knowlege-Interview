#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

class Fd {
public:
    explicit Fd(int fd = -1) noexcept : fd_(fd) {}

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept : fd_(other.release()) {}

    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.release();
        }
        return *this;
    }

    ~Fd() {
        reset();
    }

    int get() const noexcept {
        return fd_;
    }

    int release() noexcept {
        int old = fd_;
        fd_ = -1;
        return old;
    }

    void reset(int fd = -1) noexcept {
        if (fd_ >= 0) {
            (void)::close(fd_);
        }
        fd_ = fd;
    }

private:
    int fd_;
};

static Fd open_for_write(const char* path) {
    int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "open");
    }
    return Fd(fd);
}

static void write_all(int fd, std::string_view text) {
    const char* data = text.data();
    std::size_t sent = 0;

    while (sent < text.size()) {
        ssize_t n = ::write(fd, data + sent, text.size() - sent);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        throw std::system_error(errno, std::generic_category(), "write");
    }
}

int main() {
    const char* path = "fd_raii_demo.txt";

    try {
        Fd file = open_for_write(path);
        write_all(file.get(), "POSIX fd owned by C++ RAII\n");

        Fd moved = std::move(file);
        write_all(moved.get(), "moved owner still closes exactly once\n");

        std::cout << "wrote " << path << '\n';
    } catch (const std::system_error& e) {
        std::cerr << e.what() << ": " << std::strerror(e.code().value()) << '\n';
        return 1;
    }

    return 0;
}

