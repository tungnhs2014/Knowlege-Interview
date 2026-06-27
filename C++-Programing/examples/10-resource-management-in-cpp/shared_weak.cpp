#include <iostream>
#include <memory>

struct Parent;

struct Child {
    explicit Child(int& destructions) noexcept
        : destructions_{destructions}
    {
    }

    ~Child()
    {
        ++destructions_;
    }

    std::weak_ptr<Parent> parent;

private:
    int& destructions_;
};

struct Parent {
    explicit Parent(int& destructions) noexcept
        : destructions_{destructions}
    {
    }

    ~Parent()
    {
        ++destructions_;
    }

    std::shared_ptr<Child> child;

private:
    int& destructions_;
};

int main()
{
    int parent_destructions = 0;
    int child_destructions = 0;
    std::weak_ptr<Parent> observed;

    {
        auto parent = std::make_shared<Parent>(parent_destructions);
        auto child = std::make_shared<Child>(child_destructions);
        parent->child = child;
        child->parent = parent;
        observed = parent;

        if (!observed.lock()) {
            return 1;
        }
    }

    const bool expired = !observed.lock();
    const bool passed =
        expired
        && parent_destructions == 1
        && child_destructions == 1;

    std::cout
        << std::boolalpha
        << "expired=" << expired
        << " parent-destructions=" << parent_destructions
        << " child-destructions=" << child_destructions
        << " result=" << (passed ? "passed" : "failed")
        << '\n';
    return passed ? 0 : 1;
}
