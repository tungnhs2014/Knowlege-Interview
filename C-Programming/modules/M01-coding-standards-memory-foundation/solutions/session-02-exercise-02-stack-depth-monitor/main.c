/*
 * Session 02 / Exercise 2 reference solution.
 *
 * Recursion is intentionally required by this teaching exercise. The guard
 * reduces experimental risk but does not make recursion MISRA C:2012 Rule
 * 17.2 compliant. Reported magnitudes are observations from one build and
 * run, not exact physical stack usage or a fixed frame-size guarantee.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UINTPTR_MAX
#error "This laboratory requires an implementation that provides uintptr_t."
#endif

#define STACK_MONITOR_SUCCESS ((int8_t)0)
#define STACK_MONITOR_LIMIT_REACHED ((int8_t)-1)

static uintptr_t numeric_magnitude(uintptr_t left_address,
                                   uintptr_t right_address)
{
    if (left_address >= right_address)
    {
        return left_address - right_address;
    }

    return right_address - left_address;
}

/**
 * @brief Recurse while reporting a build-specific stack-address observation.
 *
 * @param[in] current_depth Current recursive depth, beginning at zero.
 * @param[in] max_depth Maximum depth accepted by this controlled run.
 * @param[in] stack_base_addr Converted address of a local marker in main().
 * @param[in] stack_limit_bytes Source-defined guard threshold compared with
 *                               this build-specific numeric address proxy; it
 *                               is not an exact or portable stack-byte count.
 * @return 0 when max_depth is reached, or -1 when the guard is reached.
 */
int8_t recurse_with_monitor(uint32_t current_depth,
                            uint32_t max_depth,
                            const uintptr_t stack_base_addr,
                            uint32_t stack_limit_bytes)
{
    uint8_t stack_marker = 0U;
    uintptr_t current_address = (uintptr_t)(void *)&stack_marker;
    uintptr_t observed_magnitude = numeric_magnitude(current_address,
                                                     stack_base_addr);

    (void)printf("[Depth %" PRIu32 "] stack_marker: %p, observed address magnitude: %" PRIuPTR
                 "\n",
                 current_depth, (void *)&stack_marker, observed_magnitude);

    if ((uintmax_t)observed_magnitude >= (uintmax_t)stack_limit_bytes)
    {
        (void)printf("[Depth %" PRIu32 "] WARNING: observed magnitude reached "
                     "the configured guard.\n",
                     current_depth);
        return STACK_MONITOR_LIMIT_REACHED;
    }

    if (current_depth >= max_depth)
    {
        return STACK_MONITOR_SUCCESS;
    }

    return recurse_with_monitor(current_depth + 1U, max_depth,
                                stack_base_addr, stack_limit_bytes);
}

int main(void)
{
    uint8_t stack_base_marker = 0U;
    uintptr_t stack_base_addr = (uintptr_t)(void *)&stack_base_marker;
    int8_t bounded_result;
    int8_t guard_result;

    (void)puts("=== Bounded stack-monitor run ===");
    bounded_result = recurse_with_monitor(0U, 3U, stack_base_addr,
                                          UINT32_MAX);

    (void)puts("=== Guard stack-monitor run ===");
    guard_result = recurse_with_monitor(0U, 8U, stack_base_addr, 1U);

    if ((bounded_result != STACK_MONITOR_SUCCESS) ||
        (guard_result != STACK_MONITOR_LIMIT_REACHED))
    {
        (void)fputs("Stack monitor tests failed.\n", stderr);
        return EXIT_FAILURE;
    }

    (void)puts("Stack monitor tests passed.");
    return EXIT_SUCCESS;
}
