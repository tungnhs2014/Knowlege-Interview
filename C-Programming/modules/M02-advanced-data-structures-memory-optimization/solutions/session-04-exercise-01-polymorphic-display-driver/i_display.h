#ifndef I_DISPLAY_H
#define I_DISPLAY_H

#include <stdint.h>

/**
 * @brief Declares the configuration type without exposing its representation.
 */
typedef struct display_config_s display_config_t;

/**
 * @brief Defines the operations available to display implementations.
 */
typedef struct i_display_s
{
    void (*init)(display_config_t *p_config);
    void (*draw_pixel)(uint16_t x, uint16_t y, uint8_t color);
} i_display_t;

#endif
