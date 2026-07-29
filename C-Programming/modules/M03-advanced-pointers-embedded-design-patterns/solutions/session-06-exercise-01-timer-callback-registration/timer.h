#ifndef M03_TIMER_H
#define M03_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Defines the callback invoked when a software timer expires.
 */
typedef void (*timer_callback_t)(void);

/**
 * @brief Registers one callback for an expiry tick when no timer is running.
 *
 * @param[in] expire_at_tick Tick at which the callback is invoked.
 * @param[in] callback Non-null callback to store.
 */
void Timer_Register(uint32_t expire_at_tick, timer_callback_t callback);

/**
 * @brief Advances the active software timer by one tick.
 */
void Timer_Tick(void);

/**
 * @brief Cancels the active timer and clears its private state.
 */
void Timer_Reset(void);

/**
 * @brief Reports whether a timer is currently running.
 *
 * @return true when an active timer is registered; otherwise false.
 */
bool Timer_IsRunning(void);

#endif
