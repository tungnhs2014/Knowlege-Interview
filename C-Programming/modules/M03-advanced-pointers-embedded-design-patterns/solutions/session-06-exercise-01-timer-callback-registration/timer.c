#include "timer.h"

#include <stddef.h>
#include <stdio.h>

/**
 * @brief Holds private persistent state for the software timer module.
 */
typedef struct
{
    uint32_t expire_at_tick;
    uint32_t current_tick;
    timer_callback_t on_expire;
    bool is_running;
} timer_state_t;

/** @brief Owns the module's one active timer. */
static timer_state_t s_timer = { 0U, 0U, NULL, false };

/**
 * @brief Clears every member of the private timer state.
 */
static void clear_timer_state(void)
{
    s_timer.expire_at_tick = 0U;
    s_timer.current_tick = 0U;
    s_timer.on_expire = NULL;
    s_timer.is_running = false;
}

void Timer_Register(uint32_t expire_at_tick, timer_callback_t callback)
{
    if (s_timer.is_running)
    {
        (void)puts("[WARN] Timer already running! Ignoring new registration.");
        return;
    }

    if (callback == NULL)
    {
        (void)puts("[WARN] Timer callback is NULL. Ignoring registration.");
        return;
    }

    s_timer.expire_at_tick = expire_at_tick;
    s_timer.current_tick = 0U;
    s_timer.on_expire = callback;
    s_timer.is_running = true;
}

void Timer_Tick(void)
{
    timer_callback_t callback;

    if (!s_timer.is_running)
    {
        return;
    }

    ++s_timer.current_tick;
    if (s_timer.current_tick != s_timer.expire_at_tick)
    {
        return;
    }

    callback = s_timer.on_expire;
    if (callback != NULL)
    {
        callback();
    }

    s_timer.is_running = false;
}

void Timer_Reset(void)
{
    clear_timer_state();
}

bool Timer_IsRunning(void)
{
    return s_timer.is_running;
}
