#include <stdlib.h>

int main(void)
{
    const size_t count = 4U;
    int *values = malloc(count * sizeof *values);
    volatile int *observed = values;

    if (values == NULL) {
        return EXIT_FAILURE;
    }

    /*
     * Intentional undefined behavior for an isolated ASan demonstration.
     * volatile keeps the invalid store observable; it does not make it valid.
     */
    observed[count] = 99;

    free(values);
    return EXIT_SUCCESS;
}
