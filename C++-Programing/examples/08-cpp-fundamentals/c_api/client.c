#include "counter.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

int main(void)
{
    CounterHandle *handle = NULL;
    int value = 0;

    bool normal_case_passed = counter_create(4, &handle) == COUNTER_OK
        && counter_increment(handle) == COUNTER_OK
        && counter_value(handle, &value) == COUNTER_OK
        && value == 5;

    counter_destroy(handle);
    handle = NULL;

    int maximum_value = 0;
    bool overflow_case_passed =
        counter_create(INT_MAX, &handle) == COUNTER_OK
        && counter_increment(handle) == COUNTER_OVERFLOW
        && counter_value(handle, &maximum_value) == COUNTER_OK
        && maximum_value == INT_MAX;

    counter_destroy(handle);

    bool passed = normal_case_passed && overflow_case_passed;

    printf(
        "counter-value=%d overflow-rejected=%s result=%s\n",
        value,
        overflow_case_passed ? "true" : "false",
        passed ? "passed" : "failed");

    return passed ? 0 : 1;
}
