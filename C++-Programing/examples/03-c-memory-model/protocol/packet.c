#include <stddef.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct LayoutSample {
    char status;
    int value;
    char quality;
};

struct PacketView {
    uint16_t payload_size;
    unsigned char type;
    unsigned char flags;
    const unsigned char *payload;
};

static int parse_packet(const unsigned char *data,
                        size_t size,
                        struct PacketView *out)
{
    uint16_t payload_size;

    if (data == NULL || out == NULL || size < 4U) {
        return 0;
    }

    payload_size = (uint16_t)(((uint16_t)data[0] << 8)
                            | (uint16_t)data[1]);

    if ((size_t)payload_size != size - 4U) {
        return 0;
    }

    out->payload_size = payload_size;
    out->type = data[2];
    out->flags = data[3];
    out->payload = data + 4U;
    return 1;
}

static void print_representation(void)
{
    const uint32_t value = UINT32_C(0x01020304);
    const unsigned char *bytes = (const unsigned char *)&value;

    printf("uint32_t bytes:");
    for (size_t index = 0U; index < sizeof value; ++index) {
        printf(" %02X", bytes[index]);
    }
    putchar('\n');

    if (bytes[0] == 0x04U) {
        puts("observed byte order: little-endian");
    } else if (bytes[0] == 0x01U) {
        puts("observed byte order: big-endian");
    } else {
        puts("observed byte order: another representation");
    }
}

int main(void)
{
    const unsigned char packet[] = {
        0x00U, 0x03U, 0x21U, 0x80U, 'A', 'B', 'C'
    };
    struct PacketView view;

    printf("LayoutSample: size=%zu alignment=%zu offsets=%zu,%zu,%zu\n",
           sizeof(struct LayoutSample),
           alignof(struct LayoutSample),
           offsetof(struct LayoutSample, status),
           offsetof(struct LayoutSample, value),
           offsetof(struct LayoutSample, quality));

    print_representation();

    if (!parse_packet(packet, sizeof packet, &view)) {
        fputs("packet rejected\n", stderr);
        return EXIT_FAILURE;
    }

    printf("packet: payload=%u type=0x%02X flags=0x%02X data=",
           (unsigned int)view.payload_size,
           (unsigned int)view.type,
           (unsigned int)view.flags);

    for (size_t index = 0U; index < view.payload_size; ++index) {
        putchar((int)view.payload[index]);
    }
    putchar('\n');

    return EXIT_SUCCESS;
}
