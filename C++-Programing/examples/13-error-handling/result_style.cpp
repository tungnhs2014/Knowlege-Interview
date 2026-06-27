#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

enum class ParseError {
    Empty,
    BadDigit,
    Overflow
};

static const char* to_string(ParseError error) noexcept
{
    switch (error) {
    case ParseError::Empty:
        return "empty";
    case ParseError::BadDigit:
        return "bad digit";
    case ParseError::Overflow:
        return "overflow";
    }
    return "unknown";
}

template <typename T, typename E>
class Result {
public:
    static Result ok(T value)
    {
        return Result(std::move(value));
    }

    static Result err(E error)
    {
        return Result(error);
    }

    bool has_value() const noexcept
    {
        return has_value_;
    }

    const T& value() const
    {
        if (!has_value_) {
            throw std::logic_error("Result::value() called without a value");
        }
        return value_;
    }

    E error() const
    {
        if (has_value_) {
            throw std::logic_error("Result::error() called with a value");
        }
        return error_;
    }

private:
    explicit Result(T value)
        : has_value_(true), value_(std::move(value)), error_{}
    {
    }

    explicit Result(E error)
        : has_value_(false), value_{}, error_(error)
    {
    }

    bool has_value_;
    T value_;
    E error_;
};

static Result<unsigned, ParseError> parse_u8_result(const std::string& text)
{
    if (text.empty()) {
        return Result<unsigned, ParseError>::err(ParseError::Empty);
    }

    unsigned value = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return Result<unsigned, ParseError>::err(ParseError::BadDigit);
        }

        value = value * 10u + static_cast<unsigned>(ch - '0');
        if (value > 255u) {
            return Result<unsigned, ParseError>::err(ParseError::Overflow);
        }
    }

    return Result<unsigned, ParseError>::ok(value);
}

int main()
{
    const auto ok = parse_u8_result("77");
    const auto bad = parse_u8_result("999");

    if (ok.has_value()) {
        std::cout << "ok-value=" << ok.value() << '\n';
    }
    if (!bad.has_value()) {
        std::cout << "bad-error=" << to_string(bad.error()) << '\n';
    }

    bool unchecked_access_rejected = false;
    try {
        (void)bad.value();
    } catch (const std::logic_error&) {
        unchecked_access_rejected = true;
    }
    std::cout << "unchecked-access-rejected="
              << unchecked_access_rejected << '\n';

    const bool passed = ok.has_value() && ok.value() == 77u &&
                        !bad.has_value() &&
                        bad.error() == ParseError::Overflow &&
                        unchecked_access_rejected;

    std::cout << "result=" << (passed ? "passed" : "failed") << '\n';
    return passed ? 0 : 1;
}
