#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

struct MaybeThrowMove {
    static int copies;
    static int moves;

    MaybeThrowMove() = default;
    MaybeThrowMove(const MaybeThrowMove&)
    {
        ++copies;
    }
    MaybeThrowMove(MaybeThrowMove&&) noexcept(false)
    {
        ++moves;
    }
};

int MaybeThrowMove::copies = 0;
int MaybeThrowMove::moves = 0;

struct NoThrowMove {
    static int copies;
    static int moves;

    NoThrowMove() = default;
    NoThrowMove(const NoThrowMove&)
    {
        ++copies;
    }
    NoThrowMove(NoThrowMove&&) noexcept
    {
        ++moves;
    }
};

int NoThrowMove::copies = 0;
int NoThrowMove::moves = 0;

template <typename T>
static void force_vector_reallocation()
{
    std::vector<T> values;
    values.reserve(1);
    values.emplace_back();
    values.emplace_back();
}

int main()
{
    static_assert(!std::is_nothrow_move_constructible<MaybeThrowMove>::value,
                  "MaybeThrowMove is intentionally not noexcept");
    static_assert(std::is_nothrow_move_constructible<NoThrowMove>::value,
                  "NoThrowMove should be noexcept");

    force_vector_reallocation<MaybeThrowMove>();
    force_vector_reallocation<NoThrowMove>();

    std::cout << "throwing-move copies=" << MaybeThrowMove::copies
              << " moves=" << MaybeThrowMove::moves << '\n';
    std::cout << "nothrow-move copies=" << NoThrowMove::copies
              << " moves=" << NoThrowMove::moves << '\n';

    const bool passed = MaybeThrowMove::copies >= 1 && NoThrowMove::moves >= 1;
    std::cout << "result=" << (passed ? "passed" : "failed") << '\n';
    return passed ? 0 : 1;
}
