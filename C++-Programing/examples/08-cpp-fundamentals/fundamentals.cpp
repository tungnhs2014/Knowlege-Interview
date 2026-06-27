#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sensing {

class Reading {
public:
    explicit Reading(int milli_celsius)
        : milli_celsius_{milli_celsius}
    {
        if (milli_celsius_ < -50000 || milli_celsius_ > 150000) {
            throw std::out_of_range{"temperature outside supported range"};
        }
    }

    int milli_celsius() const
    {
        return milli_celsius_;
    }

private:
    int milli_celsius_;
};

bool is_alarm(const Reading& reading)
{
    return reading.milli_celsius() >= 80000;
}

bool is_alarm(int milli_celsius)
{
    return is_alarm(Reading{milli_celsius});
}

struct Millivolts {
    int value;
};

constexpr Millivolts operator+(
    Millivolts left,
    Millivolts right)
{
    if ((right.value > 0
            && left.value > std::numeric_limits<int>::max() - right.value)
        || (right.value < 0
            && left.value < std::numeric_limits<int>::min() - right.value)) {
        throw std::overflow_error{"millivolt addition overflow"};
    }

    return Millivolts{left.value + right.value};
}

} // namespace sensing

int main()
{
    std::vector<sensing::Reading> readings{
        sensing::Reading{25000},
        sensing::Reading{80000},
        sensing::Reading{82000}
    };

    const auto alarms = std::count_if(
        readings.begin(),
        readings.end(),
        [](const sensing::Reading& reading) {
            return sensing::is_alarm(reading);
        });

    constexpr sensing::Millivolts rail_a{1200};
    constexpr sensing::Millivolts rail_b{300};
    static_assert((rail_a + rail_b).value == 1500);

    bool invalid_rejected = false;
    try {
        const sensing::Reading invalid{200000};
        (void)invalid;
    } catch (const std::out_of_range&) {
        invalid_rejected = true;
    }

    bool positive_overflow_rejected = false;
    try {
        const sensing::Millivolts maximum{
            std::numeric_limits<int>::max()
        };
        const auto invalid_sum = maximum + sensing::Millivolts{1};
        (void)invalid_sum;
    } catch (const std::overflow_error&) {
        positive_overflow_rejected = true;
    }

    bool negative_overflow_rejected = false;
    try {
        const sensing::Millivolts minimum{
            std::numeric_limits<int>::min()
        };
        const auto invalid_sum = minimum + sensing::Millivolts{-1};
        (void)invalid_sum;
    } catch (const std::overflow_error&) {
        negative_overflow_rejected = true;
    }

    const bool overflow_rejected =
        positive_overflow_rejected && negative_overflow_rejected;

    const bool passed = alarms == 2
        && sensing::is_alarm(80000)
        && invalid_rejected
        && overflow_rejected;

    std::cout
        << "alarms=" << alarms
        << " invalid-rejected=" << std::boolalpha << invalid_rejected
        << " overflow-rejected=" << overflow_rejected
        << " result=" << (passed ? "passed" : "failed")
        << '\n';

    return passed ? 0 : 1;
}
