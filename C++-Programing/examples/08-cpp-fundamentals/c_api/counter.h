#ifndef CHAPTER08_COUNTER_H
#define CHAPTER08_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CounterHandle CounterHandle;

enum CounterStatus {
    COUNTER_OK = 0,
    COUNTER_INVALID_ARGUMENT,
    COUNTER_ALLOCATION_FAILED,
    COUNTER_OVERFLOW
};

int counter_create(int initial, CounterHandle **out_handle);
int counter_increment(CounterHandle *handle);
int counter_value(const CounterHandle *handle, int *out_value);
void counter_destroy(CounterHandle *handle);

#ifdef __cplusplus
}
#endif

#endif
