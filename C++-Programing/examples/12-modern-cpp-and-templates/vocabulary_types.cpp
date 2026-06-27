#include <array>
#include <charconv>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

std::optional<int> parse_positive(std::string_view text) {
    int value = 0;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec != std::errc{} || ptr != last || value <= 0) {
        return std::nullopt;
    }

    return value;
}

int sum(std::span<const int> values) {
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}

struct Temperature {
    int celsius;
};

struct Fault {
    std::string_view code;
};

using Event = std::variant<Temperature, Fault>;

std::string describe(const Event& event) {
    return std::visit(
        Overloaded{
            [](Temperature reading) {
                return std::string("temperature:") + std::to_string(reading.celsius);
            },
            [](Fault fault) {
                return std::string("fault:") + std::string(fault.code);
            },
        },
        event);
}

int main() {
    const auto parsed = parse_positive("42");
    const auto rejected = parse_positive("-1");
    const std::array samples{1, 2, 3};
    const Event event = Fault{"E42"};

    const bool ok = parsed.has_value() && *parsed == 42 && !rejected.has_value() &&
                    sum(samples) == 6 && describe(event) == "fault:E42";

    std::cout << "parsed=" << parsed.value_or(0)
              << " rejected=" << std::boolalpha << !rejected.has_value()
              << " sum=" << sum(samples) << " event=" << describe(event)
              << " result=" << (ok ? "passed" : "failed") << '\n';

    return ok ? 0 : 1;
}
