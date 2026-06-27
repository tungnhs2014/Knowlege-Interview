#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *owner = malloc(sizeof *owner);
    if (owner == NULL) {
        return EXIT_FAILURE;
    }

    *owner = 42;
    int *alias = owner;

    free(owner);
    owner = NULL;

    printf("%d\n", *alias); /* intentional heap use-after-free */
    return EXIT_SUCCESS;
}
