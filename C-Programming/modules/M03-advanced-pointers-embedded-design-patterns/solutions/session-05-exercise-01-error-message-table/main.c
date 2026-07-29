#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Identifies the supported error-message entries.
 */
typedef enum
{
    ERR_OK = 0,
    ERR_TIMEOUT,
    ERR_HW_FAIL,
    ERR_COUNT
} error_code_t;

/**
 * @brief Maps valid error codes to immutable diagnostic text.
 */
const char *const g_error_strings[ERR_COUNT] = {
    "OK",
    "TIMEOUT_ERROR",
    "HARDWARE_FAILURE"
};

/**
 * @brief Obtains text for an error code without indexing outside the table.
 *
 * @param[in] err_code Error-code value to translate.
 * @return Matching immutable text, or "UNKNOWN_ERROR" for an invalid value.
 */
const char *get_error_string(uint8_t err_code)
{
    if (err_code >= (uint8_t)ERR_COUNT)
    {
        return "UNKNOWN_ERROR";
    }

    return g_error_strings[err_code];
}

int main(void)
{
    const char *p_timeout = get_error_string((uint8_t)ERR_TIMEOUT);
    const char *p_invalid = get_error_string(99U);

    if ((strcmp(p_timeout, "TIMEOUT_ERROR") != 0) ||
        (strcmp(p_invalid, "UNKNOWN_ERROR") != 0))
    {
        return EXIT_FAILURE;
    }

    (void)printf("Error code %u: %s\n", (unsigned int)ERR_TIMEOUT, p_timeout);
    (void)printf("Error code 99: %s\n", p_invalid);

    return EXIT_SUCCESS;
}
