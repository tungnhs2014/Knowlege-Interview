#include <iostream>

int left_value();
int right_value();

int main() {
    std::cout << "inline-sum=" << left_value() + right_value() << '\n';
}
