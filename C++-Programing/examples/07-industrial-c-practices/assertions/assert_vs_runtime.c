#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static int first_value_nonempty(
    const int *values,
    size_t count)
{
    assert(values != NULL);
    assert(count > 0U);
    (void)count;
    return values[0];
}

static bool try_first_value(
    const int *values,
    size_t count,
    int *out_value)
{
    if (values == NULL || out_value == NULL || count == 0U) {
        return false;
    }

    *out_value = first_value_nonempty(values, count);
    return true;
}

int main(void)
{
    const int values[] = {42, 7};
    int first = 0;

    bool valid_accepted = try_first_value(
        values,
        sizeof values / sizeof values[0],
        &first);
    bool invalid_rejected = !try_first_value(NULL, 0U, &first);
    bool passed = valid_accepted
        && invalid_rejected
        && first == 42;

    printf(
        "first=%d invalid-input=%s result=%s\n",
        first,
        invalid_rejected ? "rejected" : "accepted",
        passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
