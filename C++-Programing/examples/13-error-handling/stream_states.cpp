#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

static void print_state(const std::istream& input, const char* label)
{
    std::cout << label << " good=" << input.good()
              << " eof=" << input.eof()
              << " fail=" << input.fail()
              << " bad=" << input.bad() << '\n';
}

static int sum_numbers(std::istream& input)
{
    int sum = 0;
    int value = 0;

    while (input >> value) {
        sum += value;
    }

    if (input.bad()) {
        throw std::runtime_error("serious I/O error");
    }

    if (input.fail() && !input.eof()) {
        throw std::runtime_error("format error before EOF");
    }

    return sum;
}

int main()
{
    std::istringstream valid{"10 20 30"};
    const int sum = sum_numbers(valid);
    print_state(valid, "valid-after-read");

    bool format_error = false;
    std::istringstream invalid{"10 xx 30"};
    try {
        (void)sum_numbers(invalid);
    } catch (const std::exception& e) {
        format_error = true;
        std::cout << "invalid-error=" << e.what() << '\n';
        print_state(invalid, "invalid-after-error");
    }

    std::ifstream missing{"missing-stream-demo.txt"};
    std::cout << "missing-opened=" << missing.is_open()
              << " fail=" << missing.fail() << '\n';

    const bool passed = sum == 60 && format_error && !missing.is_open();
    std::cout << "sum=" << sum
              << " format-error=" << format_error
              << " result=" << (passed ? "passed" : "failed") << '\n';
    return passed ? 0 : 1;
}
