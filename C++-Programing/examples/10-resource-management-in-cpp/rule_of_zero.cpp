#include <algorithm>
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

class ManualBuffer {
public:
    explicit ManualBuffer(std::size_t size)
        : data_{size == 0U ? nullptr : new int[size]{}},
          size_{size}
    {
    }

    ~ManualBuffer()
    {
        delete[] data_;
    }

    ManualBuffer(const ManualBuffer& other)
        : data_{copy_data(other)},
          size_{other.size_}
    {
    }

    ManualBuffer& operator=(const ManualBuffer& other)
    {
        if (this != &other) {
            ManualBuffer replacement{other};
            swap(replacement);
        }
        return *this;
    }

    ManualBuffer(ManualBuffer&& other) noexcept
        : data_{std::exchange(other.data_, nullptr)},
          size_{std::exchange(other.size_, 0U)}
    {
    }

    ManualBuffer& operator=(ManualBuffer&& other) noexcept
    {
        if (this != &other) {
            delete[] data_;
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0U);
        }
        return *this;
    }

    static void fail_next_copy_for_test() noexcept
    {
        fail_next_copy_ = true;
    }

    void swap(ManualBuffer& other) noexcept
    {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
    }

    int& at(std::size_t index)
    {
        if (index >= size_) {
            throw std::out_of_range{"ManualBuffer index"};
        }
        return data_[index];
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

private:
    static int* copy_data(const ManualBuffer& other)
    {
        if (fail_next_copy_) {
            fail_next_copy_ = false;
            throw std::bad_alloc{};
        }
        if (other.size_ == 0U) {
            return nullptr;
        }

        int* copy = new int[other.size_];
        std::copy(other.data_, other.data_ + other.size_, copy);
        return copy;
    }

    inline static bool fail_next_copy_{false};
    int* data_;
    std::size_t size_;
};

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(size)
    {
    }

    int& at(std::size_t index)
    {
        return data_.at(index);
    }

    std::size_t size() const noexcept
    {
        return data_.size();
    }

private:
    std::vector<int> data_;
};

int main()
{
    ManualBuffer manual{2U};
    manual.at(0U) = 7;
    ManualBuffer copied = manual;
    copied.at(0U) = 9;
    ManualBuffer moved = std::move(copied);
    ManualBuffer copy_assigned{1U};
    copy_assigned = manual;
    copy_assigned = copy_assigned;

    ManualBuffer move_assigned{1U};
    move_assigned = std::move(moved);
    move_assigned = std::move(move_assigned);

    ManualBuffer copied_from_moved{moved};

    ManualBuffer failure_target{1U};
    failure_target.at(0U) = 55;
    ManualBuffer::fail_next_copy_for_test();
    bool failure_preserved_state = false;
    try {
        failure_target = manual;
    } catch (const std::bad_alloc&) {
        failure_preserved_state =
            failure_target.size() == 1U
            && failure_target.at(0U) == 55;
    }

    Buffer zero{2U};
    zero.at(0U) = 11;
    Buffer zero_copy = zero;
    zero_copy.at(0U) = 13;

    const bool passed =
        manual.at(0U) == 7
        && copy_assigned.at(0U) == 7
        && move_assigned.at(0U) == 9
        && move_assigned.size() == 2U
        && moved.size() == 0U
        && copied_from_moved.size() == 0U
        && failure_preserved_state
        && zero.at(0U) == 11
        && zero_copy.at(0U) == 13;

    std::cout
        << "manual-copy-independent=" << std::boolalpha
        << (manual.at(0U) != move_assigned.at(0U))
        << " assignments=true"
        << " moved-from-empty=" << (moved.size() == 0U)
        << " failure-safe=" << failure_preserved_state
        << " rule-zero-copy-independent="
        << (zero.at(0U) != zero_copy.at(0U))
        << " result=" << (passed ? "passed" : "failed")
        << '\n';

    return passed ? 0 : 1;
}
