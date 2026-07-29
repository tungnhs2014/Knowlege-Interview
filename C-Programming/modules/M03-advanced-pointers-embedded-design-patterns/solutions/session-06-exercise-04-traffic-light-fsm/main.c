#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Identifies the traffic-light states in table order.
 */
typedef enum
{
    RED = 0,
    GREEN,
    YELLOW,
    NUM_STATES
} traffic_state_t;

/**
 * @brief Defines the one compatible handler signature for every traffic state.
 *
 * @param[in] tick Current tick number.
 * @param[in,out] p_next_state Current state, updated by the handler.
 */
typedef void (*traffic_handler_t)(uint32_t tick, traffic_state_t *p_next_state);

/**
 * @brief Emits the source-defined message for the state visible on a tick.
 *
 * @param[in] state State displayed before any transition on this tick.
 * @param[in] tick Current tick number.
 */
static void print_traffic_message(traffic_state_t state, uint32_t tick)
{
    switch (state)
    {
        case RED:
            (void)printf("[RED]    Tick %u — Stop! Holding for 3 ticks.\n", (unsigned int)tick);
            break;

        case GREEN:
            (void)printf("[GREEN]  Tick %u — Go!  Holding for 3 ticks.\n", (unsigned int)tick);
            break;

        case YELLOW:
            (void)printf("[YELLOW] Tick %u — Slow down!\n", (unsigned int)tick);
            break;

        default:
            break;
    }
}

/**
 * @brief Executes the provided switch-based baseline without printing it twice.
 *
 * @param[in,out] p_state Current baseline state.
 * @param[out] p_displayed_state State that the source baseline would print.
 * @param[in] tick Current tick number.
 * @return true for a valid baseline state; otherwise false after recovery.
 */
static bool run_traffic_light_baseline(traffic_state_t *p_state,
                                       traffic_state_t *p_displayed_state,
                                       uint32_t tick)
{
    if ((p_state == NULL) || (p_displayed_state == NULL))
    {
        return false;
    }

    *p_displayed_state = *p_state;

    switch (*p_state)
    {
        case RED:
            if ((tick % 3U) == 0U)
            {
                *p_state = GREEN;
            }
            break;

        case GREEN:
            if ((tick % 3U) == 0U)
            {
                *p_state = YELLOW;
            }
            break;

        case YELLOW:
            *p_state = RED;
            break;

        default:
            *p_state = RED;
            return false;
    }

    return true;
}

/**
 * @brief Handles the RED state with the source baseline transition rule.
 *
 * @param[in] tick Current tick number.
 * @param[in,out] p_next_state State to update.
 */
static void State_Red(uint32_t tick, traffic_state_t *p_next_state)
{
    if (p_next_state == NULL)
    {
        return;
    }

    print_traffic_message(RED, tick);
    if ((tick % 3U) == 0U)
    {
        *p_next_state = GREEN;
    }
}

/**
 * @brief Handles the GREEN state with the source baseline transition rule.
 *
 * @param[in] tick Current tick number.
 * @param[in,out] p_next_state State to update.
 */
static void State_Green(uint32_t tick, traffic_state_t *p_next_state)
{
    if (p_next_state == NULL)
    {
        return;
    }

    print_traffic_message(GREEN, tick);
    if ((tick % 3U) == 0U)
    {
        *p_next_state = YELLOW;
    }
}

/**
 * @brief Handles the YELLOW state with the source baseline transition rule.
 *
 * @param[in] tick Current tick number.
 * @param[in,out] p_next_state State to update.
 */
static void State_Yellow(uint32_t tick, traffic_state_t *p_next_state)
{
    if (p_next_state == NULL)
    {
        return;
    }

    print_traffic_message(YELLOW, tick);
    *p_next_state = RED;
}

/**
 * @brief Maps each contiguous state identifier to a compatible handler.
 */
static traffic_handler_t const TrafficFSM[NUM_STATES] = {
    [RED] = State_Red,
    [GREEN] = State_Green,
    [YELLOW] = State_Yellow
};

/**
 * @brief Runs one refactored traffic FSM step with checked table dispatch.
 *
 * @param[in] tick Current tick number.
 * @param[in,out] p_state Current state, updated by the selected handler.
 */
void RunTrafficFSM(uint32_t tick, traffic_state_t *p_state)
{
    traffic_handler_t handler;
    size_t state_index;

    if (p_state == NULL)
    {
        return;
    }

    if ((*p_state < RED) || (*p_state >= NUM_STATES))
    {
        *p_state = RED;
        return;
    }

    state_index = (size_t)*p_state;
    handler = TrafficFSM[state_index];
    if (handler == NULL)
    {
        *p_state = RED;
        return;
    }

    handler(tick, p_state);
}

int main(void)
{
    traffic_state_t baseline_state = RED;
    traffic_state_t refactored_state = RED;
    traffic_state_t baseline_displayed;
    traffic_state_t refactored_displayed;
    traffic_state_t invalid_state = NUM_STATES;
    uint32_t tick;

    for (tick = 1U; tick <= 10U; ++tick)
    {
        if (!run_traffic_light_baseline(&baseline_state, &baseline_displayed, tick))
        {
            return EXIT_FAILURE;
        }

        refactored_displayed = refactored_state;
        RunTrafficFSM(tick, &refactored_state);

        if ((baseline_displayed != refactored_displayed) ||
            (baseline_state != refactored_state))
        {
            return EXIT_FAILURE;
        }

        if ((tick == 10U) && (refactored_displayed != GREEN))
        {
            return EXIT_FAILURE;
        }
    }

    RunTrafficFSM(1U, &invalid_state);
    if (invalid_state != RED)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
