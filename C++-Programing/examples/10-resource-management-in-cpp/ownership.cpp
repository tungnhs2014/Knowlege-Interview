#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

class Sensor {
public:
    explicit Sensor(int value) noexcept
        : value_{value}
    {
    }

    int read() const noexcept
    {
        return value_;
    }

private:
    int value_;
};

class Device {
public:
    explicit Device(std::unique_ptr<Sensor> sensor)
        : sensor_{std::move(sensor)}
    {
        if (!sensor_) {
            throw std::invalid_argument{"Device requires a sensor"};
        }
    }

    int read() const noexcept
    {
        return sensor_->read();
    }

private:
    std::unique_ptr<Sensor> sensor_;
};

int inspect(const Sensor& sensor) noexcept
{
    return sensor.read();
}

int main()
{
    auto owner = std::make_unique<Sensor>(2700);
    const int borrowed_reading = inspect(*owner);

    Device device{std::move(owner)};
    const bool ownership_transferred = owner == nullptr;
    bool null_rejected = false;
    try {
        Device invalid{std::unique_ptr<Sensor>{}};
    } catch (const std::invalid_argument&) {
        null_rejected = true;
    }
    const bool passed =
        borrowed_reading == 2700
        && ownership_transferred
        && null_rejected
        && device.read() == 2700;

    std::cout
        << std::boolalpha
        << "ownership-transferred=" << ownership_transferred
        << " null-rejected=" << null_rejected
        << " value=" << device.read()
        << " result=" << (passed ? "passed" : "failed")
        << '\n';
    return passed ? 0 : 1;
}
