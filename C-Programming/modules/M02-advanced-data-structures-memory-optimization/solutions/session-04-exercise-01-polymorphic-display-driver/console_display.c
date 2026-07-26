#include "console_display.h"

#include <stdio.h>

/**
 * @brief Defines the Console Display configuration representation privately.
 */
struct display_config_s
{
    uint32_t baud_rate;
};

/** @brief Holds the single static Console Display configuration. */
static struct display_config_s s_console_config;

/**
 * @brief Initializes the Console Display for a valid configuration.
 *
 * @param[in] p_config Pointer to the opaque Console Display configuration.
 */
static void console_display_init(display_config_t *p_config)
{
    if (p_config == NULL)
    {
        return;
    }

    /* The type is complete here; no opaque-pointer cast is necessary. */
    (void)p_config->baud_rate;
}

/**
 * @brief Simulates drawing one pixel on the Console Display.
 *
 * @param[in] x Horizontal pixel coordinate.
 * @param[in] y Vertical pixel coordinate.
 * @param[in] color Source-defined pixel color value.
 */
static void console_display_draw_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    (void)printf("[Console] Drawing pixel at (%u,%u) with color %u\n",
                 (unsigned int)x,
                 (unsigned int)y,
                 (unsigned int)color);
}

/**
 * @brief Provides the static Console Display configuration.
 *
 * @param[in] baud_rate Requested console baud-rate setting.
 *
 * @return Pointer to the static, opaque configuration.
 */
display_config_t *console_config_create(uint32_t baud_rate)
{
    s_console_config.baud_rate = baud_rate;

    return &s_console_config;
}

/** @brief Exposes Console Display operations through the common interface. */
i_display_t console_display = {
    .init = console_display_init,
    .draw_pixel = console_display_draw_pixel
};
