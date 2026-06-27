#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct IntArray {
    int *data;
    size_t size;
    size_t capacity;
};

static void int_array_init(struct IntArray *array)
{
    array->data = NULL;
    array->size = 0U;
    array->capacity = 0U;
}

static void int_array_destroy(struct IntArray *array)
{
    if (array != NULL) {
        free(array->data);
        int_array_init(array);
    }
}

static int int_array_reserve(struct IntArray *array, size_t required)
{
    size_t new_capacity;
    int *new_data;

    if (array == NULL) {
        return 0;
    }

    if (required <= array->capacity) {
        return 1;
    }

    new_capacity = array->capacity == 0U ? 4U : array->capacity;

    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2U) {
            return 0;
        }
        new_capacity *= 2U;
    }

    if (new_capacity > SIZE_MAX / sizeof *array->data) {
        return 0;
    }

    new_data = realloc(array->data,
                       new_capacity * sizeof *array->data);
    if (new_data == NULL) {
        return 0;
    }

    array->data = new_data;
    array->capacity = new_capacity;
    return 1;
}

static int int_array_push(struct IntArray *array, int value)
{
    if (array == NULL || array->size == SIZE_MAX) {
        return 0;
    }

    if (!int_array_reserve(array, array->size + 1U)) {
        return 0;
    }

    array->data[array->size] = value;
    ++array->size;
    return 1;
}

int main(void)
{
    struct IntArray values;

    int_array_init(&values);

    for (int value = 10; value <= 50; value += 10) {
        if (!int_array_push(&values, value)) {
            fputs("array growth failed\n", stderr);
            int_array_destroy(&values);
            return EXIT_FAILURE;
        }
    }

    printf("size=%zu capacity=%zu values=", values.size, values.capacity);
    for (size_t index = 0U; index < values.size; ++index) {
        printf("%d%c", values.data[index],
               index + 1U == values.size ? '\n' : ' ');
    }

    int_array_destroy(&values);
    return EXIT_SUCCESS;
}
