#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

class IntQueue {
public:
    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) {
                return false;
            }
            values_.push(value);
        }
        cv_.notify_one();
        return true;
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return closed_ || !values_.empty();
        });

        if (values_.empty()) {
            return std::nullopt;
        }

        int value = values_.front();
        values_.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> values_;
    bool closed_ = false;
};

int main() {
    IntQueue queue;
    std::mutex cout_mutex;

    std::thread producer([&] {
        for (int value = 1; value <= 10; ++value) {
            queue.push(value);
        }
        queue.close();
    });

    std::vector<std::thread> consumers;
    for (int id = 0; id < 2; ++id) {
        consumers.emplace_back([&, id] {
            while (auto value = queue.pop()) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "consumer " << id << " got " << *value << "\n";
            }
        });
    }

    producer.join();
    for (auto& consumer : consumers) {
        consumer.join();
    }
}

