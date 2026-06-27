#include "status.h"

int system_status = 0;

static int normalize_status(int status)
{
    return status < 0 ? 0 : status;
}

void set_system_status(int status)
{
    system_status = normalize_status(status);
}

int get_system_status(void)
{
    return system_status;
}

unsigned int next_sequence(void)
{
    static unsigned int sequence = 0U;
    return ++sequence;
}

