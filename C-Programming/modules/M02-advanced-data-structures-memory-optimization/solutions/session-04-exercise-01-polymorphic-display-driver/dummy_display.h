#ifndef DUMMY_DISPLAY_H
#define DUMMY_DISPLAY_H

#include "i_display.h"

/**
 * @brief Provides the Dummy Display implementation of the display interface.
 */
extern i_display_t dummy_display;

/**
 * @brief Reports the number of draw requests accepted by the Dummy Display.
 *
 * @return Number of calls to the Dummy Display draw operation.
 */
uint32_t dummy_display_get_draw_count(void);

#endif
