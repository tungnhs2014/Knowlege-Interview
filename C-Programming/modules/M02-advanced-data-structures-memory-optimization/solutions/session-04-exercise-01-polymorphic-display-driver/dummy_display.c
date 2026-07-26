#include "dummy_display.h"

#include <stddef.h>

/** @brief Counts draw requests without exposing the counter for modification. */
static uint32_t s_dummy_draw_count;

/**
 * @brief Accepts a configuration without requiring display hardware.
 *
 * @param[in] p_config Pointer to an opaque display configuration.
 */
static void dummy_display_init(display_config_t *p_config)
{
    (void)p_config;
}

/**
 * @brief Records one draw request without emitting Console Display output.
 *
 * @param[in] x Horizontal pixel coordinate.
 * @param[in] y Vertical pixel coordinate.
 * @param[in] color Requested pixel color value.
 */
static void dummy_display_draw_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    (void)x;
    (void)y;
    (void)color;

    ++s_dummy_draw_count;
}

/**
 * @brief Reports the implementation-owned draw-request count.
 *
 * @return Number of Dummy Display draw requests observed.
 */
uint32_t dummy_display_get_draw_count(void)
{
    return s_dummy_draw_count;
}

/** @brief Exposes Dummy Display operations through the common interface. */
i_display_t dummy_display = {
    .init = dummy_display_init,
    .draw_pixel = dummy_display_draw_pixel
};
