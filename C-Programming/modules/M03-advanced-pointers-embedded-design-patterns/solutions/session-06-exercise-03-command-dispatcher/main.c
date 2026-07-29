#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Defines the compatible action signature for every command.
 */
typedef void (*cmd_action_t)(void);

/**
 * @brief Associates one immutable command string with one compatible action.
 */
typedef struct
{
    const char *p_command_str;
    cmd_action_t action;
} command_entry_t;

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/** @brief Counts known-command actions for the runtime self-check. */
static unsigned int s_action_count = 0U;

/** @brief Counts unknown commands for the runtime self-check. */
static unsigned int s_unknown_count = 0U;

/** @brief Counts null-input calls for the runtime self-check. */
static unsigned int s_null_input_count = 0U;

/** @brief Handles the LED_ON command. */
static void Cmd_LED_On(void)
{
    ++s_action_count;
    (void)puts("[CMD] LED turned ON.");
}

/** @brief Handles the LED_OFF command. */
static void Cmd_LED_Off(void)
{
    ++s_action_count;
    (void)puts("[CMD] LED turned OFF.");
}

/** @brief Handles the MOTOR_START command. */
static void Cmd_Motor_Start(void)
{
    ++s_action_count;
    (void)puts("[CMD] Motor started at 1500 RPM.");
}

/** @brief Handles the MOTOR_STOP command. */
static void Cmd_Motor_Stop(void)
{
    ++s_action_count;
    (void)puts("[CMD] Motor stopped.");
}

/** @brief Handles the STATUS command. */
static void Cmd_Status(void)
{
    ++s_action_count;
    (void)puts("[CMD] System status: OK.");
}

/**
 * @brief Contains the source-defined command vocabulary and handlers.
 *
 * Static const describes module-owned read-only data; physical placement is a
 * compiler, linker, and target decision rather than an ISO C guarantee.
 */
static const command_entry_t s_command_table[] = {
    { "LED_ON", Cmd_LED_On },
    { "LED_OFF", Cmd_LED_Off },
    { "MOTOR_START", Cmd_Motor_Start },
    { "MOTOR_STOP", Cmd_Motor_Stop },
    { "STATUS", Cmd_Status }
};

/**
 * @brief Dispatches an exact, null-terminated command string.
 *
 * @param[in] p_received_cmd Non-null, null-terminated command text.
 */
void Dispatch_Command(const char *p_received_cmd)
{
    size_t index;
    size_t command_count = ARRAY_SIZE(s_command_table);

    if (p_received_cmd == NULL)
    {
        ++s_null_input_count;
        return;
    }

    for (index = 0U; index < command_count; ++index)
    {
        const command_entry_t *p_entry = &s_command_table[index];

        if ((p_entry->p_command_str == NULL) || (p_entry->action == NULL))
        {
            return;
        }

        if (strcmp(p_received_cmd, p_entry->p_command_str) == 0)
        {
            p_entry->action();
            return;
        }
    }

    ++s_unknown_count;
    (void)printf("[CMD] Unknown command: %s\n", p_received_cmd);
}

int main(void)
{
    Dispatch_Command("LED_ON");
    Dispatch_Command("LED_OFF");
    Dispatch_Command("MOTOR_START");
    Dispatch_Command("MOTOR_STOP");
    Dispatch_Command("STATUS");
    Dispatch_Command("REBOOT");
    Dispatch_Command(NULL);

    if ((s_action_count != ARRAY_SIZE(s_command_table)) || (s_unknown_count != 1U) ||
        (s_null_input_count != 1U))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
