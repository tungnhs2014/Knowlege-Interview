#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};
    values.reserve(3);

    auto saved_index = std::size_t{0};
    const int* old_data = values.data();

    values.push_back(4);
    const bool reallocated = old_data != values.data();

    // Learning point: any iterator, pointer, or reference into old_data would
    // be invalid if reallocated is true. Keep an index or refresh the iterator.
    assert(values[saved_index] == 1);

    for (auto it = values.begin(); it != values.end(); ) {
        if (*it % 2 == 0) {
            it = values.erase(it);
        } else {
            ++it;
        }
    }

    assert((values == std::vector<int>{1, 3}));

    std::cout << "reallocated=" << std::boolalpha << reallocated
              << " safe-first=" << values[saved_index]
              << " size-after-erase=" << values.size()
              << " result=passed\n";
}
