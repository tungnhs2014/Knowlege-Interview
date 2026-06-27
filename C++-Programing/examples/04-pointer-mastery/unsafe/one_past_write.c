#include <stddef.h>

int main(void)
{
    int values[4] = {0};
    int *one_past = values + 4;

    *one_past = 42; /* intentional one-past write */

    return values[0];
}
