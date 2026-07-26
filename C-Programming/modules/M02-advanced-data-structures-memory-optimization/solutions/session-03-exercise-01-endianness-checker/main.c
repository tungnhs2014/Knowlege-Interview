/*
 * Endianness is the order in which the bytes of a multi-byte value occupy
 * increasing memory addresses. The least-significant byte (LSB) carries the
 * lowest-order bits of a value; the most-significant byte (MSB) carries the
 * highest-order bits. For 0x11223344, a conventional little-endian
 * representation places 0x44 at the lowest address, while a conventional
 * big-endian representation places 0x11 there.
 *
 * Embedded code must respect the byte order specified by a network protocol
 * or sensor format. This program observes one implementation's host object
 * representation only; it is not a portable serializer or deserializer.
 */

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Stores one 32-bit value and an observed four-byte representation.
 *
 * This union is used only for the source-required representation observation.
 */
typedef union
{
    uint32_t full_word;
    uint8_t bytes[4];
} endian_checker_u;

/**
 * @brief Observes the selected build's representation of one 32-bit value.
 *
 * @return Zero after reporting a classified or unclassified observation.
 */
int main(void)
{
    endian_checker_u checker = {0U};

    checker.full_word = 0x11223344U;

    (void)printf("=== Endianness Checker ===\n");
    (void)printf("Stored Value: 0x%08lX\n", (unsigned long)checker.full_word);
    (void)printf("First Byte in Memory: 0x%02X\n",
                 (unsigned int)checker.bytes[0]);

    if (checker.bytes[0] == 0x44U)
    {
        (void)printf("Result: This build observes Little-Endian order.\n");
    }
    else if (checker.bytes[0] == 0x11U)
    {
        (void)printf("Result: This build observes Big-Endian order.\n");
    }
    else
    {
        (void)printf("Result: Observation is not classified by this lab.\n");
    }

    return 0;
}
