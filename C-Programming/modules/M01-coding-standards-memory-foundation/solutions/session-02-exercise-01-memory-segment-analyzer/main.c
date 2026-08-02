/*
 * Session 02 / Exercise 1 — one reviewed implementation path.
 *
 * This program records observations from one selected build and run. It does
 * not establish ISO C section-placement rules or portable object ordering.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UINTPTR_MAX
#error "This laboratory requires an implementation that provides uintptr_t."
#endif

const uint32_t g_global_const = UINT32_C(100);
uint32_t g_global_init = UINT32_C(42);
uint32_t g_global_uninit;

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
 * @brief Print object-address and symbol-evidence observations for one build.
 *
 * @return None. If the required allocation fails, this standalone laboratory
 *         reports the failure and terminates with `EXIT_FAILURE`.
 *
 * `const` does not guarantee a `.rodata` section or Flash placement, and
 * BSS-like storage still consumes runtime memory. Function code is verified
 * through `nm`; `%p` is used only with object pointers converted to `void *`.
 * This source-required `malloc`/`free` laboratory conflicts with MISRA C:2012
 * Directive 4.12 and is not a MISRA-compliance claim.
 * Numeric magnitudes use this implementation's `uintptr_t` representation and
 * are observations, not portable pointer distances or section sizes.
 */
void print_memory_map(void)
{
    uint32_t stack_value = UINT32_C(0);
    uint32_t *p_heap_value = malloc(sizeof(*p_heap_value));
    const uintptr_t const_address = (uintptr_t)(const void *)&g_global_const;
    const uintptr_t data_address = (uintptr_t)(void *)&g_global_init;
    const uintptr_t bss_address = (uintptr_t)(void *)&g_global_uninit;
    uintptr_t heap_address;
    uintptr_t stack_address;

    if (p_heap_value == NULL)
    {
        (void)fputs("FAIL: heap allocation unavailable.\n", stderr);
        exit(EXIT_FAILURE);
    }

    *p_heap_value = UINT32_C(0);
    heap_address = (uintptr_t)(void *)p_heap_value;
    stack_address = (uintptr_t)(void *)&stack_value;

    (void)puts("=== Memory Segment Map (one build/run observation) ===");
    (void)puts("[TEXT]   print_memory_map code: inspect its symbol with nm");
    /* A function pointer is not passed to %p because %p is for void * values. */
    (void)printf("[CONST]  global const object:    %p\n",
                 (void *)&g_global_const);
    (void)printf("[DATA]   initialized object:     %p\n",
                 (void *)&g_global_init);
    (void)printf("[BSS]    uninitialized object:   %p\n",
                 (void *)&g_global_uninit);
    (void)printf("[HEAP]   allocated object:       %p\n", (void *)p_heap_value);
    (void)printf("[STACK]  automatic object:       %p\n", (void *)&stack_value);

    (void)puts("=== Numeric Address Magnitudes (implementation-specific) ===");
    (void)printf("CONST object <-> DATA object:  %" PRIuPTR "\n",
                 numeric_magnitude(const_address, data_address));
    (void)printf("DATA object <-> BSS object:    %" PRIuPTR "\n",
                 numeric_magnitude(data_address, bss_address));
    (void)printf("BSS object <-> HEAP object:    %" PRIuPTR "\n",
                 numeric_magnitude(bss_address, heap_address));
    (void)printf("HEAP object <-> STACK object:  %" PRIuPTR "\n",
                 numeric_magnitude(heap_address, stack_address));

    /* Every successful allocation has one explicit cleanup path. */
    free(p_heap_value);
    p_heap_value = NULL;
}

int main(void)
{
    print_memory_map();
    (void)puts("PASS: memory-segment observations completed.");
    return EXIT_SUCCESS;
}

/*
 * Human verification must regenerate evidence from the reviewed binary:
 *
 * $ cc --version
 * $ cc -dumpmachine
 * $ cc -std=c99 -Wall -Wextra -Wpedantic -Werror main.c -o memory_segment_analyzer
 * $ size memory_segment_analyzer
 * $ nm memory_segment_analyzer | grep -E 'g_global_(const|init|uninit)|print_memory_map'
 *
 * GNU size's Berkeley columns are summaries for that binary, not literal
 * section names. The nm letters are likewise evidence for that build only.
 */
