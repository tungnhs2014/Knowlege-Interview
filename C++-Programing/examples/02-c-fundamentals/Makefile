CC := gcc
LC_ALL := C
export LC_ALL

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion \
	-Wsign-conversion -Wshadow -Wformat=2 -Werror
SANITIZERS := -fsanitize=address,undefined -fno-omit-frame-pointer
UBSAN := -fsanitize=undefined -fno-sanitize-recover=undefined

BUILD := build
CC_FAMILY := $(shell $(CC) --version 2>/dev/null | head -n 1)

.PHONY: all check types storage control input debug sanitize
.PHONY: expected-failures expect-sign-warning expect-format-warning
.PHONY: expect-invalid-input expect-long-input expect-ubsan-overflow clean
.PHONY: check-toolchain

all: types storage control input

check: check-toolchain all debug sanitize expected-failures
	$(BUILD)/types
	$(BUILD)/storage
	$(BUILD)/control
	printf '42\n' | $(BUILD)/input
	$(BUILD)/debug-types
	$(BUILD)/sanitize-types
	@echo "All Chapter 02 examples passed."

check-toolchain:
	@if ! printf '%s\n' "$(CC_FAMILY)" | grep -Eq "gcc|GCC|clang|Clang"; then \
		echo "ERROR: make check requires GCC or Clang-compatible diagnostics"; \
		echo "Detected: $(CC_FAMILY)"; \
		exit 1; \
	fi

types: $(BUILD)/types

$(BUILD)/types: types/types.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

storage: $(BUILD)/storage

$(BUILD)/storage: storage/main.c storage/status.c storage/status.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) storage/main.c storage/status.c -o $@

control: $(BUILD)/control

$(BUILD)/control: control/control.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

input: $(BUILD)/input

$(BUILD)/input: input/parse_int.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

debug: $(BUILD)/debug-types

$(BUILD)/debug-types: types/types.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -O0 -g3 $< -o $@

sanitize: $(BUILD)/sanitize-types

$(BUILD)/sanitize-types: types/types.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -O1 -g $(SANITIZERS) $< -o $@

expected-failures: expect-sign-warning expect-format-warning \
	expect-invalid-input expect-long-input expect-ubsan-overflow
	@echo "All intentional Chapter 02 failures were observed."

expect-sign-warning: diagnostics/signed_compare.c
	@mkdir -p $(BUILD)/failures
	@if $(CC) -std=c17 -Wall -Wextra -Wpedantic \
		-Werror=sign-compare -fsyntax-only $< \
		>$(BUILD)/failures/signed-compare.log 2>&1; then \
		echo "ERROR: signed comparison unexpectedly compiled cleanly"; \
		exit 1; \
	fi
	@grep -Eq "sign-compare|different signedness" \
		$(BUILD)/failures/signed-compare.log
	@echo "Observed expected signed/unsigned comparison diagnostic."

expect-format-warning: diagnostics/format_mismatch.c
	@mkdir -p $(BUILD)/failures
	@if $(CC) -std=c17 -Wall -Wextra -Wpedantic \
		-Werror=format -fsyntax-only $< \
		>$(BUILD)/failures/format-mismatch.log 2>&1; then \
		echo "ERROR: format mismatch unexpectedly compiled cleanly"; \
		exit 1; \
	fi
	@grep -Eq "format|expects argument" \
		$(BUILD)/failures/format-mismatch.log
	@echo "Observed expected format-string diagnostic."

expect-invalid-input: $(BUILD)/input
	@mkdir -p $(BUILD)/failures
	@if printf '12x\n' | $< \
		>$(BUILD)/failures/invalid-input.log 2>&1; then \
		echo "ERROR: malformed input unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -q "invalid integer" $(BUILD)/failures/invalid-input.log
	@echo "Observed expected malformed-input rejection."

expect-long-input: $(BUILD)/input
	@mkdir -p $(BUILD)/failures
	@if printf '%080d\n' 1 | $< \
		>$(BUILD)/failures/long-input.log 2>&1; then \
		echo "ERROR: overlong input unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -q "input line too long" $(BUILD)/failures/long-input.log
	@echo "Observed expected overlong-input rejection."

expect-ubsan-overflow: unsafe/signed_overflow.c
	@mkdir -p $(BUILD)/failures
	$(CC) -std=c17 -Wall -Wextra -Wpedantic -Werror \
		-O1 -g $(UBSAN) $< -o $(BUILD)/failures/signed-overflow
	@if $(BUILD)/failures/signed-overflow overflow \
		>$(BUILD)/failures/signed-overflow.log 2>&1; then \
		echo "ERROR: UBSan overflow test unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -Eq "signed integer overflow|runtime error" \
		$(BUILD)/failures/signed-overflow.log
	@echo "Observed expected UBSan signed-overflow failure."

clean:
	rm -rf $(BUILD)
