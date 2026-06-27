#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum command {
    COMMAND_STOP,
    COMMAND_START,
    COMMAND_RESET
};

static const char *command_name(enum command command)
{
    switch (command) {
        case COMMAND_STOP:
            return "stop";
        case COMMAND_START:
            return "start";
        case COMMAND_RESET:
            return "reset";
        default:
            return "invalid";
    }
}

static bool set_bit(unsigned int *value, unsigned int bit)
{
    const unsigned int width =
        (unsigned int)(sizeof(*value) * CHAR_BIT);

    if (value == NULL || bit >= width) {
        return false;
    }

    *value |= 1U << bit;
    return true;
}

static void print_reverse(const int values[], size_t count)
{
    bool first_output = true;

    for (size_t index = count; index-- > 0U;) {
        if (values[index] < 0) {
            continue;
        }

        if (!first_output) {
            putchar(',');
        }

        printf("%d", values[index]);
        first_output = false;
    }

    putchar('\n');
}

int main(void)
{
    const int values[] = {1, -1, 3, 4};
    const int trailing_negative[] = {-1, 2};
    unsigned int flags = 0U;

    printf("command=%s\n", command_name(COMMAND_START));
    print_reverse(values, sizeof(values) / sizeof(values[0]));
    print_reverse(
        trailing_negative,
        sizeof(trailing_negative) / sizeof(trailing_negative[0]));

    if (!set_bit(&flags, 2U)) {
        return 1;
    }

    if (set_bit(&flags, (unsigned int)(sizeof(flags) * CHAR_BIT))) {
        fputs("invalid shift count was accepted\n", stderr);
        return 1;
    }

    printf("flags=%u\n", flags);
    return 0;
}
