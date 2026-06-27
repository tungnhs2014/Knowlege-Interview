#include <stddef.h>
#include <stdio.h>

static void print_range(const int *begin, const int *end)
{
    while (begin != end) {
        printf("%d%c", *begin, begin + 1 == end ? '\n' : ' ');
        ++begin;
    }
}

int main(void)
{
    int values[4] = {10, 20, 30, 40};
    int *element_ptr = values;
    int (*array_ptr)[4] = &values;

    printf("first=%d third=%d\n", *element_ptr, (*array_ptr)[2]);
    printf("array-bytes=%zu pointer-bytes=%zu\n",
           sizeof values, sizeof element_ptr);

    print_range(values, values + 4);

    const int *pointer_to_const = values;
    int * const const_pointer = &values[0];
    const int * const const_pointer_to_const = &values[1];

    const_pointer[0] = 11;

    printf("read-only=%d fixed-target=%d fixed-read-only=%d\n",
           *pointer_to_const,
           *const_pointer,
           *const_pointer_to_const);

    return 0;
}
