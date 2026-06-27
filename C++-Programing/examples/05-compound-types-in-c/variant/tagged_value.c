#include <stddef.h>
#include <stdio.h>

typedef enum {
    VALUE_INTEGER,
    VALUE_REAL,
    VALUE_TEXT
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        int integer;
        double real;
        struct {
            const char *data;
            size_t length;
        } text;
    } data;
} Value;

static Value value_from_integer(int integer)
{
    Value value = {
        .kind = VALUE_INTEGER,
        .data.integer = integer
    };
    return value;
}

static Value value_from_real(double real)
{
    Value value = {
        .kind = VALUE_REAL,
        .data.real = real
    };
    return value;
}

static Value value_from_text(const char *data, size_t length)
{
    Value value = {
        .kind = VALUE_TEXT,
        .data.text = {
            .data = data,
            .length = length
        }
    };
    return value;
}

static int print_value(const Value *value)
{
    if (value == NULL) {
        return 0;
    }

    switch (value->kind) {
    case VALUE_INTEGER:
        printf("integer=%d\n", value->data.integer);
        return 1;
    case VALUE_REAL:
        printf("real=%.2f\n", value->data.real);
        return 1;
    case VALUE_TEXT:
        if (value->data.text.data == NULL
            && value->data.text.length != 0U) {
            return 0;
        }

        fputs("text=", stdout);
        if (value->data.text.length != 0U) {
            fwrite(value->data.text.data,
                   1U,
                   value->data.text.length,
                   stdout);
        }
        putchar('\n');
        return 1;
    default:
        return 0;
    }
}

int main(void)
{
    static const char label[] = {'r', 'e', 'a', 'd', 'y'};
    const Value values[] = {
        value_from_integer(42),
        value_from_real(3.5),
        value_from_text(label, sizeof label)
    };

    for (size_t index = 0U;
         index < sizeof values / sizeof values[0];
         ++index) {
        if (!print_value(&values[index])) {
            fputs("invalid value\n", stderr);
            return 1;
        }
    }

    return 0;
}
