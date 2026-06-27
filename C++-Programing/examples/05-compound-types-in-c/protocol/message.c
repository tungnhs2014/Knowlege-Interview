#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t type;
    size_t length;
    unsigned char payload[];
} Message;

typedef struct {
    uint8_t version;
    uint8_t flags;
    uint16_t payload_length;
} Header;

static Message *message_create(uint16_t type,
                               const unsigned char *payload,
                               size_t length)
{
    if ((payload == NULL && length != 0U)
        || length > SIZE_MAX - sizeof(Message)) {
        return NULL;
    }

    Message *message = malloc(sizeof(Message) + length);
    if (message == NULL) {
        return NULL;
    }

    message->type = type;
    message->length = length;
    if (length != 0U) {
        memcpy(message->payload, payload, length);
    }

    return message;
}

static Message *message_clone(const Message *source)
{
    if (source == NULL) {
        return NULL;
    }

    return message_create(source->type,
                          source->payload,
                          source->length);
}

static int decode_header(const unsigned char *bytes,
                         size_t size,
                         Header *out)
{
    if (bytes == NULL || out == NULL || size < 4U) {
        return 0;
    }

    out->version = bytes[0];
    out->flags = bytes[1];
    out->payload_length =
        (uint16_t)((uint16_t)bytes[2] << 8)
        | (uint16_t)bytes[3];
    return 1;
}

int main(void)
{
    static const unsigned char wire[] = {
        1U, 0x80U, 0U, 3U, 'A', 'B', 'C'
    };
    Header header;

    if (!decode_header(wire, sizeof wire, &header)
        || (size_t)header.payload_length != sizeof wire - 4U) {
        fputs("invalid wire message\n", stderr);
        return EXIT_FAILURE;
    }

    Message *message =
        message_create(7U, wire + 4U, header.payload_length);
    if (message == NULL) {
        fputs("allocation failed\n", stderr);
        return EXIT_FAILURE;
    }

    Message *copy = message_clone(message);
    if (copy == NULL) {
        free(message);
        fputs("clone failed\n", stderr);
        return EXIT_FAILURE;
    }

    printf("version=%u flags=0x%02X type=%u payload=",
           (unsigned int)header.version,
           (unsigned int)header.flags,
           (unsigned int)copy->type);
    fwrite(copy->payload, 1U, copy->length, stdout);
    putchar('\n');

    free(copy);
    free(message);
    return EXIT_SUCCESS;
}
