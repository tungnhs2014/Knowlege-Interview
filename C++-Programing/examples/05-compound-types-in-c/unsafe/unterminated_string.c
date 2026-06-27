#include <stdio.h>
#include <string.h>

int main(void)
{
    char code[4] = {'O', 'K', 'A', 'Y'};
    volatile size_t observed = strlen(code);

    printf("length=%zu\n", observed);
    return 0;
}
