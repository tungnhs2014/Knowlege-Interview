#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool (*read)(void *context, int *out_value);
    bool (*configure)(void *context, unsigned int rate_hz);
} SensorOps;

typedef struct {
    const SensorOps *ops;
    void *context;
} Sensor;

typedef struct {
    int next_value;
    unsigned int configured_rate;
    unsigned int reads;
} FakeSensor;

static bool sensor_read(const Sensor *sensor, int *out_value)
{
    if (sensor == NULL
        || sensor->ops == NULL
        || sensor->ops->read == NULL
        || out_value == NULL) {
        return false;
    }

    return sensor->ops->read(sensor->context, out_value);
}

static bool sensor_configure(
    const Sensor *sensor,
    unsigned int rate_hz)
{
    if (sensor == NULL
        || sensor->ops == NULL
        || sensor->ops->configure == NULL) {
        return false;
    }

    return sensor->ops->configure(sensor->context, rate_hz);
}

static bool fake_read(void *context, int *out_value)
{
    FakeSensor *fake = context;

    if (fake == NULL || out_value == NULL) {
        return false;
    }

    ++fake->reads;
    *out_value = fake->next_value;
    return true;
}

static bool fake_configure(void *context, unsigned int rate_hz)
{
    FakeSensor *fake = context;

    if (fake == NULL || rate_hz == 0U || rate_hz > 1000U) {
        return false;
    }

    fake->configured_rate = rate_hz;
    return true;
}

static const SensorOps fake_ops = {
    .read = fake_read,
    .configure = fake_configure
};

int main(void)
{
    FakeSensor fake = {37, 0U, 0U};
    Sensor sensor = {&fake_ops, &fake};
    int value;

    if (!sensor_configure(&sensor, 50U)
        || !sensor_read(&sensor, &value)) {
        return 1;
    }

    printf("rate=%u value=%d reads=%u\n",
           fake.configured_rate,
           value,
           fake.reads);
    printf("invalid-rate=%s\n",
           sensor_configure(&sensor, 0U) ? "accepted" : "rejected");
    return 0;
}
