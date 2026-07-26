/*
 * This parser handles an external six-byte, big-endian representation. It
 * keeps three separate hazards out of the design: a byte buffer may lack
 * wider-type alignment, a wider incompatible lvalue can violate effective
 * type/aliasing requirements, and a native wider load can use a byte order
 * different from the protocol. Explicit byte reconstruction avoids relying on
 * all three assumptions.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Holds decoded sensor values in their native numeric types.
 */
typedef struct
{
    uint16_t temperature;
    uint32_t timestamp;
} sensor_data_t;

/**
 * @brief Decodes one six-byte big-endian sensor payload.
 *
 * @pre p_buffer refers to at least six readable bytes.
 * @param[in] p_buffer Pointer to the external payload bytes.
 * @param[out] p_out_data Destination for the decoded numeric values.
 *
 * When either pointer is NULL, this function returns without dereferencing it
 * and leaves a non-null output object unchanged. The fixed interface has no
 * length parameter, so the caller—not this function—must establish the
 * six-readable-byte precondition before the call.
 */
void parse_sensor_data(const uint8_t *p_buffer, sensor_data_t *p_out_data)
{
    if ((p_buffer == NULL) || (p_out_data == NULL))
    {
        return;
    }

    p_out_data->temperature =
        (uint16_t)(((uint16_t)p_buffer[0] << 8U) | (uint16_t)p_buffer[1]);
    p_out_data->timestamp = ((uint32_t)p_buffer[2] << 24U) |
                            ((uint32_t)p_buffer[3] << 16U) |
                            ((uint32_t)p_buffer[4] << 8U) |
                            (uint32_t)p_buffer[5];
}

/**
 * @brief Verifies the source-required payload and null-input behavior.
 *
 * @return EXIT_SUCCESS when decoding and null checks preserve the required
 *         behavior; otherwise EXIT_FAILURE.
 */
int main(void)
{
    const uint8_t payload[6] = {0x01U, 0x2CU, 0x00U, 0x00U, 0x1AU, 0x0AU};
    sensor_data_t sensor_data = {0U, 0U};
    sensor_data_t unchanged_data = {7U, 11U};

    parse_sensor_data(NULL, &unchanged_data);
    parse_sensor_data(payload, NULL);

    if ((unchanged_data.temperature != 7U) || (unchanged_data.timestamp != 11U))
    {
        return EXIT_FAILURE;
    }

    parse_sensor_data(payload, &sensor_data);

    (void)printf("Temperature: %u\n", (unsigned int)sensor_data.temperature);
    (void)printf("Timestamp: %" PRIu32 "\n", sensor_data.timestamp);

    if ((sensor_data.temperature != 300U) || (sensor_data.timestamp != 6666U))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
