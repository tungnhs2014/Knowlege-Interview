#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static bool decode_u16_be(
    const uint8_t *bytes,
    size_t length,
    uint16_t *out_value)
{
    if (bytes == NULL || out_value == NULL || length < 2U) {
        return false;
    }

    *out_value = (uint16_t)(
        ((uint16_t)bytes[0] << 8)
        | (uint16_t)bytes[1]);
    return true;
}

static bool encode_u16_be(
    uint16_t value,
    uint8_t *bytes,
    size_t capacity)
{
    if (bytes == NULL || capacity < 2U) {
        return false;
    }

    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)(value & UINT16_C(0x00ff));
    return true;
}

int main(void)
{
    const uint8_t input[] = {UINT8_C(0x12), UINT8_C(0x34)};
    uint8_t output[2] = {0U, 0U};
    uint16_t value;

    if (!decode_u16_be(input, sizeof input, &value)
        || !encode_u16_be(value, output, sizeof output)) {
        return 1;
    }

    printf("value=0x%04" PRIx16 " bytes=%02" PRIx8 "%02" PRIx8 "\n",
           value,
           output[0],
           output[1]);
    printf("short-input=%s\n",
           decode_u16_be(input, 1U, &value) ? "accepted" : "rejected");
    return 0;
}
