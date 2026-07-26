#ifndef CONSOLE_DISPLAY_H
#define CONSOLE_DISPLAY_H

#include "i_display.h"

/**
 * @brief Returns the implementation-owned console configuration.
 *
 * The returned pointer remains valid for the program's lifetime. Each call
 * reuses the same static configuration and replaces its baud-rate value.
 *
 * @param[in] baud_rate Requested console baud-rate setting.
 *
 * @return Pointer to the static, opaque console configuration.
 */
display_config_t *console_config_create(uint32_t baud_rate);

/**
 * @brief Provides the Console Display implementation of the display interface.
 */
extern i_display_t console_display;

#endif
