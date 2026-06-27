#include <stddef.h>
#include <stdio.h>

enum {
    ROWS = 2,
    COLUMNS = 3
};

static int sum(const int *values, size_t count)
{
    int total = 0;

    for (size_t index = 0U; index < count; ++index) {
        total += values[index];
    }

    return total;
}

static void print_matrix(size_t rows,
                         int matrix[][COLUMNS])
{
    for (size_t row = 0U; row < rows; ++row) {
        for (size_t column = 0U; column < COLUMNS; ++column) {
            printf("%d%c",
                   matrix[row][column],
                   column + 1U == COLUMNS ? '\n' : ' ');
        }
    }
}

int main(void)
{
    int values[] = {2, 4, 6, 8};
    int matrix[ROWS][COLUMNS] = {
        {1, 2, 3},
        {4, 5, 6}
    };
    const size_t count = sizeof values / sizeof values[0];
    int *element_pointer = values;
    int (*array_pointer)[4] = &values;

    printf("count=%zu sum=%d\n", count, sum(values, count));
    printf("array-bytes=%zu pointer-bytes=%zu\n",
           sizeof values,
           sizeof element_pointer);
    printf("second=%d whole-array-third=%d\n",
           element_pointer[1],
           (*array_pointer)[2]);
    print_matrix(ROWS, matrix);

    return 0;
}
