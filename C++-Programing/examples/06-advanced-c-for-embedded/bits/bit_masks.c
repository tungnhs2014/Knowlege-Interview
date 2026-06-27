#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static bool mask_u32(unsigned int bit, uint32_t *out_mask)
{
    if (out_mask == NULL || bit >= 32U) {
        return false;
    }

    *out_mask = UINT32_C(1) << bit;
    return true;
}

static bool update_bit(
    uint32_t value,
    unsigned int bit,
    bool set,
    uint32_t *out_value)
{
    uint32_t mask;

    if (out_value == NULL || !mask_u32(bit, &mask)) {
        return false;
    }

    *out_value = set ? value | mask : value & ~mask;
    return true;
}

int main(void)
{
    uint32_t value = UINT32_C(0);
    uint32_t top_mask;

    if (!update_bit(value, 3U, true, &value)
        || !update_bit(value, 7U, true, &value)
        || !update_bit(value, 3U, false, &value)
        || !mask_u32(31U, &top_mask)) {
        return 1;
    }

    value ^= top_mask;

    printf("value=0x%08" PRIx32 "\n", value);
    printf("bit31=%s bit7=%s invalid32=%s\n",
           (value & top_mask) != 0U ? "set" : "clear",
           (value & (UINT32_C(1) << 7)) != 0U ? "set" : "clear",
           mask_u32(32U, &top_mask) ? "accepted" : "rejected");
    return 0;
}
