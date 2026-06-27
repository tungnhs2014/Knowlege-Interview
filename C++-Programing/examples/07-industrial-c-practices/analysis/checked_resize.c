#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool resize_ints(
    int **items,
    size_t count)
{
    if (items == NULL || count > SIZE_MAX / sizeof **items) {
        return false;
    }

    if (count == 0U) {
        free(*items);
        *items = NULL;
        return true;
    }

    void *temporary = realloc(*items, count * sizeof **items);
    if (temporary == NULL) {
        return false;
    }

    *items = temporary;
    return true;
}

int main(void)
{
    int *items = NULL;

    if (!resize_ints(&items, 4U)) {
        return 1;
    }

    for (size_t index = 0U; index < 4U; ++index) {
        items[index] = (int)(index + 1U);
    }

    bool overflow_rejected = !resize_ints(
        &items,
        SIZE_MAX / sizeof *items + 1U);
    bool values_preserved = items[0] == 1 && items[3] == 4;
    bool released = resize_ints(&items, 0U) && items == NULL;
    bool passed = overflow_rejected && values_preserved && released;

    printf(
        "overflow=%s ownership-release=%s result=%s\n",
        overflow_rejected ? "rejected" : "accepted",
        released ? "passed" : "failed",
        passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
