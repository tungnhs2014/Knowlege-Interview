/*
 * This program records one GCC/Clang-family build's aggregate and bit-field
 * observations. __attribute__((packed)) is a compiler extension, not an ISO C
 * layout guarantee. Packing an outer aggregate controls that aggregate's
 * layout; it does not recursively rewrite the size or layout of an already
 * defined nested union.
 *
 * Bit-field allocation order, allocation/storage-unit boundaries, padding,
 * alignment, representation, and byte order can depend on the compiler, ABI,
 * target, and options. uint32_t as a bit-field base type is also
 * implementation/toolchain dependent in C99. The whole-word/bit-field union
 * below therefore observes this build only; it is not a portable peripheral
 * register ABI. CMSIS-style device headers commonly use integer-width
 * register objects with named bit positions and masks so exact hardware bit
 * positions do not rely on C bit-field allocation order.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Shares one whole-word member and a five-byte member without packing.
 *
 * The selected ABI may add tail padding so that this union's size satisfies
 * its alignment requirement.
 */
typedef union
{
    uint32_t word;
    uint8_t bytes[5];
} unpacked_data_u;

/**
 * @brief Applies the selected compiler's packing extension to an outer type.
 *
 * @note The nested unpacked_data_u retains its independently defined layout.
 */
typedef struct __attribute__((packed))
{
    unpacked_data_u data;
} packed_outer_with_unpacked_u;

/**
 * @brief Shares the source-required members with a direct packing request.
 *
 * @note __attribute__((packed)) is a GCC/Clang-family extension.
 */
typedef union __attribute__((packed))
{
    uint32_t word;
    uint8_t bytes[5];
} packed_data_u;

/**
 * @brief Verifies the source-controlled extents of both required union views.
 *
 * The sizeof operands are not evaluated. This check does not assert either
 * union's complete layout, alignment, or packed size.
 *
 * @return true when both declarations retain the required word and byte-array
 *         member extents; otherwise false.
 */
static bool packing_members_have_source_extents(void)
{
    return (sizeof(((unpacked_data_u *)0)->word) == sizeof(uint32_t)) &&
           (sizeof(((unpacked_data_u *)0)->bytes) ==
            (5U * sizeof(uint8_t))) &&
           (sizeof(((packed_data_u *)0)->word) == sizeof(uint32_t)) &&
           (sizeof(((packed_data_u *)0)->bytes) == (5U * sizeof(uint8_t)));
}

/**
 * @brief Applies the selected compiler's packing extension to an outer type.
 */
typedef struct __attribute__((packed))
{
    packed_data_u data;
} packed_outer_with_packed_u;

/**
 * @brief Models the source-required logical widths for this build experiment.
 *
 * This declaration does not define a portable 32-bit register representation.
 */
typedef struct
{
    uint32_t EN       : 1;
    uint32_t MODE     : 3;
    uint32_t FLAG     : 1;
    uint32_t RESERVED : 27;
} register_bits_t;

/**
 * @brief Places a whole-word view beside the bit-field view for observation.
 *
 * This is an educational representation experiment only. It is neither
 * volatile nor mapped to a hardware address.
 */
typedef union
{
    uint32_t ALL;
    register_bits_t BIT;
} peripheral_register_u;

/**
 * @brief Runs the selected-build packing and bit-field observations.
 *
 * @return EXIT_SUCCESS after recording the selected-build observations.
 */
int main(void)
{
    peripheral_register_u reg = {0};

    if (!packing_members_have_source_extents())
    {
        return EXIT_FAILURE;
    }

    (void)printf("=== Nested Union Packing ===\n");
    (void)printf("Unpacked union size: %zu\n", sizeof(unpacked_data_u));
    (void)printf("Packed outer with unpacked union size: %zu\n",
                 sizeof(packed_outer_with_unpacked_u));
    (void)printf("Packed union size: %zu\n", sizeof(packed_data_u));
    (void)printf("Packed outer with packed union size: %zu\n",
                 sizeof(packed_outer_with_packed_u));

    (void)printf("\n=== Bit-field Layout ===\n");
    (void)printf("Bit-field struct size: %zu\n", sizeof(register_bits_t));

    (void)printf("\n=== Peripheral Union Observation ===\n");
    reg.ALL = 0U;
    (void)printf("Initial ALL: 0x%08" PRIX32 "\n", reg.ALL);

    reg.BIT.EN = 1U;
    (void)printf("After BIT.EN = 1, observed ALL: 0x%08" PRIX32 "\n",
                 reg.ALL);

    reg.ALL = 0U;
    (void)printf("After ALL = 0, ALL: 0x%08" PRIX32 "\n", reg.ALL);
    (void)printf("Observed EN after clear: %u\n", (unsigned int)reg.BIT.EN);

    return EXIT_SUCCESS;
}
