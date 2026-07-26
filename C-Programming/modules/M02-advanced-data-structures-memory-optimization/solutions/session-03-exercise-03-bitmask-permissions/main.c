/*
 * Each permission is one bit in a mask. has_permission() answers the
 * all-required question: every bit requested by required_perms must also be
 * set in user_perms. This is different from testing whether any one requested
 * bit is present.
 *
 * uint8_t operands undergo integer promotion in a bitwise expression. The
 * flag values in this exercise fit in uint8_t, so the combined enum mask is
 * explicitly converted when it is stored in a uint8_t object.
 *
 * sizeof(sys_perms_e) is an implementation, ABI, compiler, and option
 * observation. A compiler that supports -fshort-enums may select a smaller
 * enum representation. That choice can change structure layout, calling, and
 * interface expectations, so separately compiled objects and libraries that
 * exchange enum-containing interfaces require compatible ABI assumptions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Defines the source-required independent permission bits.
 */
typedef enum
{
    PERM_READ = (1U << 0),
    PERM_WRITE = (1U << 1),
    PERM_EXECUTE = (1U << 2),
    PERM_DELETE = (1U << 3)
} sys_perms_e;

/**
 * @brief Determines whether every required permission is present.
 *
 * @param[in] user_perms Permissions currently granted to the user.
 * @param[in] required_perms Permissions that the operation requires.
 *
 * @return true when all required bits are present; otherwise false.
 */
bool has_permission(uint8_t user_perms, uint8_t required_perms)
{
    return (user_perms & required_perms) == required_perms;
}

/**
 * @brief Prints one permission result and verifies its expected outcome.
 *
 * @param[in] p_label Text that identifies the required permission set.
 * @param[in] user_perms Permissions currently granted to the user.
 * @param[in] required_perms Permissions that this check requires.
 * @param[in] expected Expected all-required result.
 *
 * @return true when the observed result matches expected; otherwise false.
 */
static bool report_permission_check(const char *p_label,
                                    uint8_t user_perms,
                                    uint8_t required_perms,
                                    bool expected)
{
    bool granted = has_permission(user_perms, required_perms);

    (void)printf("%s %s\n", p_label, granted ? "GRANTED" : "DENIED");

    return granted == expected;
}

/**
 * @brief Exercises all-required permission checks for the source scenarios.
 *
 * @return EXIT_SUCCESS when every expected result is observed; otherwise
 *         EXIT_FAILURE.
 */
int main(void)
{
    uint8_t user_perms = (uint8_t)(PERM_READ | PERM_WRITE);
    uint8_t required_read = (uint8_t)PERM_READ;
    uint8_t required_execute = (uint8_t)PERM_EXECUTE;
    uint8_t required_read_write = (uint8_t)(PERM_READ | PERM_WRITE);

    (void)printf("=== Bitmask Permissions Tester ===\n");
    (void)printf("Enum size: %zu bytes\n", sizeof(sys_perms_e));
    (void)printf("User 1 (Read|Write): 0x%02X\n",
                 (unsigned int)user_perms);

    if (!report_permission_check("Checking for Read permission...",
                                 user_perms,
                                 required_read,
                                 true))
    {
        return EXIT_FAILURE;
    }

    if (!report_permission_check("Checking for Execute permission...",
                                 user_perms,
                                 required_execute,
                                 false))
    {
        return EXIT_FAILURE;
    }

    if (!report_permission_check("Checking for Read AND Write...",
                                 user_perms,
                                 required_read_write,
                                 true))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
