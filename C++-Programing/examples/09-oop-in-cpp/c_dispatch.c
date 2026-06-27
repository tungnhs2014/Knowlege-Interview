#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum SensorStatus {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT,
    SENSOR_READ_FAILED
};

struct SensorOps {
    unsigned int version;
    enum SensorStatus (*read_millivolts)(
        const void *context,
        int *out_value);
};

struct Sensor {
    const void *context;
    const struct SensorOps *ops;
};

enum SensorStatus sensor_read_millivolts(
    const struct Sensor *sensor,
    int *out_value)
{
    if (sensor == NULL
        || sensor->ops == NULL
        || sensor->ops->version != 1U
        || sensor->ops->read_millivolts == NULL
        || out_value == NULL) {
        return SENSOR_INVALID_ARGUMENT;
    }

    return sensor->ops->read_millivolts(sensor->context, out_value);
}

struct FixedSensor {
    int value;
};

enum SensorStatus fixed_sensor_read(
    const void *context,
    int *out_value)
{
    if (context == NULL || out_value == NULL) {
        return SENSOR_INVALID_ARGUMENT;
    }

    const struct FixedSensor *fixed = context;
    *out_value = fixed->value;
    return SENSOR_OK;
}

static const struct SensorOps fixed_sensor_ops = {
    .version = 1U,
    .read_millivolts = fixed_sensor_read
};

int main(void)
{
    const struct FixedSensor implementation = {.value = 2700};
    const struct Sensor sensor = {
        .context = &implementation,
        .ops = &fixed_sensor_ops
    };

    int value = 0;
    const bool normal_case =
        sensor_read_millivolts(&sensor, &value) == SENSOR_OK
        && value == 2700;
    const bool null_rejected =
        sensor_read_millivolts(NULL, &value) == SENSOR_INVALID_ARGUMENT
        && sensor_read_millivolts(&sensor, NULL) == SENSOR_INVALID_ARGUMENT;
    const bool passed = normal_case && null_rejected;

    printf(
        "value=%d null-rejected=%s result=%s\n",
        value,
        null_rejected ? "true" : "false",
        passed ? "passed" : "failed");

    return passed ? 0 : 1;
}
