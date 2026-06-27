#include <limits.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int value = INT_MAX;

    if (argc == 2 && strcmp(argv[1], "overflow") == 0) {
        value += 1;
    }

    printf("value=%d\n", value);
    return 0;
}
