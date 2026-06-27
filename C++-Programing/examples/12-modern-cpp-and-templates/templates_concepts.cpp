#include <array>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

constexpr int max_packet_size = 256;

template <typename T>
constexpr T max_value(T left, T right) {
    return left < right ? right : left;
}

template <typename T, std::size_t Capacity>
class FixedBuffer {
public:
    bool push(T value) {
        if (size_ == Capacity) {
            return false;
        }

        values_[size_] = std::move(value);
        ++size_;
        return true;
    }

    std::size_t size() const {
        return size_;
    }

    const T& at(std::size_t index) const {
        return values_.at(index);
    }

private:
    std::array<T, Capacity> values_{};
    std::size_t size_ = 0;
};

template <typename... Args>
auto sum_all(Args... args) {
    return (args + ...);
}

template <typename T>
concept SizedRange = requires(const T& value) {
    value.begin();
    value.end();
    { value.size() } -> std::convertible_to<std::size_t>;
};

template <SizedRange Range>
std::size_t checked_size(const Range& range) {
    return range.size();
}

int main() {
    FixedBuffer<int, 3> buffer;
    const bool pushed = buffer.push(10) && buffer.push(20) && buffer.push(30);
    const bool full = !buffer.push(40);

    const std::vector<int> values{1, 2, 3, 4};

    const bool ok = max_packet_size == 256 && max_value(3, 7) == 7 && pushed && full &&
                    buffer.size() == 3 && buffer.at(1) == 20 && sum_all(1, 2, 3) == 6 &&
                    checked_size(values) == 4;

    std::cout << "buffer_size=" << buffer.size() << " fold_sum=" << sum_all(1, 2, 3)
              << " range_size=" << checked_size(values)
              << " result=" << (ok ? "passed" : "failed") << '\n';

    return ok ? 0 : 1;
}
