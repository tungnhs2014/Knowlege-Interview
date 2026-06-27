#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

void read_sensor(std::promise<int> result) {
    try {
        throw std::runtime_error("sensor timeout");
    } catch (...) {
        result.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> result;
    std::future<int> value = result.get_future();

    std::thread worker(read_sensor, std::move(result));

    try {
        int sensor_value = value.get();
        std::cout << "sensor value = " << sensor_value << "\n";
    } catch (const std::exception& error) {
        std::cout << "worker failed: " << error.what() << "\n";
    }

    worker.join();
}
