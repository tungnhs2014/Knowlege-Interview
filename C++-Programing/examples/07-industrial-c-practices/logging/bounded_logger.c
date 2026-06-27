#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
} LogLevel;

typedef enum {
    LOG_EVENT_SENSOR_TIMEOUT = 1001,
    LOG_EVENT_STATUS_READ = 1002
} LogEvent;

typedef enum {
    LOG_OK = 0,
    LOG_INVALID_ARGUMENT,
    LOG_FORMAT_ERROR,
    LOG_TRUNCATED
} LogStatus;

static LogStatus format_log(
    char *destination,
    size_t capacity,
    LogLevel level,
    LogEvent event,
    const char *format,
    ...)
{
    if (destination == NULL
        || capacity == 0U
        || format == NULL) {
        return LOG_INVALID_ARGUMENT;
    }

    int prefix_length = snprintf(
        destination,
        capacity,
        "level=%d event=%d ",
        (int)level,
        (int)event);
    if (prefix_length < 0) {
        destination[0] = '\0';
        return LOG_FORMAT_ERROR;
    }
    if ((size_t)prefix_length >= capacity) {
        return LOG_TRUNCATED;
    }

    size_t used = (size_t)prefix_length;
    va_list arguments;
    va_start(arguments, format);
    int message_length = vsnprintf(
        destination + used,
        capacity - used,
        format,
        arguments);
    va_end(arguments);

    if (message_length < 0) {
        destination[0] = '\0';
        return LOG_FORMAT_ERROR;
    }
    if ((size_t)message_length >= capacity - used) {
        return LOG_TRUNCATED;
    }
    return LOG_OK;
}

static int read_status(unsigned int *calls)
{
    ++(*calls);
    return 7;
}

int main(void)
{
    char complete[64];
    char truncated[24];
    unsigned int status_reads = 0U;

    int status = read_status(&status_reads);
    LogStatus complete_result = format_log(
        complete,
        sizeof complete,
        LOG_DEBUG,
        LOG_EVENT_STATUS_READ,
        "status=%d",
        status);
    LogStatus truncated_result = format_log(
        truncated,
        sizeof truncated,
        LOG_ERROR,
        LOG_EVENT_SENSOR_TIMEOUT,
        "sensor=%u timeout-ms=%u",
        12U,
        5000U);

    bool passed = complete_result == LOG_OK
        && truncated_result == LOG_TRUNCATED
        && status_reads == 1U;

    printf("%s\n", complete);
    printf(
        "truncation=%s side-effect-count=%u result=%s\n",
        truncated_result == LOG_TRUNCATED ? "reported" : "hidden",
        status_reads,
        passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
