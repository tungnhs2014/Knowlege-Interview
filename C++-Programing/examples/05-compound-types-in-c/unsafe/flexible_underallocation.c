#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t length;
    unsigned char payload[];
} Message;

int main(void)
{
    Message *message = malloc(sizeof *message);
    if (message == NULL) {
        return EXIT_FAILURE;
    }

    message->length = 1U;
    volatile unsigned char *payload = message->payload;
    payload[0] = 0x42U;

    free(message);
    return EXIT_SUCCESS;
}
