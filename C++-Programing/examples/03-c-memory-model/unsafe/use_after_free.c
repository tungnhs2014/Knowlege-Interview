#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *value = malloc(sizeof *value);

    if (value == NULL) {
        return EXIT_FAILURE;
    }

    *value = 42;
    free(value);

    /* Intentional undefined behavior for an isolated ASan demonstration. */
    printf("%d\n", *value);
    return EXIT_SUCCESS;
}
