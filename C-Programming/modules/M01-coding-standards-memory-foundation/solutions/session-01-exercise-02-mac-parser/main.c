/*
 * Session 01 / Exercise 2 — one reviewed implementation path.
 *
 * The parser accepts only the two explicitly documented six-field forms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PARSE_MAC_SUCCESS ((int8_t)0)
#define PARSE_MAC_INVALID_INPUT ((int8_t)-1)
#define MAC_BYTE_COUNT (6U)
#define MAC_SENTINEL UINT8_C(0xA5)

static bool hex_digit_to_value(char character, uint8_t *p_value)
{
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
 * @brief Convert a six-byte MAC address from delimited hexadecimal text.
 *
 * @param[in] mac_str Null-terminated text in the `HH:HH:HH:HH:HH:HH` or
 *                    `HH-HH-HH-HH-HH-HH` form.
 * @param[out] p_mac_out Caller storage for exactly six output bytes.
 * @return `0` on success; a negative value for null, malformed, mixed-
 *         delimiter, or invalid-hex input.
 *
 * On failure, this function leaves every byte of `p_mac_out` unchanged when
 * the output pointer is non-null. It uses temporary storage so no partially
 * parsed address can be committed to the caller.
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

    /* Commit only after all six bounded fields have been validated. */
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

static bool check_success_case(const char *p_input,
                               const uint8_t *p_expected,
                               const char *p_case_name)
{
    uint8_t actual_mac[MAC_BYTE_COUNT] = {0U};

    if ((parse_mac(p_input, actual_mac) != PARSE_MAC_SUCCESS) ||
        !mac_values_match(actual_mac, p_expected))
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_case_name);
        return false;
    }

    return true;
}

static bool check_failure_preserves_output(const char *p_input,
                                           const char *p_case_name)
{
    const uint8_t expected_sentinel[MAC_BYTE_COUNT] = {
        MAC_SENTINEL, MAC_SENTINEL, MAC_SENTINEL,
        MAC_SENTINEL, MAC_SENTINEL, MAC_SENTINEL
    };
    uint8_t actual_mac[MAC_BYTE_COUNT] = {
        MAC_SENTINEL, MAC_SENTINEL, MAC_SENTINEL,
        MAC_SENTINEL, MAC_SENTINEL, MAC_SENTINEL
    };

    if ((parse_mac(p_input, actual_mac) >= PARSE_MAC_SUCCESS) ||
        !mac_values_match(actual_mac, expected_sentinel))
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_case_name);
        return false;
    }

    return true;
}

static bool check_null_output(void)
{
    if (parse_mac("00:1A:2B:3C:4D:5E", NULL) >= PARSE_MAC_SUCCESS)
    {
        (void)fputs("FAIL: null output pointer\n", stderr);
        return false;
    }

    return true;
}

int main(void)
{
    static const uint8_t source_expected[MAC_BYTE_COUNT] = {
        UINT8_C(0x00), UINT8_C(0x1A), UINT8_C(0x2B),
        UINT8_C(0x3C), UINT8_C(0x4D), UINT8_C(0x5E)
    };
    static const uint8_t zero_expected[MAC_BYTE_COUNT] = {0U};
    static const uint8_t ff_expected[MAC_BYTE_COUNT] = {
        UINT8_MAX, UINT8_MAX, UINT8_MAX,
        UINT8_MAX, UINT8_MAX, UINT8_MAX
    };
    bool all_tests_passed = true;

    all_tests_passed = check_success_case("00:1A:2B:3C:4D:5E", source_expected,
                                           "source colon form") && all_tests_passed;
    all_tests_passed = check_success_case("00-1a-2b-3c-4d-5e", source_expected,
                                           "source hyphen form") && all_tests_passed;
    all_tests_passed = check_success_case("00:00:00:00:00:00", zero_expected,
                                           "all-zero address") && all_tests_passed;
    all_tests_passed = check_success_case("FF:fF:Ff:ff:FF:fF", ff_expected,
                                           "mixed hexadecimal case") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A:2B:3C:4D",
                                                       "source too few fields") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A:2B:3C:4D:5E:6F",
                                                       "source too many fields") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A:2B:3C:4D:5G",
                                                       "source invalid hexadecimal") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("", "empty input") &&
                       all_tests_passed;
    all_tests_passed = check_failure_preserves_output("0:1A:2B:3C:4D:5E",
                                                       "one-digit field") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("000:1A:2B:3C:4D:5E",
                                                       "three-digit field") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A-2B:3C:4D:5E",
                                                       "mixed delimiters") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("0x00:1A:2B:3C:4D:5E",
                                                       "prefix") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output(" 00:1A:2B:3C:4D:5E",
                                                       "leading whitespace") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A:2B:3C:4D:5E ",
                                                       "trailing whitespace") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("00:1A:2B:3C:4D:5Ex",
                                                       "trailing text") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output(NULL, "source null input") &&
                       all_tests_passed;
    all_tests_passed = check_null_output() && all_tests_passed;

    if (!all_tests_passed)
    {
        return EXIT_FAILURE;
    }

    (void)puts("PASS: MAC parser self-checks.");
    return EXIT_SUCCESS;
}
