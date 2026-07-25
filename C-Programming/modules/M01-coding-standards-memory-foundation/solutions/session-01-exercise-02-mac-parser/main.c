/*
 * Session 01 / Exercise 2 reference solution.
 *
 * This file demonstrates one reviewed implementation path for the approved
 * MAC parser exercise. Parsing remains local to this small program.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PARSE_MAC_SUCCESS ((int8_t)0)
#define PARSE_MAC_INVALID_INPUT ((int8_t)-1)
#define MAC_BYTE_COUNT (6U)

static bool hex_digit_to_value(char character, uint8_t *p_value)
{
    if (p_value == NULL)
    {
        return false;
    }

    if ((character >= '0') && (character <= '9'))
    {
        *p_value = (uint8_t)(character - '0');
        return true;
    }

    if ((character >= 'A') && (character <= 'F'))
    {
        *p_value = (uint8_t)((character - 'A') + 10);
        return true;
    }

    if ((character >= 'a') && (character <= 'f'))
    {
        *p_value = (uint8_t)((character - 'a') + 10);
        return true;
    }

    return false;
}

/**
 * @brief Parse a six-byte MAC address from colon- or hyphen-delimited text.
 *
 * @param[in] mac_str Null-terminated text containing six hexadecimal bytes.
 * @param[out] p_mac_out Caller-provided storage for exactly six bytes; it is
 *                       written only when parsing succeeds.
 * @return 0 on success; a negative value for null, malformed, or invalid
 *         input.
 */
int8_t parse_mac(const char *mac_str, uint8_t *p_mac_out)
{
    const char *p_current;
    uint8_t parsed_mac[MAC_BYTE_COUNT] = {0U};
    uint8_t byte_index;
    char delimiter = '\0';

    if ((mac_str == NULL) || (p_mac_out == NULL))
    {
        return PARSE_MAC_INVALID_INPUT;
    }

    p_current = mac_str;

    for (byte_index = 0U; byte_index < MAC_BYTE_COUNT; ++byte_index)
    {
        uint8_t high_nibble = 0U;
        uint8_t low_nibble = 0U;

        if (!hex_digit_to_value(*p_current, &high_nibble))
        {
            return PARSE_MAC_INVALID_INPUT;
        }

        ++p_current;

        if (!hex_digit_to_value(*p_current, &low_nibble))
        {
            return PARSE_MAC_INVALID_INPUT;
        }

        ++p_current;
        parsed_mac[byte_index] = (uint8_t)((high_nibble << 4U) | low_nibble);

        if (byte_index < (MAC_BYTE_COUNT - 1U))
        {
            if (byte_index == 0U)
            {
                delimiter = *p_current;

                if ((delimiter != ':') && (delimiter != '-'))
                {
                    return PARSE_MAC_INVALID_INPUT;
                }
            }
            else if (*p_current != delimiter)
            {
                return PARSE_MAC_INVALID_INPUT;
            }

            ++p_current;
        }
    }

    if (*p_current != '\0')
    {
        return PARSE_MAC_INVALID_INPUT;
    }

    for (byte_index = 0U; byte_index < MAC_BYTE_COUNT; ++byte_index)
    {
        p_mac_out[byte_index] = parsed_mac[byte_index];
    }

    return PARSE_MAC_SUCCESS;
}

static bool mac_values_match(const uint8_t *p_left, const uint8_t *p_right)
{
    uint8_t byte_index;

    for (byte_index = 0U; byte_index < MAC_BYTE_COUNT; ++byte_index)
    {
        if (p_left[byte_index] != p_right[byte_index])
        {
            return false;
        }
    }

    return true;
}

static int run_success_case(const char *p_input, const uint8_t *p_expected,
                            const char *p_case_name)
{
    uint8_t parsed_mac[MAC_BYTE_COUNT] = {0U};

    if ((parse_mac(p_input, parsed_mac) != PARSE_MAC_SUCCESS) ||
        !mac_values_match(parsed_mac, p_expected))
    {
        (void)fprintf(stderr, "FAILED: %s\n", p_case_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_failure_case(const char *p_input, const char *p_case_name)
{
    uint8_t parsed_mac[MAC_BYTE_COUNT] = {0U};

    if (parse_mac(p_input, parsed_mac) >= PARSE_MAC_SUCCESS)
    {
        (void)fprintf(stderr, "FAILED: %s\n", p_case_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_null_output_case(void)
{
    if (parse_mac("00:1A:2B:3C:4D:5E", NULL) >= PARSE_MAC_SUCCESS)
    {
        (void)fputs("FAILED: null output pointer\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(void)
{
    static const uint8_t expected_mac[MAC_BYTE_COUNT] = {
        UINT8_C(0x00), UINT8_C(0x1A), UINT8_C(0x2B),
        UINT8_C(0x3C), UINT8_C(0x4D), UINT8_C(0x5E)
    };
    int failures = 0;

    failures += run_success_case("00:1A:2B:3C:4D:5E", expected_mac,
                                 "colon-delimited MAC");
    failures += run_success_case("00-1a-2b-3c-4d-5e", expected_mac,
                                 "hyphen-delimited MAC");
    failures += run_failure_case("00:1A:2B:3C:4D", "five byte fields");
    failures += run_failure_case("00:1A:2B:3C:4D:5E:6F",
                                 "seven byte fields");
    failures += run_failure_case("00:1A:2B:3C:4D:5G", "invalid hex digit");
    failures += run_failure_case("0:1A:2B:3C:4D:5E", "missing hex digit");
    failures += run_failure_case("00:1A-2B:3C:4D:5E", "mixed delimiters");
    failures += run_failure_case(NULL, "null input pointer");
    failures += run_null_output_case();

    if (failures != 0)
    {
        return EXIT_FAILURE;
    }

    (void)puts("MAC parser tests passed.");
    return EXIT_SUCCESS;
}
