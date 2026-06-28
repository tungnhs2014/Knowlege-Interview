#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

std::string_view stable_label() {
    return "ready"; // string literal has static storage duration
}

void remove_even(std::vector<int>& values) {
    for (auto it = values.begin(); it != values.end();) {
        if (*it % 2 == 0) {
            it = values.erase(it);
        } else {
            ++it;
        }
    }
}

std::function<void()> make_callback() {
    std::string message = "done";
    return [message] {
        std::cout << "callback: " << message << '\n';
    };
}

class IntQueue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            values_.push(value);
        }
        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !values_.empty(); });
        const int value = values_.front();
        values_.pop();
        return value;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> values_;
};

int main() {
    std::cout << "stable label: " << stable_label() << '\n';

    std::vector<int> values{1, 2, 3, 4, 5};
    remove_even(values);

    std::cout << "values:";
    for (const int value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    const auto callback = make_callback();
    callback();

    IntQueue queue;
    queue.push(42);
    std::cout << "popped: " << queue.pop() << '\n';
}
