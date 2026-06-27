#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class Queue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            values_.push(value);
        }
        cv_.notify_one();
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
        }
        cv_.notify_all();
    }

    bool pop(int& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return finished_ || !values_.empty();
        });

        if (values_.empty()) {
            return false;
        }

        out = values_.front();
        values_.pop();
        return true;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> values_;
    bool finished_{false};
};

int main() {
    Queue queue;
    int sum = 0;

    std::thread consumer([&] {
        int value = 0;
        while (queue.pop(value)) {
            sum += value;
        }
    });

    std::thread producer([&] {
        for (int i = 1; i <= 4; ++i) {
            queue.push(i);
        }
        queue.finish();
    });

    producer.join();
    consumer.join();

    std::cout << "condition_variable sum: " << sum << '\n';
    return 0;
}

