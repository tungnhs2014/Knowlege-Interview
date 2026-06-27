#include <functional>
#include <iostream>
#include <memory>

std::function<int()> make_counter() {
    return [count = 0]() mutable {
        return ++count;
    };
}

int call_twice(int (*operation)(int), int value) {
    return operation(operation(value));
}

int main() {
    auto counter = make_counter();
    const int first = counter();
    const int second = counter();

    auto owned_value = [number = std::make_unique<int>(21)] {
        return *number * 2;
    };

    auto add_one = [](int value) {
        return value + 1;
    };

    const int moved_capture = owned_value();
    const int function_pointer = call_twice(add_one, 10);

    const bool ok = first == 1 && second == 2 && moved_capture == 42 && function_pointer == 12;

    std::cout << "counter=" << second << " moved_capture=" << moved_capture
              << " function_pointer=" << function_pointer
              << " result=" << (ok ? "passed" : "failed") << '\n';

    return ok ? 0 : 1;
}
