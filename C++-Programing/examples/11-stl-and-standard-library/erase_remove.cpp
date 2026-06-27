#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void remove_invalid_readings(std::vector<int>& readings)
{
    readings.erase(
        std::remove_if(readings.begin(), readings.end(),
                       [](int value) { return value < 0; }),
        readings.end());
}

int main()
{
    std::vector<int> readings{42, -1, 44, -99, 45, 0};

    remove_invalid_readings(readings);

    const std::vector<int> expected{42, 44, 45, 0};
    assert(readings == expected);

    for (int reading : readings) {
        std::cout << reading << ' ';
    }
    std::cout << "result=passed\n";
}
