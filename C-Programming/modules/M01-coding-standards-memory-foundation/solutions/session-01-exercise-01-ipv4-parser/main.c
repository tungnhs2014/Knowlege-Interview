/*
 * Session 01 / Exercise 1 reference solution.
 *
 * This file demonstrates one reviewed implementation path for the approved
 * IPv4 parser exercise. It is intentionally a small, direct parser rather
 * than a reusable text-conversion framework.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PARSE_IPV4_SUCCESS ((int8_t)0)
#define PARSE_IPV4_INVALID_INPUT ((int8_t)-1)
#define IPV4_OCTET_COUNT (4U)

/**
 * @brief Parse a dotted-decimal IPv4 address into a 32-bit numeric value.
 *
 * @param[in] ip_str Null-terminated ASCII input containing four decimal
 *                   octets separated by dots.
 * @param[out] p_ip_out Output location written only when parsing succeeds.
 * @return 0 on success; a negative value for null, malformed, or out-of-range
 *         input.
 */
int8_t parse_ipv4(const char *ip_str, uint32_t *p_ip_out)
{
    const char *p_current;
    uint32_t parsed_address = 0U;
    uint8_t octet_index;

    if ((ip_str == NULL) || (p_ip_out == NULL))
    {
        return PARSE_IPV4_INVALID_INPUT;
    }

    p_current = ip_str;

    for (octet_index = 0U; octet_index < IPV4_OCTET_COUNT; ++octet_index)
    {
        bool has_digit = false;
        uint32_t octet_value = 0U;

        while ((*p_current >= '0') && (*p_current <= '9'))
        {
            uint32_t digit_value = (uint32_t)(*p_current - '0');

            if ((octet_value > 25U) ||
                ((octet_value == 25U) && (digit_value > 5U)))
            {
                return PARSE_IPV4_INVALID_INPUT;
            }

            octet_value = (octet_value * 10U) + digit_value;
            has_digit = true;
            ++p_current;
        }

        if (!has_digit)
        {
            return PARSE_IPV4_INVALID_INPUT;
        }

        parsed_address = (parsed_address << 8U) | octet_value;

        if (octet_index < (IPV4_OCTET_COUNT - 1U))
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

    *p_ip_out = parsed_address;
    return PARSE_IPV4_SUCCESS;
}

static int run_success_case(const char *p_input, uint32_t expected_value,
                            const char *p_case_name)
{
    uint32_t parsed_value = 0U;

    if ((parse_ipv4(p_input, &parsed_value) != PARSE_IPV4_SUCCESS) ||
        (parsed_value != expected_value))
    {
        (void)fprintf(stderr, "FAILED: %s\n", p_case_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_failure_case(const char *p_input, const char *p_case_name)
{
    uint32_t parsed_value = 0U;

    if (parse_ipv4(p_input, &parsed_value) >= PARSE_IPV4_SUCCESS)
    {
        (void)fprintf(stderr, "FAILED: %s\n", p_case_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_null_output_case(void)
{
    if (parse_ipv4("192.168.1.50", NULL) >= PARSE_IPV4_SUCCESS)
    {
        (void)fputs("FAILED: null output pointer\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(void)
{
    int failures = 0;

    failures += run_success_case("192.168.1.50", UINT32_C(0xC0A80132),
                                 "192.168.1.50");
    failures += run_success_case("0.0.0.0", UINT32_C(0), "0.0.0.0");
    failures += run_success_case("255.255.255.255", UINT32_MAX,
                                 "255.255.255.255");
    failures += run_failure_case("256.0.0.1", "octet out of range");
    failures += run_failure_case("1.2.3", "too few octets");
    failures += run_failure_case("1.2.3.4.5", "too many octets");
    failures += run_failure_case("1..2.3", "missing octet");
    failures += run_failure_case("1.a.2.3", "non-decimal octet");
    failures += run_failure_case("-1.2.3.4", "signed octet");
    failures += run_failure_case("1.2.3.4x", "trailing invalid text");
    failures += run_failure_case(NULL, "null input pointer");
    failures += run_null_output_case();

    if (failures != 0)
    {
        return EXIT_FAILURE;
    }

    (void)puts("IPv4 parser tests passed.");
    return EXIT_SUCCESS;
}
