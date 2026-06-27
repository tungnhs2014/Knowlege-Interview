#include <stdio.h>
#include <stdlib.h>

int global_zero;
int global_initialized = 7;
static const char read_only_label[] = "memory-layout";

static void print_addresses(void)
{
    int automatic = 11;
    int *allocated = malloc(sizeof *allocated);

    if (allocated == NULL) {
        fputs("allocation failed\n", stderr);
        return;
    }

    *allocated = 13;

    printf("&global_zero        = %p, value=%d\n",
           (void *)&global_zero, global_zero);
    printf("&global_initialized = %p, value=%d\n",
           (void *)&global_initialized, global_initialized);
    printf("read_only_label     = %p, value=%s\n",
           (void *)read_only_label, read_only_label);
    printf("&automatic          = %p, value=%d\n",
           (void *)&automatic, automatic);
    printf("allocated           = %p, value=%d\n",
           (void *)allocated, *allocated);

    free(allocated);
}

int main(void)
{
    print_addresses();
    return EXIT_SUCCESS;
}
