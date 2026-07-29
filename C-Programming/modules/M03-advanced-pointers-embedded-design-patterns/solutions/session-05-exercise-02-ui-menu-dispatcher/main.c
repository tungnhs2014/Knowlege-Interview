#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#define M03_SECTION(section_name) __attribute__((section(section_name)))
#else
#error "This solution requires a GCC/Clang-family section attribute."
#endif

/**
 * @brief Defines a compatible UI menu-handler signature.
 *
 * @param[in] page_id Page identifier supplied by the dispatcher.
 */
typedef void (*ui_handler_t)(uint8_t page_id);

/** @brief Counts calls to the main-menu handler. */
static uint8_t s_main_menu_calls = 0U;

/** @brief Counts calls to the settings-menu handler. */
static uint8_t s_settings_menu_calls = 0U;

/** @brief Counts calls to the about-menu handler. */
static uint8_t s_about_menu_calls = 0U;

/** @brief Counts invalid dispatch attempts. */
static uint8_t s_invalid_dispatches = 0U;

/**
 * @brief Draws the main menu.
 *
 * @param[in] page_id Page identifier; retained for the common handler type.
 */
static void draw_menu(uint8_t page_id)
{
    (void)page_id;
    ++s_main_menu_calls;
    (void)puts("Drawing Main Menu...");
}

/**
 * @brief Draws the settings menu.
 *
 * @param[in] page_id Page identifier; retained for the common handler type.
 */
static void draw_settings(uint8_t page_id)
{
    (void)page_id;
    ++s_settings_menu_calls;
    (void)puts("Drawing Settings Menu...");
}

/**
 * @brief Draws the about menu.
 *
 * @param[in] page_id Page identifier; retained for the common handler type.
 */
static void draw_about(uint8_t page_id)
{
    (void)page_id;
    ++s_about_menu_calls;
    (void)puts("Drawing About Menu...");
}

/**
 * @brief Contains the selectable UI menu handlers.
 *
 * The GNU-family attribute requests an input section. Final target placement is
 * determined by the selected linker script and target memory map.
 */
static ui_handler_t const s_menu_handlers[] M03_SECTION(".my_dispatch_table") = {
    draw_menu,
    draw_settings,
    draw_about
};

/**
 * @brief Dispatches a menu index after validating the table access and entry.
 *
 * @param[in] menu_index Requested menu index.
 */
void dispatch_ui(uint8_t menu_index)
{
    size_t handler_count = sizeof(s_menu_handlers) / sizeof(s_menu_handlers[0]);
    ui_handler_t handler;

    if ((size_t)menu_index >= handler_count)
    {
        ++s_invalid_dispatches;
        (void)puts("Error: Invalid menu index!");
        return;
    }

    handler = s_menu_handlers[menu_index];
    if (handler == NULL)
    {
        ++s_invalid_dispatches;
        (void)puts("Error: Invalid menu index!");
        return;
    }

    handler(menu_index);
}

int main(void)
{
    dispatch_ui(0U);
    dispatch_ui(1U);
    dispatch_ui(2U);
    dispatch_ui(3U);

    if ((s_main_menu_calls != 1U) || (s_settings_menu_calls != 1U) ||
        (s_about_menu_calls != 1U) || (s_invalid_dispatches != 1U))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
