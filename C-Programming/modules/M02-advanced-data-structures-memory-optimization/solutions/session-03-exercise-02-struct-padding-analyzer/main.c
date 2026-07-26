/*
 * Internal padding can appear between members so that a later member meets
 * this build's alignment requirements. Tail padding can make every element
 * of an array of structures suitably aligned. The reported sizes and offsets
 * are observations for this compiler, ABI, target, and options.
 *
 * __attribute__((packed)) is a GCC/Clang-family compiler extension, not an
 * ISO C layout feature. Direct access to a packed member is different from
 * taking its address as an ordinary typed pointer: the compiler can account
 * for direct member access in the selected build, while that pointer may not
 * meet the member type's normal alignment requirement. An actual unaligned
 * access may be handled, decomposed, slowed, or fault depending on the target
 * and generated access sequence; it is not universally an ARM HardFault.
 */

#include <stddef.h>
#include <stdio.h>

/**
 * @brief Preserves the source-required mixed member order.
 */
typedef struct
{
    char tag;
    int sample_count;
    double measurement;
    short status;
} original_layout_t;

/**
 * @brief Uses the same member types in an order measured for this build.
 */
typedef struct
{
    double measurement;
    int sample_count;
    short status;
    char tag;
} reordered_layout_t;

/**
 * @brief Uses the original member order with a compiler-specific packed request.
 */
typedef struct __attribute__((packed))
{
    char tag;
    int sample_count;
    double measurement;
    short status;
} packed_layout_t;

/**
 * @brief Prints the original structure's selected-build layout observation.
 */
static void report_original_layout(void)
{
    (void)printf("=== Original Layout ===\n");
    (void)printf("sizeof: %zu\n", sizeof(original_layout_t));
    (void)printf("member offsets: tag=%zu, sample_count=%zu, "
                 "measurement=%zu, status=%zu\n",
                 offsetof(original_layout_t, tag),
                 offsetof(original_layout_t, sample_count),
                 offsetof(original_layout_t, measurement),
                 offsetof(original_layout_t, status));
}

/**
 * @brief Prints the reordered structure's selected-build layout observation.
 */
static void report_reordered_layout(void)
{
    (void)printf("=== Reordered Layout ===\n");
    (void)printf("sizeof: %zu\n", sizeof(reordered_layout_t));
    (void)printf("member offsets: measurement=%zu, sample_count=%zu, "
                 "status=%zu, tag=%zu\n",
                 offsetof(reordered_layout_t, measurement),
                 offsetof(reordered_layout_t, sample_count),
                 offsetof(reordered_layout_t, status),
                 offsetof(reordered_layout_t, tag));
}

/**
 * @brief Prints the packed structure's selected-build layout observation.
 */
static void report_packed_layout(void)
{
    (void)printf("=== Packed Layout ===\n");
    (void)printf("sizeof: %zu\n", sizeof(packed_layout_t));
    (void)printf("member offsets: tag=%zu, sample_count=%zu, "
                 "measurement=%zu, status=%zu\n",
                 offsetof(packed_layout_t, tag),
                 offsetof(packed_layout_t, sample_count),
                 offsetof(packed_layout_t, measurement),
                 offsetof(packed_layout_t, status));
}

#ifdef PACKED_MEMBER_DIAGNOSTIC
/**
 * @brief Intentionally takes a packed-member address for the isolated warning experiment.
 *
 * The resulting pointer is deliberately not dereferenced.
 *
 * @param[in] p_packed Valid packed object used only for the experiment.
 */
static void observe_packed_member_address(const packed_layout_t *p_packed)
{
    const int *p_sample_count = &p_packed->sample_count;

    (void)p_sample_count;
}
#endif

/**
 * @brief Reports layout observations and demonstrates direct packed-member access.
 *
 * @return Zero after completing the selected-build observations.
 */
int main(void)
{
    packed_layout_t packed_layout = {0};

    report_original_layout();
    report_reordered_layout();
    report_packed_layout();

    packed_layout.sample_count = 10;
    (void)printf("=== Direct Packed Access ===\n");
    (void)printf("sample_count: %d\n", packed_layout.sample_count);

#ifdef PACKED_MEMBER_DIAGNOSTIC
    observe_packed_member_address(&packed_layout);
#endif

    return 0;
}
