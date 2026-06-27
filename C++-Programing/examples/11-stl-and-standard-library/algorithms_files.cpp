#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<int> read_values(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open input file: " + path);
    }

    std::vector<int> values;
    int value{};
    while (input >> value) {
        values.push_back(value);
    }

    if (input.bad()) {
        throw std::runtime_error("I/O error while reading: " + path);
    }

    if (!input.eof()) {
        throw std::runtime_error("non-integer token in: " + path);
    }

    return values;
}

void write_demo_file(const std::string& path)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("cannot create output file: " + path);
    }

    output << "7 2 9 2 5\n";
    output.close();
    if (!output) {
        throw std::runtime_error("failed to flush/close output file: " + path);
    }
}

int main()
{
    const std::string path = "build/readings.txt";
    write_demo_file(path);

    std::vector<int> values = read_values(path);
    std::sort(values.begin(), values.end());

    const bool has_five = std::binary_search(values.begin(), values.end(), 5);
    const auto first_two = std::lower_bound(values.begin(), values.end(), 2);
    const auto after_twos = std::upper_bound(values.begin(), values.end(), 2);

    std::vector<int> squares(values.size());
    std::transform(values.begin(), values.end(), squares.begin(),
                   [](int value) { return value * value; });

    const int sum = std::accumulate(values.begin(), values.end(), 0);
    const int square_sum = std::accumulate(squares.begin(), squares.end(), 0);

    assert(has_five);
    assert(after_twos - first_two == 2);
    assert(sum == 25);
    assert(square_sum == 163);

    std::cout << "count=" << values.size()
              << " sum=" << sum
              << " square-sum=" << square_sum
              << " twos=" << (after_twos - first_two)
              << " result=passed\n";
}
