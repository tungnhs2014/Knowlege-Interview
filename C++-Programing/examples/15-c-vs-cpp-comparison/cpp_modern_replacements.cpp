#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

template <class T>
constexpr T square(T value) {
    return value * value;
}

using Value = std::variant<int, std::string>;
using File = std::unique_ptr<FILE, int (*)(FILE*)>;

static void print_value(const Value& value) {
    std::visit([](const auto& item) { std::cout << item << '\n'; }, value);
}

static bool starts_with(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

static File open_readme() {
    return File(std::fopen("README.md", "r"), std::fclose);
}

int main() {
    std::vector<int> values{1, 2, 3, 4};

    int sum = 0;
    for (int value : values) {
        sum += value;
    }

    auto report = [sum] {
        std::cout << "C++ vector sum: " << sum << '\n';
    };
    report();

    std::cout << "square(5): " << square(5) << '\n';
    std::cout << std::boolalpha
              << "starts_with(\"sensor:42\", \"sensor\"): "
              << starts_with("sensor:42", "sensor") << '\n';

    print_value(42);
    print_value(std::string{"variant text"});

    File readme = open_readme();
    if (readme) {
        std::cout << "README.md opened through RAII FILE wrapper\n";
    }

    return 0;
}

