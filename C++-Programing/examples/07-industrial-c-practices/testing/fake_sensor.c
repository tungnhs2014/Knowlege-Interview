#include <stdbool.h>
#include <stdio.h>

typedef bool (*ReadSensor)(
    void *context,
    int *out_value);

typedef struct {
    ReadSensor read;
    void *context;
} Sensor;

typedef struct {
    int value;
    bool succeeds;
    unsigned int calls;
} FakeSensor;

static bool alarm_required(
    const Sensor *sensor,
    bool *out_required)
{
    if (sensor == NULL
        || sensor->read == NULL
        || out_required == NULL) {
        return false;
    }

    int value = 0;
    if (!sensor->read(sensor->context, &value)) {
        return false;
    }

    *out_required = value > 100;
    return true;
}

static bool fake_sensor_read(
    void *context,
    int *out_value)
{
    if (context == NULL || out_value == NULL) {
        return false;
    }

    FakeSensor *fake = context;
    ++fake->calls;

    if (!fake->succeeds) {
        return false;
    }

    *out_value = fake->value;
    return true;
}

int main(void)
{
    FakeSensor fake = {
        .value = 101,
        .succeeds = true,
        .calls = 0U
    };
    Sensor sensor = {
        .read = fake_sensor_read,
        .context = &fake
    };
    bool alarm = false;

    bool high_value_passed = alarm_required(&sensor, &alarm)
        && alarm
        && fake.calls == 1U;

    fake.value = 100;
    bool boundary_passed = alarm_required(&sensor, &alarm)
        && !alarm
        && fake.calls == 2U;

    fake.succeeds = false;
    bool failure_passed = !alarm_required(&sensor, &alarm)
        && fake.calls == 3U;

    bool passed = high_value_passed
        && boundary_passed
        && failure_passed;

    printf(
        "calls=%u failure-injection=%s result=%s\n",
        fake.calls,
        failure_passed ? "passed" : "failed",
        passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
