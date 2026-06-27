#include <stdio.h>

#define MAX_UNSAFE(a, b) ((a) > (b) ? (a) : (b))

static unsigned int calls;

static int next_value(void)
{
    ++calls;
    return 10;
}

static inline int max_int(int left, int right)
{
    return left > right ? left : right;
}

int main(void)
{
    calls = 0U;
    int macro_result = MAX_UNSAFE(next_value(), 5);
    printf("macro-result=%d calls=%u\n", macro_result, calls);

    calls = 0U;
    int function_result = max_int(next_value(), 5);
    printf("function-result=%d calls=%u\n", function_result, calls);

    return calls == 1U && macro_result == function_result ? 0 : 1;
}
