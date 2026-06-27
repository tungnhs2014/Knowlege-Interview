#include <iostream>
#include <memory>
#include <span>
#include <vector>

static int sum(std::span<const int> values)
{
    int total = 0;

    for (int value : values) {
        total += value;
    }

    return total;
}

int main()
{
    std::vector<int> values{1, 2, 3, 4};
    std::span<const int> view{values};

    std::cout << "sum=" << sum(view) << '\n';

    auto owner = std::make_unique<int>(42);
    int *observer = owner.get();

    std::cout << "observed=" << *observer << '\n';

    owner.reset();
    observer = nullptr;

    // A span or raw observer is non-owning. Do not use it after the owner or
    // backing container invalidates the target.
    return 0;
}
