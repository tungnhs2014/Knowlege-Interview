#include <stddef.h>
#include <stdio.h>

enum {
    ROWS = 2,
    COLUMNS = 3
};

static void print_contiguous(size_t rows,
                             int matrix[][COLUMNS])
{
    for (size_t row = 0U; row < rows; ++row) {
        for (size_t column = 0U; column < COLUMNS; ++column) {
            printf("%d%c", matrix[row][column],
                   column + 1U == COLUMNS ? '\n' : ' ');
        }
    }
}

static void print_pointer_array(int * const rows[], size_t row_count)
{
    for (size_t row = 0U; row < row_count; ++row) {
        printf("row[%zu][0]=%d\n", row, rows[row][0]);
    }
}

int main(void)
{
    int contiguous[ROWS][COLUMNS] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    int first[] = {10, 20};
    int second[] = {30, 40, 50};
    int *ragged[] = {first, second};

    int (*row_ptr)[COLUMNS] = contiguous;

    print_contiguous(ROWS, row_ptr);
    print_pointer_array(ragged, 2U);

    printf("next-row-first=%d\n", (*(row_ptr + 1))[0]);
    return 0;
}
