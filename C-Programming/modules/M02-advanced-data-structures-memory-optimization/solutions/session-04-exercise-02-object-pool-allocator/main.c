/*
 * Part B — Memory-layout inspection for the selected build
 *
 * A. ISO C requires zero initialization for these static objects, but it does
 * not require an object-file section named .bss. Embedded toolchains commonly
 * place zero-initialized writable static storage in BSS-like runtime RAM that
 * startup code clears.
 *
 * B. Both `static network_packet_t s_pool[5];` and an equivalent declaration
 * with `= {0}` have required zero-initialization semantics. The selected build
 * must be inspected; `= {0}` does not universally force a .data section.
 *
 * C. Function code commonly contributes to text-like image storage, while
 * writable static state needs runtime storage. Exact Flash and RAM placement
 * remains a target and linker-environment decision, not an ISO C rule.
 *
 * D. With the source-provided sizeof(network_packet_t) of 68 bytes, increasing
 * capacity from 5 to 50 adds (50 - 5) * 68 = 3060 bytes for packet storage.
 * It also adds (50 - 5) * sizeof(bool) bytes of occupancy bookkeeping; with
 * sizeof(bool) equal to one on the selected build, that contribution is 45
 * bytes. That calculation is not an exact total-BSS prediction because
 * alignment and other static symbols can contribute to the binary's summary.
 *
 * A uniform fixed-size pool avoids external fragmentation within this pool.
 * It can still reserve unused capacity or become full.
 *
 * Selected host observation (GCC 11.4.0, Linux x86-64):
 * `size exercise_2` reported text=2458, data=616, bss=384, dec=3458,
 * hex=d82. `nm -S` reported s_pool as a 0x154-byte local BSS symbol and
 * s_packet_in_use as a 0x5-byte local BSS symbol. These are selected-build
 * observations, not section-placement guarantees from ISO C.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 5

/**
 * @brief Represents one fixed-size packet slot managed by the pool.
 */
typedef struct
{
    uint32_t id;
    uint8_t payload[64];
} network_packet_t;

/** @brief Holds the five static packet slots. */
static network_packet_t s_pool[POOL_SIZE];

/** @brief Records whether each corresponding packet slot is allocated. */
static bool s_packet_in_use[POOL_SIZE];

/**
 * @brief Allocates the first available packet slot.
 *
 * A linear scan is O(N) in capacity. For POOL_SIZE equal to five, it performs
 * at most five occupancy checks.
 *
 * @return Pointer to a newly owned pool slot, or NULL when every slot is in
 *         use.
 */
network_packet_t *packet_alloc(void)
{
    size_t index;

    for (index = 0U; index < POOL_SIZE; ++index)
    {
        if (!s_packet_in_use[index])
        {
            s_packet_in_use[index] = true;
            return &s_pool[index];
        }
    }

    return NULL;
}

/**
 * @brief Returns an owned packet slot to the pool when it is an exact member.
 *
 * NULL and non-pool pointers are ignored without dereferencing them. An
 * already-free pool slot remains free. A valid release resets the slot ID
 * before returning it to the pool; the caller must no longer treat it as
 * owned.
 *
 * @param[in] p_packet Candidate packet pointer to release.
 */
void packet_free(network_packet_t *p_packet)
{
    size_t index;

    if (p_packet == NULL)
    {
        return;
    }

    for (index = 0U; index < POOL_SIZE; ++index)
    {
        if (p_packet == &s_pool[index])
        {
            if (s_packet_in_use[index])
            {
                p_packet->id = 0U;
                s_packet_in_use[index] = false;
            }

            return;
        }
    }
}

/**
 * @brief Demonstrates full-pool failure followed by valid slot reuse.
 *
 * A successful allocation temporarily transfers ownership of the returned
 * slot to the caller. Pool exhaustion is expected and is reported by NULL.
 *
 * @return EXIT_SUCCESS when the required state transitions occur; otherwise
 *         EXIT_FAILURE.
 */
int main(void)
{
    network_packet_t *p_packets[POOL_SIZE] = {NULL};
    network_packet_t *p_reused_packet;
    network_packet_t non_pool_packet = {0};
    size_t index;

    for (index = 0U; index < POOL_SIZE; ++index)
    {
        p_packets[index] = packet_alloc();
        (void)printf("Allocating packet %u: %s\n",
                     (unsigned int)(index + 1U),
                     (p_packets[index] != NULL) ? "Success" : "Failed");

        if (p_packets[index] == NULL)
        {
            return EXIT_FAILURE;
        }
    }

    if (packet_alloc() != NULL)
    {
        return EXIT_FAILURE;
    }

    (void)printf("Allocating packet 6: Failed (Pool Full)\n");

    packet_free(NULL);
    packet_free(&non_pool_packet);

    (void)printf("Freeing packet 2...\n");
    packet_free(p_packets[1]);

    p_reused_packet = packet_alloc();
    if (p_reused_packet == NULL)
    {
        return EXIT_FAILURE;
    }

    (void)printf("Allocating packet 6 again: Success\n");

    return EXIT_SUCCESS;
}
