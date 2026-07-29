#include "timer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/** @brief Counts first-alarm invocations for the runtime self-check. */
static uint32_t s_first_alarm_count = 0U;

/** @brief Counts second-alarm invocations for the runtime self-check. */
static uint32_t s_second_alarm_count = 0U;

/**
 * @brief Records and reports the first alarm.
 */
static void first_alarm(void)
{
    ++s_first_alarm_count;
    (void)puts("[ALARM] Timer fired at tick 5!");
}

/**
 * @brief Records and reports the second alarm.
 */
static void second_alarm(void)
{
    ++s_second_alarm_count;
    (void)puts("[ALARM] Second alarm fired at tick 3!");
}

/**
 * @brief Advances and reports one test tick.
 *
 * @param[in] tick Tick number shown by the test harness.
 */
static void run_tick(uint32_t tick)
{
    (void)printf("Tick %u...\n", (unsigned int)tick);
    Timer_Tick();
}

int main(void)
{
    uint32_t tick;

    (void)puts("--- Test 1: Alarm at tick 5, run for 10 ticks ---");
    Timer_Register(5U, first_alarm);

    for (tick = 1U; tick <= 10U; ++tick)
    {
        run_tick(tick);

        if (tick == 2U)
        {
            Timer_Register(3U, second_alarm);
        }
    }

    if ((s_first_alarm_count != 1U) || (s_second_alarm_count != 0U) ||
        Timer_IsRunning())
    {
        return EXIT_FAILURE;
    }

    (void)puts("\n--- Test 2: Reset, then new alarm at tick 3 ---");
    Timer_Reset();
    (void)puts("[TIMER] Reset.");
    Timer_Register(3U, second_alarm);

    for (tick = 1U; tick <= 3U; ++tick)
    {
        run_tick(tick);
    }

    if ((s_second_alarm_count != 1U) || Timer_IsRunning())
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
