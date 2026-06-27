#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    OPEN_OK = 0,
    OPEN_INVALID_ARGUMENT,
    OPEN_NOT_FOUND,
    OPEN_PERMISSION,
    OPEN_OTHER
} OpenStatus;

typedef FILE *(*OpenFile)(const char *path, const char *mode);

static OpenStatus open_input(
    const char *path,
    FILE **out_file,
    int *out_platform_error,
    OpenFile open_file)
{
    if (path == NULL
        || out_file == NULL
        || out_platform_error == NULL
        || open_file == NULL) {
        return OPEN_INVALID_ARGUMENT;
    }

    FILE *file = open_file(path, "rb");
    if (file != NULL) {
        *out_file = file;
        *out_platform_error = 0;
        return OPEN_OK;
    }

    int saved_errno = errno;
    *out_platform_error = saved_errno;

    if (saved_errno == ENOENT) {
        return OPEN_NOT_FOUND;
    }
    if (saved_errno == EACCES) {
        return OPEN_PERMISSION;
    }
    return OPEN_OTHER;
}

static FILE *fail_not_found(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    errno = ENOENT;
    return NULL;
}

int main(void)
{
    FILE *file = NULL;
    int platform_error = 0;
    OpenStatus status = open_input(
        "input.bin",
        &file,
        &platform_error,
        fail_not_found);

    bool passed = status == OPEN_NOT_FOUND
        && file == NULL
        && platform_error == ENOENT;

    printf(
        "domain-status=%d platform-error=%d result=%s\n",
        (int)status,
        platform_error,
        passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
