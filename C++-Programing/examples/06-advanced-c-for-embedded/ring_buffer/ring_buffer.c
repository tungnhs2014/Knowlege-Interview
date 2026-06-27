#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum { RING_CAPACITY = 4 };

typedef struct {
    uint8_t data[RING_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
} ByteRing;

static bool byte_ring_push(ByteRing *ring, uint8_t value)
{
    if (ring == NULL || ring->count == RING_CAPACITY) {
        return false;
    }

    ring->data[ring->head] = value;
    ring->head = (ring->head + 1U) % RING_CAPACITY;
    ++ring->count;
    return true;
}

static bool byte_ring_pop(ByteRing *ring, uint8_t *out_value)
{
    if (ring == NULL || out_value == NULL || ring->count == 0U) {
        return false;
    }

    *out_value = ring->data[ring->tail];
    ring->tail = (ring->tail + 1U) % RING_CAPACITY;
    --ring->count;
    return true;
}

int main(void)
{
    ByteRing ring = {{0U}, 0U, 0U, 0U};

    for (uint8_t value = 1U; value <= RING_CAPACITY; ++value) {
        if (!byte_ring_push(&ring, value)) {
            return 1;
        }
    }

    printf("push-when-full=%s\n",
           byte_ring_push(&ring, UINT8_C(99))
               ? "accepted"
               : "rejected");

    while (ring.count != 0U) {
        uint8_t value;
        if (!byte_ring_pop(&ring, &value)) {
            return 1;
        }
        printf("%" PRIu8 "%c", value, ring.count == 0U ? '\n' : ' ');
    }

    return 0;
}
