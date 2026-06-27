#include <functional>
#include <iostream>
#include <memory>
#include <vector>

class Dispatcher {
public:
    void add(std::function<void(int)> callback) {
        callbacks_.push_back(std::move(callback));
    }

    void emit(int value) const {
        for (const auto& callback : callbacks_) {
            callback(value);
        }
    }

private:
    std::vector<std::function<void(int)>> callbacks_;
};

class Client : public std::enable_shared_from_this<Client> {
public:
    void subscribe(Dispatcher& dispatcher) {
        std::weak_ptr<Client> self = shared_from_this();
        dispatcher.add([self](int value) {
            if (auto locked = self.lock()) {
                locked->total_ += value;
            }
        });
    }

    int total() const {
        return total_;
    }

private:
    int total_ = 0;
};

int main() {
    Dispatcher dispatcher;
    std::weak_ptr<Client> observed;
    int total_before_destroy = 0;

    {
        auto client = std::make_shared<Client>();
        observed = client;
        client->subscribe(dispatcher);

        dispatcher.emit(5);
        total_before_destroy = client->total();
    }

    dispatcher.emit(7);

    const bool ok = total_before_destroy == 5 && observed.expired();

    std::cout << "total_before_destroy=" << total_before_destroy
              << " expired=" << std::boolalpha << observed.expired()
              << " result=" << (ok ? "passed" : "failed") << '\n';

    return ok ? 0 : 1;
}
