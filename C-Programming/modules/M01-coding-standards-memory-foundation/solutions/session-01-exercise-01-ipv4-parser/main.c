/*
 * Session 01 / Exercise 1 — one reviewed implementation path.
 *
 * The parser intentionally stays local to this exercise rather than becoming
 * a general-purpose text-conversion framework.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PARSE_IPV4_SUCCESS ((int8_t)0)
#define PARSE_IPV4_INVALID_INPUT ((int8_t)-1)
#define IPV4_COMPONENT_COUNT (4U)
#define IPV4_MAX_COMPONENT_VALUE UINT32_C(255)
#define IPV4_DECIMAL_BASE UINT32_C(10)
#define IPV4_SENTINEL UINT32_C(0xA5A5A5A5)

static bool is_decimal_digit(char character)
{
    return (character >= '0') && (character <= '9');
}

/**
 * @brief Convert dotted-decimal IPv4 text to its documented numeric value.
 *
 * @param[in] ip_str Null-terminated ASCII text containing four decimal
 *                   components separated by three dots.
 * @param[out] p_ip_out Destination for the fully validated numeric result.
 * @return `0` on success; a negative value for a null, malformed, signed,
 *         out-of-range, or trailing-character input.
 *
 * On every failure, this function leaves `*p_ip_out` unchanged when the
 * output pointer is non-null. The result is the integer
 * `(A << 24) | (B << 16) | (C << 8) | D`; it is not derived from host memory
 * byte order.
 */
int8_t parse_ipv4(const char *ip_str, uint32_t *p_ip_out)
{
    const char *p_current;
    uint32_t parsed_address = UINT32_C(0);
    uint8_t component_index;

    if ((ip_str == NULL) || (p_ip_out == NULL))
    {
        return PARSE_IPV4_INVALID_INPUT;
    }

    p_current = ip_str;

    for (component_index = 0U;
         component_index < IPV4_COMPONENT_COUNT;
         ++component_index)
    {
        bool has_digit = false;
        uint32_t component_value = UINT32_C(0);

        while (is_decimal_digit(*p_current))
        {
            const uint32_t digit_value = (uint32_t)(*p_current - '0');

            /* Check before accumulation so the component cannot exceed 255. */
            if ((component_value >
                 (IPV4_MAX_COMPONENT_VALUE / IPV4_DECIMAL_BASE)) ||
                ((component_value ==
                  (IPV4_MAX_COMPONENT_VALUE / IPV4_DECIMAL_BASE)) &&
                 (digit_value >
                  (IPV4_MAX_COMPONENT_VALUE % IPV4_DECIMAL_BASE))))
            {
                return PARSE_IPV4_INVALID_INPUT;
            }

            component_value = (component_value * IPV4_DECIMAL_BASE) + digit_value;
            has_digit = true;
            ++p_current;
        }

        if (!has_digit)
        {
            return PARSE_IPV4_INVALID_INPUT;
        }

        /* All earlier components occupy at most 24 bits before this shift. */
        parsed_address = (parsed_address << 8U) | component_value;

        if (component_index < (IPV4_COMPONENT_COUNT - 1U))
        {
            if (*p_current != '.')
            {
                return PARSE_IPV4_INVALID_INPUT;
            }

            ++p_current;
        }
    }

    if (*p_current != '\0')
    {
        return PARSE_IPV4_INVALID_INPUT;
    }

    /* Commit only after all syntax and range checks have completed. */
    *p_ip_out = parsed_address;
    return PARSE_IPV4_SUCCESS;
}

static bool check_success_case(const char *p_input,
                               uint32_t expected_value,
                               const char *p_case_name)
{
    uint32_t actual_value = UINT32_C(0);

    if ((parse_ipv4(p_input, &actual_value) != PARSE_IPV4_SUCCESS) ||
        (actual_value != expected_value))
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_case_name);
        return false;
    }

    return true;
}

static bool check_failure_preserves_output(const char *p_input,
                                           const char *p_case_name)
{
    uint32_t actual_value = IPV4_SENTINEL;

    if ((parse_ipv4(p_input, &actual_value) >= PARSE_IPV4_SUCCESS) ||
        (actual_value != IPV4_SENTINEL))
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_case_name);
        return false;
    }

    return true;
}

static bool check_null_output(void)
{
    if (parse_ipv4("192.168.1.50", NULL) >= PARSE_IPV4_SUCCESS)
    {
        (void)fputs("FAIL: null output pointer\n", stderr);
        return false;
    }

    return true;
}

int main(void)
{
    bool all_tests_passed = true;

    all_tests_passed = check_success_case("192.168.1.50", UINT32_C(0xC0A80132),
                                           "source success case") && all_tests_passed;
    all_tests_passed = check_success_case("0.0.0.0", UINT32_C(0),
                                           "all-zero address") && all_tests_passed;
    all_tests_passed = check_success_case("255.255.255.255", UINT32_MAX,
                                           "all-ones address") && all_tests_passed;
    all_tests_passed = check_success_case("001.002.003.004", UINT32_C(0x01020304),
                                           "decimal leading zeroes") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("256.0.0.1",
                                                       "source range failure") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1.2.3",
                                                       "too few components") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1.2.3.4.5",
                                                       "too many components") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1..2.3",
                                                       "empty component") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("-1.2.3.4",
                                                       "negative component") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("+1.2.3.4",
                                                       "signed component") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1.a.2.3",
                                                       "non-decimal component") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1.2.3.4x",
                                                       "trailing text") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("1.2.3.4 ",
                                                       "trailing whitespace") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output(" 1.2.3.4",
                                                       "leading whitespace") && all_tests_passed;
    all_tests_passed = check_failure_preserves_output("", "empty input") &&
                       all_tests_passed;
    all_tests_passed = check_failure_preserves_output(NULL, "null input pointer") &&
                       all_tests_passed;
    all_tests_passed = check_null_output() && all_tests_passed;

    if (!all_tests_passed)
    {
        return EXIT_FAILURE;
    }

    (void)puts("PASS: IPv4 parser self-checks.");
    return EXIT_SUCCESS;
}
