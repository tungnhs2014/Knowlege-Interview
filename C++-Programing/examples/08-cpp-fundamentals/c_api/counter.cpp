#include "counter.h"

#include <limits>
#include <new>

struct CounterHandle {
    int value;
};

extern "C" int counter_create(
    int initial,
    CounterHandle **out_handle)
{
    if (out_handle == nullptr) {
        return COUNTER_INVALID_ARGUMENT;
    }

    *out_handle = nullptr;
    CounterHandle *handle =
        new (std::nothrow) CounterHandle{initial};
    if (handle == nullptr) {
        return COUNTER_ALLOCATION_FAILED;
    }

    *out_handle = handle;
    return COUNTER_OK;
}

extern "C" int counter_increment(CounterHandle *handle)
{
    if (handle == nullptr) {
        return COUNTER_INVALID_ARGUMENT;
    }

    if (handle->value == std::numeric_limits<int>::max()) {
        return COUNTER_OVERFLOW;
    }

    ++handle->value;
    return COUNTER_OK;
}

extern "C" int counter_value(
    const CounterHandle *handle,
    int *out_value)
{
    if (handle == nullptr || out_value == nullptr) {
        return COUNTER_INVALID_ARGUMENT;
    }

    *out_value = handle->value;
    return COUNTER_OK;
}

extern "C" void counter_destroy(CounterHandle *handle)
{
    delete handle;
}
