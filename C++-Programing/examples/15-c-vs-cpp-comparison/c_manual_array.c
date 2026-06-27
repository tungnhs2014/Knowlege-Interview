#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int* data;
    size_t size;
} IntArray;

typedef void (*value_callback_t)(int value, void* user_data);

static int int_array_create(IntArray* out, size_t size) {
    if (out == NULL) {
        return -1;
    }

    out->data = NULL;
    out->size = 0;

    if (size == 0) {
        return 0;
    }

    out->data = malloc(size * sizeof(out->data[0]));
    if (out->data == NULL) {
        return -1;
    }

    out->size = size;
    return 0;
}

static void int_array_destroy(IntArray* array) {
    if (array == NULL) {
        return;
    }

    free(array->data);
    array->data = NULL;
    array->size = 0;
}

static void int_array_for_each(const IntArray* array,
                               value_callback_t callback,
                               void* user_data) {
    if (array == NULL || callback == NULL) {
        return;
    }

    for (size_t i = 0; i < array->size; ++i) {
        callback(array->data[i], user_data);
    }
}

static void add_to_sum(int value, void* user_data) {
    int* sum = user_data;
    if (sum != NULL) {
        *sum += value;
    }
}

int main(void) {
    IntArray values;

    if (int_array_create(&values, 4) != 0) {
        fputs("allocation failed\n", stderr);
        return 1;
    }

    for (size_t i = 0; i < values.size; ++i) {
        values.data[i] = (int)(i + 1U);
    }

    int sum = 0;
    int_array_for_each(&values, add_to_sum, &sum);

    printf("C manual array sum: %d\n", sum);

    int_array_destroy(&values);
    return 0;
}

