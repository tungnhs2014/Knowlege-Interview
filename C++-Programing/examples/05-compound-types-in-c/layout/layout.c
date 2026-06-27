#include <stddef.h>
#include <stdalign.h>
#include <stdio.h>

struct Unordered {
    char code;
    int value;
    char state;
};

struct Grouped {
    int value;
    char code;
    char state;
};

static void print_unordered_layout(void)
{
    printf("Unordered: size=%zu alignment=%zu offsets=%zu,%zu,%zu\n",
           sizeof(struct Unordered),
           alignof(struct Unordered),
           offsetof(struct Unordered, code),
           offsetof(struct Unordered, value),
           offsetof(struct Unordered, state));
}

static void print_grouped_layout(void)
{
    printf("Grouped: size=%zu alignment=%zu offsets=%zu,%zu,%zu\n",
           sizeof(struct Grouped),
           alignof(struct Grouped),
           offsetof(struct Grouped, value),
           offsetof(struct Grouped, code),
           offsetof(struct Grouped, state));
}

int main(void)
{
    struct Unordered first = {
        .code = 'A',
        .value = 42,
        .state = 'R'
    };
    struct Unordered second = first;

    print_unordered_layout();
    print_grouped_layout();

    printf("semantic-equal=%s\n",
           first.code == second.code
               && first.value == second.value
               && first.state == second.state
               ? "yes"
               : "no");

    return 0;
}
