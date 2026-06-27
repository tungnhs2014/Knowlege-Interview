#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    FORMAT_OK,
    FORMAT_TRUNCATED,
    FORMAT_ERROR
} FormatResult;

static FormatResult format_sensor(char *destination,
                                  size_t capacity,
                                  unsigned int sensor_id,
                                  int value)
{
    if (destination == NULL || capacity == 0U) {
        return FORMAT_ERROR;
    }

    destination[0] = '\0';

    const int written = snprintf(destination,
                                 capacity,
                                 "sensor=%u value=%d",
                                 sensor_id,
                                 value);

    if (written < 0) {
        destination[0] = '\0';
        return FORMAT_ERROR;
    }

    if ((size_t)written >= capacity) {
        return FORMAT_TRUNCATED;
    }

    return FORMAT_OK;
}

static const char *format_result_name(FormatResult result)
{
    switch (result) {
    case FORMAT_OK:
        return "ok";
    case FORMAT_TRUNCATED:
        return "truncated";
    case FORMAT_ERROR:
        return "error";
    default:
        return "invalid";
    }
}

int main(int argc, char *argv[])
{
    char complete[32];
    char input[32];
    const size_t small_capacity = argc > 1 ? 32U : 8U;
    char small[small_capacity];

    (void)argv;

    const FormatResult complete_result =
        format_sensor(complete, sizeof complete, 7U, 42);
    const FormatResult small_result =
        format_sensor(small, sizeof small, 7U, 42);

    printf("complete: %s: %s\n",
           format_result_name(complete_result),
           complete);
    printf("small: %s: %s\n",
           format_result_name(small_result),
           small);

    if (fgets(input, sizeof input, stdin) != NULL) {
        const int complete_line = strchr(input, '\n') != NULL || feof(stdin);
        input[strcspn(input, "\n")] = '\0';
        printf("input-complete=%s length=%zu text=%s\n",
               complete_line ? "yes" : "no",
               strlen(input),
               input);
    }

    return 0;
}
