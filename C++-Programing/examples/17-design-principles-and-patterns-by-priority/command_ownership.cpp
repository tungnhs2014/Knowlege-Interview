#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class CommandQueue {
public:
    void push(std::function<void()> command) {
        commands_.push_back(std::move(command));
    }

    void run_all() {
        for (const auto& command : commands_) {
            command();
        }
        commands_.clear();
    }

private:
    std::vector<std::function<void()>> commands_;
};

class DeviceConfig {
public:
    void set_mode(std::string mode) {
        mode_ = std::move(mode);
    }

    const std::string& mode() const {
        return mode_;
    }

private:
    std::string mode_{"idle"};
};

int main() {
    DeviceConfig config;
    CommandQueue queue;

    // Safe: command data is captured by value. The config reference is safe
    // because queue.run_all() executes before config goes out of scope.
    queue.push([&config, mode = std::string("active")] {
        config.set_mode(mode);
    });

    queue.push([&config, mode = std::string("diagnostic")] {
        config.set_mode(mode);
    });

    queue.run_all();
    std::cout << "mode: " << config.mode() << '\n';

    // Do not store commands with reference captures if the queue may outlive
    // the referenced object. Use value capture or explicit shared ownership.
}
