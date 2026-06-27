#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*value_callback)(int value, void *context);

struct Sum {
    intmax_t total;
    int overflow;
};

static int create_values(size_t count, int **out_values)
{
    if (out_values == NULL) {
        return 0;
    }

    *out_values = NULL;

    if (count == 0U
        || count > SIZE_MAX / sizeof **out_values
        || (uintmax_t)count > (uintmax_t)INT_MAX) {
        return 0;
    }

    int *values = malloc(count * sizeof *values);
    if (values == NULL) {
        return 0;
    }

    for (size_t index = 0U; index < count; ++index) {
        values[index] = (int)(index + 1U);
    }

    *out_values = values;
    return 1;
}

static int for_each_value(const int *values,
                          size_t count,
                          value_callback callback,
                          void *context)
{
    if (callback == NULL || (values == NULL && count != 0U)) {
        return 0;
    }

    for (size_t index = 0U; index < count; ++index) {
        callback(values[index], context);
    }

    return 1;
}

static void add_to_sum(int value, void *context)
{
    struct Sum *sum = context;

    if (sum != NULL && !sum->overflow) {
        if (value > 0 && sum->total > INTMAX_MAX - value) {
            sum->overflow = 1;
            return;
        }

        sum->total += value;
    }
}

int main(void)
{
    int *values = NULL;
    const size_t count = 5U;
    struct Sum sum = {0};

    if (!create_values(count, &values)) {
        fputs("allocation failed\n", stderr);
        return EXIT_FAILURE;
    }

    if (!for_each_value(values, count, add_to_sum, &sum)) {
        free(values);
        return EXIT_FAILURE;
    }

    if (sum.overflow) {
        fputs("sum overflow\n", stderr);
        free(values);
        return EXIT_FAILURE;
    }

    printf("sum=%" PRIdMAX "\n", sum.total);
    free(values);
    values = NULL;
    return EXIT_SUCCESS;
}
