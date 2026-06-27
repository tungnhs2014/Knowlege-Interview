#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_NULL_ARGUMENT,
    PARSE_EMPTY,
    PARSE_BAD_DIGIT,
    PARSE_OVERFLOW
} ParseStatus;

static const char* parse_status_text(ParseStatus status)
{
    switch (status) {
    case PARSE_OK:
        return "ok";
    case PARSE_NULL_ARGUMENT:
        return "null argument";
    case PARSE_EMPTY:
        return "empty input";
    case PARSE_BAD_DIGIT:
        return "bad digit";
    case PARSE_OVERFLOW:
        return "overflow";
    }
    return "unknown";
}

static ParseStatus parse_u8(const char* text, unsigned* out)
{
    unsigned value = 0;

    if (text == NULL || out == NULL) {
        return PARSE_NULL_ARGUMENT;
    }
    if (*text == '\0') {
        return PARSE_EMPTY;
    }

    for (const char* p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return PARSE_BAD_DIGIT;
        }
        value = value * 10u + (unsigned)(*p - '0');
        if (value > 255u) {
            return PARSE_OVERFLOW;
        }
    }

    *out = value;
    return PARSE_OK;
}

static int demonstrate_errno(void)
{
    FILE* file = fopen("missing-error-handling-demo.txt", "r");
    if (file == NULL) {
        int saved_errno = errno;
        printf("open-status=failed errno=%d text=%s\n",
               saved_errno,
               strerror(saved_errno));
        return 0;
    }

    fclose(file);
    return 1;
}

int main(void)
{
    unsigned value = 0;
    ParseStatus ok = parse_u8("42", &value);
    ParseStatus bad = parse_u8("999", &value);

    printf("parse-42=%s value=%u\n", parse_status_text(ok), value);
    printf("parse-999=%s\n", parse_status_text(bad));

    if (ok != PARSE_OK || value != 42u || bad != PARSE_OVERFLOW) {
        return 1;
    }

    if (demonstrate_errno() != 0) {
        return 1;
    }

    printf("result=passed\n");
    return 0;
}
