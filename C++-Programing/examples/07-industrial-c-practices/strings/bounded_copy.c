#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static bool copy_c_string(
    char *destination,
    size_t capacity,
    const char *source)
{
    if (destination == NULL || source == NULL || capacity == 0U) {
        return false;
    }

    size_t index = 0U;
    while (source[index] != '\0' && index + 1U < capacity) {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
    return source[index] == '\0';
}

static bool expect_copy(
    const char *source,
    size_t capacity,
    const char *expected,
    bool expected_complete)
{
    char destination[16] = "unchanged";

    if (capacity > sizeof destination) {
        return false;
    }

    bool complete = copy_c_string(destination, capacity, source);
    return complete == expected_complete
        && strcmp(destination, expected) == 0;
}

int main(void)
{
    bool passed = true;

    passed = passed && expect_copy("", 1U, "", true);
    passed = passed && expect_copy("abc", 4U, "abc", true);
    passed = passed && expect_copy("abcd", 4U, "abc", false);
    passed = passed && expect_copy("sensor", 16U, "sensor", true);
    passed = passed && !copy_c_string(NULL, 4U, "abc");
    passed = passed && !copy_c_string((char[1]){'\0'}, 0U, "abc");

    printf("bounded-copy=%s\n", passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
