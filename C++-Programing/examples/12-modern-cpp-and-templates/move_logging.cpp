#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct TracedPayload {
    static inline int copies = 0;
    static inline int moves = 0;

    std::string value;

    explicit TracedPayload(std::string text) : value(std::move(text)) {}

    TracedPayload(const TracedPayload& other) : value(other.value) {
        ++copies;
    }

    TracedPayload& operator=(const TracedPayload& other) {
        if (this != &other) {
            value = other.value;
            ++copies;
        }
        return *this;
    }

    TracedPayload(TracedPayload&& other) noexcept : value(std::move(other.value)) {
        ++moves;
    }

    TracedPayload& operator=(TracedPayload&& other) noexcept {
        if (this != &other) {
            value = std::move(other.value);
            ++moves;
        }
        return *this;
    }
};

int main() {
    std::vector<TracedPayload> values;
    values.reserve(1);
    values.emplace_back("first");

    TracedPayload second("second");
    values.push_back(std::move(second));

    second.value = "reused";

    const bool ok = TracedPayload::copies == 0 && TracedPayload::moves >= 1 &&
                    second.value == "reused" && values.size() == 2;

    std::cout << "copies=" << TracedPayload::copies << " moves=" << TracedPayload::moves
              << " reused=" << std::boolalpha << (second.value == "reused")
              << " result=" << (ok ? "passed" : "failed") << '\n';

    return ok ? 0 : 1;
}
