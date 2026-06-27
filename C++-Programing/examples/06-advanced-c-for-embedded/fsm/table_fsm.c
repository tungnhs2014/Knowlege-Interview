#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_FAULT
} State;

typedef enum {
    EVENT_START,
    EVENT_STOP,
    EVENT_FAILURE,
    EVENT_RESET
} Event;

typedef struct {
    State current;
    Event event;
    State next;
} Transition;

static const Transition transitions[] = {
    {STATE_IDLE, EVENT_START, STATE_RUNNING},
    {STATE_RUNNING, EVENT_STOP, STATE_IDLE},
    {STATE_RUNNING, EVENT_FAILURE, STATE_FAULT},
    {STATE_FAULT, EVENT_RESET, STATE_IDLE}
};

static bool next_state(State current, Event event, State *out_next)
{
    if (out_next == NULL) {
        return false;
    }

    for (size_t index = 0U;
         index < sizeof transitions / sizeof transitions[0];
         ++index) {
        if (transitions[index].current == current
            && transitions[index].event == event) {
            *out_next = transitions[index].next;
            return true;
        }
    }

    return false;
}

static const char *state_name(State state)
{
    switch (state) {
    case STATE_IDLE:
        return "idle";
    case STATE_RUNNING:
        return "running";
    case STATE_FAULT:
        return "fault";
    default:
        return "invalid";
    }
}

int main(void)
{
    const Event events[] = {
        EVENT_START,
        EVENT_FAILURE,
        EVENT_RESET,
        EVENT_STOP
    };
    State state = STATE_IDLE;

    for (size_t index = 0U;
         index < sizeof events / sizeof events[0];
         ++index) {
        State next;
        bool accepted = next_state(state, events[index], &next);

        printf("%s event=%u %s",
               state_name(state),
               (unsigned int)events[index],
               accepted ? "accepted" : "rejected");
        if (accepted) {
            state = next;
            printf(" next=%s", state_name(state));
        }
        putchar('\n');
    }

    return state == STATE_IDLE ? 0 : 1;
}
