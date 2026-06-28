#include <algorithm>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

class Subject {
public:
    using Callback = std::function<void(int)>;

    int subscribe(Callback callback) {
        const int id = next_id_;
        ++next_id_;
        observers_.push_back(Observer{id, true, std::move(callback)});
        return id;
    }

    void unsubscribe(int id) {
        for (auto& observer : observers_) {
            if (observer.id == id) {
                observer.active = false;
            }
        }
    }

    void notify(int value) {
        for (const auto& observer : observers_) {
            if (observer.active) {
                observer.callback(value);
            }
        }

        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                           [](const Observer& observer) {
                               return !observer.active;
                           }),
            observers_.end());
    }

private:
    struct Observer {
        int id;
        bool active;
        Callback callback;
    };

    int next_id_{1};
    std::vector<Observer> observers_;
};

int main() {
    Subject subject;

    const int console_id = subject.subscribe([](int value) {
        std::cout << "console observer: " << value << '\n';
    });

    const int metrics_id = subject.subscribe([](int value) {
        std::cout << "metrics observer: " << value * 2 << '\n';
    });

    subject.notify(10);
    subject.unsubscribe(console_id);
    subject.notify(20);
    subject.unsubscribe(metrics_id);

    // Learning-only lifecycle model:
    // production code usually returns an RAII subscription token and defines
    // reentrancy/thread-safety policy. This version is not thread-safe.
}
