#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parse_int(const char *text, int *value)
{
    char *end = NULL;
    long parsed = 0L;

    if (text == NULL || value == NULL) {
        return false;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);

    if (errno != 0 || end == text ||
        (*end != '\n' && *end != '\0') ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

int main(void)
{
    char buffer[64];
    int value = 0;

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        fputs("input error\n", stderr);
        return 1;
    }

    if (strchr(buffer, '\n') == NULL && !feof(stdin)) {
        fputs("input line too long\n", stderr);
        return 2;
    }

    if (!parse_int(buffer, &value)) {
        fputs("invalid integer\n", stderr);
        return 3;
    }

    printf("value=%d\n", value);
    return 0;
}
