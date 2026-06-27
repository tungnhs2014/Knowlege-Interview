#include <iostream>
#include <utility>

namespace fake_api {

constexpr int invalid_handle = -1;
int releases = 0;

int open()
{
    return 42;
}

void close(int handle) noexcept
{
    if (handle != invalid_handle) {
        ++releases;
    }
}

} // namespace fake_api

class Session {
public:
    explicit Session(int handle) noexcept
        : handle_{handle}
    {
    }

    ~Session() noexcept
    {
        reset();
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Session(Session&& other) noexcept
        : handle_{std::exchange(other.handle_, fake_api::invalid_handle)}
    {
    }

    Session& operator=(Session&& other) noexcept
    {
        if (this != &other) {
            reset();
            handle_ =
                std::exchange(other.handle_, fake_api::invalid_handle);
        }
        return *this;
    }

    bool valid() const noexcept
    {
        return handle_ != fake_api::invalid_handle;
    }

private:
    void reset() noexcept
    {
        if (valid()) {
            fake_api::close(handle_);
            handle_ = fake_api::invalid_handle;
        }
    }

    int handle_{fake_api::invalid_handle};
};

int main()
{
    {
        Session first{fake_api::open()};
        Session second{std::move(first)};
        Session third{fake_api::invalid_handle};
        third = std::move(second);

        if (first.valid() || second.valid() || !third.valid()) {
            return 1;
        }
    }

    const bool passed = fake_api::releases == 1;
    std::cout
        << "release-count=" << fake_api::releases
        << " result=" << (passed ? "passed" : "failed")
        << '\n';
    return passed ? 0 : 1;
}
