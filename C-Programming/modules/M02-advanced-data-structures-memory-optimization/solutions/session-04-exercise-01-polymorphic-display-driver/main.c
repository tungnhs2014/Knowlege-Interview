#include "console_display.h"
#include "dummy_display.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Requests the source-required two-by-two rectangle through an interface.
 *
 * @param[in] p_display Display interface used for every draw request.
 */
static void draw_rectangle(i_display_t *p_display)
{
    if ((p_display == NULL) || (p_display->draw_pixel == NULL))
    {
        return;
    }

    p_display->draw_pixel(0U, 0U, 1U);
    p_display->draw_pixel(1U, 0U, 1U);
    p_display->draw_pixel(0U, 1U, 1U);
    p_display->draw_pixel(1U, 1U, 1U);
}

/**
 * @brief Demonstrates Console and Dummy Display implementations.
 *
 * @return EXIT_SUCCESS when the Dummy Display observes four requests;
 *         otherwise EXIT_FAILURE.
 */
int main(void)
{
    display_config_t *p_config = console_config_create(115200U);

    if ((p_config == NULL) || (console_display.init == NULL) ||
        (dummy_display.init == NULL))
    {
        return EXIT_FAILURE;
    }

    console_display.init(p_config);
    draw_rectangle(&console_display);

    dummy_display.init(p_config);
    draw_rectangle(&dummy_display);

    if (dummy_display_get_draw_count() != 4U)
    {
        return EXIT_FAILURE;
    }

    (void)printf("Dummy display was called %u times.\n",
                 (unsigned int)dummy_display_get_draw_count());

    return EXIT_SUCCESS;
}
