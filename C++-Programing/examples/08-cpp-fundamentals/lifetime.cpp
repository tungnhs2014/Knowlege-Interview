#include <iostream>

class ScopeCounter {
public:
    explicit ScopeCounter(int& active_count)
        : active_count_{active_count}
    {
        ++active_count_;
    }

    ScopeCounter(const ScopeCounter&) = delete;
    ScopeCounter& operator=(const ScopeCounter&) = delete;

    ~ScopeCounter()
    {
        --active_count_;
    }

private:
    int& active_count_;
};

auto make_counter()
{
    int count = 0;
    return [count]() mutable {
        return ++count;
    };
}

int main()
{
    int active_count = 0;

    {
        ScopeCounter first{active_count};
        ScopeCounter second{active_count};
        if (active_count != 2) {
            return 1;
        }
    }

    auto counter = make_counter();
    const bool passed = active_count == 0
        && counter() == 1
        && counter() == 2;

    std::cout
        << "active-after-scope=" << active_count
        << " callback-state=2"
        << " result=" << (passed ? "passed" : "failed")
        << '\n';

    return passed ? 0 : 1;
}
