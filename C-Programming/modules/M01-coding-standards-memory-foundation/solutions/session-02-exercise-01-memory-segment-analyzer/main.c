/*
 * Session 02 / Exercise 1 reference solution.
 *
 * The printed addresses and numeric magnitudes are observations of this
 * build and run. They do not establish an ISO C section-placement rule or a
 * portable ordering of unrelated objects.
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
 * @brief Print runtime object-address observations for one built program.
 *
 * This laboratory allocates one small heap object. If allocation fails, the
 * standalone program exits with failure because this required interface cannot
 * return an error. That is a lab-level choice, not a reusable-library API.
 */
void print_memory_map(void)
{
    uint32_t stack_value = 0U;
    uint32_t *p_heap_value = malloc(sizeof(*p_heap_value));
    uintptr_t rodata_address;
    uintptr_t data_address;
    uintptr_t bss_address;
    uintptr_t heap_address;
    uintptr_t stack_address;

    if (p_heap_value == NULL)
    {
        (void)fputs("Heap allocation failed.\n", stderr);
        exit(EXIT_FAILURE);
    }

    *p_heap_value = UINT32_C(0);
    rodata_address = (uintptr_t)(void *)&g_global_const;
    data_address = (uintptr_t)(void *)&g_global_init;
    bss_address = (uintptr_t)(void *)&g_global_uninit;
    heap_address = (uintptr_t)(void *)p_heap_value;
    stack_address = (uintptr_t)(void *)&stack_value;

    (void)puts("=== Memory Segment Map (one build/run observation) ===");
    (void)printf("[RODATA] object g_global_const:  %p\n",
                 (void *)&g_global_const);
    (void)printf("[DATA]   object g_global_init:   %p\n",
                 (void *)&g_global_init);
    (void)printf("[BSS]    object g_global_uninit: %p\n",
                 (void *)&g_global_uninit);
    (void)printf("[HEAP]   allocated object:       %p\n", (void *)p_heap_value);
    (void)printf("[STACK]  local stack_value:       %p\n", (void *)&stack_value);

    (void)puts("=== Numeric Address Magnitudes (not pointer distances) ===");
    (void)printf("RODATA object <-> DATA object:  %" PRIuPTR "\n",
                 numeric_magnitude(rodata_address, data_address));
    (void)printf("DATA object <-> BSS object:     %" PRIuPTR "\n",
                 numeric_magnitude(data_address, bss_address));
    (void)printf("BSS object <-> HEAP object:     %" PRIuPTR "\n",
                 numeric_magnitude(bss_address, heap_address));
    (void)printf("HEAP object <-> STACK object:   %" PRIuPTR "\n",
                 numeric_magnitude(heap_address, stack_address));

    free(p_heap_value);
    p_heap_value = NULL;
}

int main(void)
{
    print_memory_map();
    return EXIT_SUCCESS;
}

/*
 * Required tool evidence captured from the actual memory_segment_analyzer
 * binary built by this solution.
 *
 * $ size memory_segment_analyzer
 *    text    data     bss     dec     hex filename
 *    3162     652      16    3830     ef6 memory_segment_analyzer
 *
 * $ nm memory_segment_analyzer | grep -E 'g_global_(const|init|uninit)|print_memory_map'
 * 0000000000002008 R g_global_const
 * 0000000000004010 D g_global_init
 * 000000000000402c B g_global_uninit
 * 0000000000001237 T print_memory_map
 *
 * GNU size's Berkeley columns are summary categories for this binary; they
 * are not a universal list of literal section names. The nm symbol letters
 * describe this build only.
 */
