#include "status.h"

#include <stdio.h>

int main(void)
{
    unsigned int first_sequence = 0U;
    unsigned int second_sequence = 0U;

    set_system_status(-5);
    printf("normalized status=%d\n", get_system_status());

    set_system_status(7);
    printf("status=%d\n", system_status);

    first_sequence = next_sequence();
    second_sequence = next_sequence();
    printf("sequence=%u,%u\n", first_sequence, second_sequence);
    return 0;
}
