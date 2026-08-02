/*
 * Session 02 / Exercise 2 — one reviewed implementation path.
 *
 * The reported address magnitude is one build/run observation. It is neither
 * exact stack usage nor a portable frame-size measurement. Recursion is
 * intentionally required here but conflicts with MISRA C:2012 Rule 17.2.
 */

#include <inttypes.h>
#include <stdbool.h>
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
 * @brief Recurse while reporting a build-specific stack-address proxy.
 *
 * @param[in] current_depth Current depth for this invocation.
 * @param[in] max_depth Highest requested depth for the bounded run.
 * @param[in] stack_base_addr Converted address of a local object in `main`.
 * @param[in] stack_limit_bytes Source-defined guard threshold compared with
 *                               the address proxy, not an exact byte count.
 * @return `0` when `max_depth` is reached before the guard; `-1` when the
 *         observed magnitude reaches or exceeds the guard.
 *
 * There is no caller-owned output object to update. The guard stops another
 * recursive call but does not prove the actual overflow boundary or make this
 * required recursion compliant with MISRA C:2012 Rule 17.2.
 */
int8_t recurse_with_monitor(uint32_t current_depth,
                            uint32_t max_depth,
                            const uintptr_t stack_base_addr,
                            uint32_t stack_limit_bytes)
{
    uint8_t stack_marker = UINT8_C(0);
    const uintptr_t current_address = (uintptr_t)(void *)&stack_marker;
    const uintptr_t observed_magnitude = numeric_magnitude(current_address,
                                                           stack_base_addr);

    (void)printf("[Depth %" PRIu32 "] stack_marker: %p, "
                 "observed address magnitude: %" PRIuPTR "\n",
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

    /* current_depth < max_depth <= UINT32_MAX, so this increment cannot wrap. */
    return recurse_with_monitor(current_depth + UINT32_C(1), max_depth,
                                stack_base_addr, stack_limit_bytes);
}

static bool result_matches(int8_t actual_result,
                           int8_t expected_result,
                           const char *p_case_name)
{
    if (actual_result != expected_result)
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_case_name);
        return false;
    }

    return true;
}

int main(void)
{
    uint8_t bounded_base_marker = UINT8_C(0);
    uint8_t guarded_base_marker = UINT8_C(0);
    const uintptr_t bounded_base_addr =
        (uintptr_t)(void *)&bounded_base_marker;
    const uintptr_t guarded_base_addr =
        (uintptr_t)(void *)&guarded_base_marker;
    const int8_t bounded_result = recurse_with_monitor(0U, 3U,
                                                        bounded_base_addr,
                                                        UINT32_MAX);
    const int8_t guarded_result = recurse_with_monitor(0U, 8U,
                                                        guarded_base_addr,
                                                        UINT32_C(1));

    if ((!result_matches(bounded_result, STACK_MONITOR_SUCCESS,
                         "bounded recursion")) ||
        (!result_matches(guarded_result, STACK_MONITOR_LIMIT_REACHED,
                         "guarded recursion")))
    {
        return EXIT_FAILURE;
    }

    (void)puts("PASS: stack-monitor self-checks.");
    return EXIT_SUCCESS;
}
