#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static bool checked_add_int(int left, int right, int *result)
{
    if (result == NULL) {
        return false;
    }

    if ((right > 0 && left > INT_MAX - right) ||
        (right < 0 && left < INT_MIN - right)) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool index_is_valid(int index, size_t count)
{
    return index >= 0 && (size_t)index < count;
}

int main(void)
{
    const uint32_t packet_id = UINT32_C(4000000000);
    int sum = 0;

    printf("CHAR_BIT=%d\n", CHAR_BIT);
    printf("sizeof(int)=%zu\n", sizeof(int));
    printf("INT_MIN=%d, INT_MAX=%d\n", INT_MIN, INT_MAX);
    printf("packet_id=%" PRIu32 "\n", packet_id);

    if (!checked_add_int(40, 2, &sum)) {
        fputs("unexpected addition failure\n", stderr);
        return 1;
    }

    if (checked_add_int(INT_MAX, 1, &sum)) {
        fputs("overflow was not rejected\n", stderr);
        return 1;
    }

    printf("sum=%d\n", sum);
    printf("index -1 valid=%s\n",
           index_is_valid(-1, 4U) ? "true" : "false");
    printf("index 3 valid=%s\n",
           index_is_valid(3, 4U) ? "true" : "false");
    return 0;
}

