#include <iostream>

enum class SensorStatus {
    ok,
    read_failed
};

struct SensorReading {
    SensorStatus status;
    int millivolts;
};

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};

class FixedSensor final : public Sensor {
public:
    explicit FixedSensor(int value)
        : value_{value}
    {
    }

    SensorReading read() const override
    {
        return {SensorStatus::ok, value_};
    }

private:
    int value_;
};

class FailingOnceSensor final : public Sensor {
public:
    explicit FailingOnceSensor(int value)
        : value_{value}
    {
    }

    SensorReading read() const override
    {
        ++calls_;
        if (calls_ == 1) {
            return {SensorStatus::read_failed, 0};
        }
        return {SensorStatus::ok, value_};
    }

    int calls() const
    {
        return calls_;
    }

private:
    int value_;
    mutable int calls_{0};
};

class FailingSensor final : public Sensor {
public:
    SensorReading read() const override
    {
        return {SensorStatus::read_failed, 0};
    }
};

class RetryingSensor final : public Sensor {
public:
    explicit RetryingSensor(const Sensor& inner)
        : inner_{inner}
    {
    }

    SensorReading read() const override
    {
        const SensorReading first = inner_.read();
        return first.status == SensorStatus::ok ? first : inner_.read();
    }

private:
    const Sensor& inner_;
};

enum class AlarmState {
    inactive,
    active,
    sensor_failure
};

class Alarm {
public:
    Alarm(const Sensor& sensor, int threshold)
        : sensor_{sensor},
          threshold_{threshold}
    {
    }

    AlarmState evaluate() const
    {
        const SensorReading reading = sensor_.read();
        if (reading.status != SensorStatus::ok) {
            return AlarmState::sensor_failure;
        }
        return reading.millivolts >= threshold_
            ? AlarmState::active
            : AlarmState::inactive;
    }

private:
    const Sensor& sensor_;
    int threshold_;
};

int main()
{
    FixedSensor fixed{2700};
    Alarm direct_alarm{fixed, 2500};

    FailingOnceSensor unreliable{2800};
    RetryingSensor retrying{unreliable};
    Alarm retry_alarm{retrying, 2500};

    FailingSensor failing;
    Alarm failing_alarm{failing, 2500};

    const bool direct_active =
        direct_alarm.evaluate() == AlarmState::active;
    const bool retry_active =
        retry_alarm.evaluate() == AlarmState::active;
    const bool failure_propagated =
        failing_alarm.evaluate() == AlarmState::sensor_failure;
    const bool passed =
        direct_active
        && retry_active
        && failure_propagated
        && unreliable.calls() == 2;

    std::cout
        << std::boolalpha
        << "direct-active=" << direct_active
        << " retry-active=" << retry_active
        << " failure-propagated=" << failure_propagated
        << " retry-calls=" << unreliable.calls()
        << " result=" << (passed ? "passed" : "failed")
        << '\n';

    return passed ? 0 : 1;
}
