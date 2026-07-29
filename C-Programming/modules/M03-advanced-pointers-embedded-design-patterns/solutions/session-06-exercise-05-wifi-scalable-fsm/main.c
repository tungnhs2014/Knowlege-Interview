#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Identifies the contiguous WiFi FSM states in table order.
 */
typedef enum
{
    WIFI_INIT = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR,
    WIFI_MAX_STATES
} WIFI_STATE;

/**
 * @brief Defines the one compatible handler signature for every WiFi state.
 *
 * @param[in] input Current FSM input.
 * @param[in,out] p_next_state Current state, updated by the handler.
 * @return 0U after valid dispatch, or 0xFFU when dispatch is unsafe.
 */
typedef uint32_t (*wifi_handler_t)(uint8_t input, WIFI_STATE *p_next_state);

/** @brief Owns the module-private, resettable retry count. */
static uint8_t s_retry_count = 0U;

/**
 * @brief Resets the retry count at a defined FSM lifecycle boundary.
 */
static void reset_retry_state(void)
{
    s_retry_count = 0U;
}

/**
 * @brief Handles initialization and starts a fresh connection attempt.
 *
 * @param[in] input Current FSM input; INIT transitions regardless of its value.
 * @param[in,out] p_next_state State to update.
 * @return 0U on valid handling or 0xFFU for a null state pointer.
 */
static uint32_t wifi_state_init(uint8_t input, WIFI_STATE *p_next_state)
{
    (void)input;

    if (p_next_state == NULL)
    {
        return 0xFFU;
    }

    reset_retry_state();
    (void)puts("[INIT] Initializing... -> CONNECTING");
    *p_next_state = WIFI_CONNECTING;
    return 0U;
}

/**
 * @brief Handles a connection result and retry progression.
 *
 * @param[in] input One means connection success; zero means failed attempt.
 * @param[in,out] p_next_state State to update.
 * @return 0U on valid handling or 0xFFU for a null state pointer.
 */
static uint32_t wifi_state_connecting(uint8_t input, WIFI_STATE *p_next_state)
{
    if (p_next_state == NULL)
    {
        return 0xFFU;
    }

    if (input == 1U)
    {
        reset_retry_state();
        (void)puts("[CONNECTING] Connected! Retry count reset.");
        *p_next_state = WIFI_CONNECTED;
        return 0U;
    }

    ++s_retry_count;
    if (s_retry_count >= 3U)
    {
        (void)puts("[CONNECTING] Attempt 3 failed. -> ERROR");
        reset_retry_state();
        *p_next_state = WIFI_ERROR;
        return 0U;
    }

    (void)printf("[CONNECTING] Attempt %u failed. Retrying...\n",
                 (unsigned int)s_retry_count);
    *p_next_state = WIFI_CONNECTING;
    return 0U;
}

/**
 * @brief Handles connected operation or a link drop.
 *
 * @param[in] input One means link remains up; zero means link dropped.
 * @param[in,out] p_next_state State to update.
 * @return 0U on valid handling or 0xFFU for a null state pointer.
 */
static uint32_t wifi_state_connected(uint8_t input, WIFI_STATE *p_next_state)
{
    if (p_next_state == NULL)
    {
        return 0xFFU;
    }

    if (input == 0U)
    {
        reset_retry_state();
        (void)puts("[CONNECTED] Link dropped. Reconnecting...");
        *p_next_state = WIFI_CONNECTING;
        return 0U;
    }

    reset_retry_state();
    (void)puts("[CONNECTED] Online.");
    *p_next_state = WIFI_CONNECTED;
    return 0U;
}

/**
 * @brief Performs deterministic recovery from the error state.
 *
 * @param[in] input Current FSM input; ERROR recovers regardless of its value.
 * @param[in,out] p_next_state State to update.
 * @return 0U on valid handling or 0xFFU for a null state pointer.
 */
static uint32_t wifi_state_error(uint8_t input, WIFI_STATE *p_next_state)
{
    (void)input;

    if (p_next_state == NULL)
    {
        return 0xFFU;
    }

    reset_retry_state();
    (void)puts("[ERROR] Recovery. Restarting -> INIT");
    *p_next_state = WIFI_INIT;
    return 0U;
}

/**
 * @brief Maps each valid state identifier to a compatible WiFi state handler.
 */
static wifi_handler_t const WifiFSM[WIFI_MAX_STATES] = {
    [WIFI_INIT] = wifi_state_init,
    [WIFI_CONNECTING] = wifi_state_connecting,
    [WIFI_CONNECTED] = wifi_state_connected,
    [WIFI_ERROR] = wifi_state_error
};

/**
 * @brief Runs one checked WiFi FSM dispatch step.
 *
 * @param[in] input Current FSM input.
 * @param[in,out] p_state Current state, updated on valid dispatch.
 * @return 0U for valid dispatch, including retry steps that remain CONNECTING;
 *         otherwise 0xFFU after safe failure handling.
 */
uint32_t RunStateMachine(uint8_t input, WIFI_STATE *p_state)
{
    wifi_handler_t handler;
    size_t state_index;

    if (p_state == NULL)
    {
        return 0xFFU;
    }

    if ((*p_state < WIFI_INIT) || (*p_state >= WIFI_MAX_STATES))
    {
        *p_state = WIFI_INIT;
        return 0xFFU;
    }

    state_index = (size_t)*p_state;
    handler = WifiFSM[state_index];
    if (handler == NULL)
    {
        *p_state = WIFI_INIT;
        return 0xFFU;
    }

    return handler(input, p_state);
}

/**
 * @brief Returns a readable state name for the test trace.
 *
 * @param[in] state Valid state identifier.
 * @return Immutable state-name text.
 */
static const char *wifi_state_name(WIFI_STATE state)
{
    switch (state)
    {
        case WIFI_INIT:
            return "WIFI_INIT";

        case WIFI_CONNECTING:
            return "WIFI_CONNECTING";

        case WIFI_CONNECTED:
            return "WIFI_CONNECTED";

        case WIFI_ERROR:
            return "WIFI_ERROR";

        default:
            return "WIFI_INVALID";
    }
}

int main(void)
{
    static const uint8_t s_inputs[] = { 0U, 0U, 1U, 0U, 0U, 0U, 0U, 1U };
    static const WIFI_STATE s_expected_states[] = {
        WIFI_CONNECTING,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_CONNECTING,
        WIFI_CONNECTING,
        WIFI_CONNECTING,
        WIFI_ERROR,
        WIFI_INIT
    };
    WIFI_STATE state = WIFI_INIT;
    WIFI_STATE invalid_state = WIFI_MAX_STATES;
    size_t step;
    size_t step_count = sizeof(s_inputs) / sizeof(s_inputs[0]);

    for (step = 0U; step < step_count; ++step)
    {
        (void)printf("[Step %u] State: %s | input=%u\n",
                     (unsigned int)step,
                     wifi_state_name(state),
                     (unsigned int)s_inputs[step]);

        if (RunStateMachine(s_inputs[step], &state) != 0U)
        {
            return EXIT_FAILURE;
        }

        if (state != s_expected_states[step])
        {
            return EXIT_FAILURE;
        }
    }

    if (RunStateMachine(0U, NULL) != 0xFFU)
    {
        return EXIT_FAILURE;
    }

    if ((RunStateMachine(0U, &invalid_state) != 0xFFU) ||
        (invalid_state != WIFI_INIT))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
