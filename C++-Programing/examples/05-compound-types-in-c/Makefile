CC ?= cc
LC_ALL := C
export LC_ALL

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wformat=2 -Werror
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
UNSAFE_SANITIZER_FLAGS := -O1 -g3 -fsanitize=address \
	-fno-omit-frame-pointer
BUILD_DIR := build

.PHONY: all check arrays strings layout variant protocol debug sanitize \
	expected-failures expect-unterminated expect-flex-overflow clean

all: arrays strings layout variant protocol

check: all
	./$(BUILD_DIR)/arrays
	printf 'operator input\n' | ./$(BUILD_DIR)/strings
	./$(BUILD_DIR)/layout
	./$(BUILD_DIR)/variant
	./$(BUILD_DIR)/protocol
	$(MAKE) sanitize
	$(MAKE) expected-failures
	@echo "All Chapter 05 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

arrays: $(BUILD_DIR)/arrays

$(BUILD_DIR)/arrays: arrays/array_matrix.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

strings: $(BUILD_DIR)/strings

$(BUILD_DIR)/strings: strings/bounded_text.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

layout: $(BUILD_DIR)/layout

$(BUILD_DIR)/layout: layout/layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

variant: $(BUILD_DIR)/variant

$(BUILD_DIR)/variant: variant/tagged_value.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

protocol: $(BUILD_DIR)/protocol

$(BUILD_DIR)/protocol: protocol/message.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

sanitize: $(BUILD_DIR)/sanitize-arrays $(BUILD_DIR)/sanitize-strings \
	$(BUILD_DIR)/sanitize-layout $(BUILD_DIR)/sanitize-variant \
	$(BUILD_DIR)/sanitize-protocol
	./$(BUILD_DIR)/sanitize-arrays
	printf 'sanitized input\n' | ./$(BUILD_DIR)/sanitize-strings
	./$(BUILD_DIR)/sanitize-layout
	./$(BUILD_DIR)/sanitize-variant
	./$(BUILD_DIR)/sanitize-protocol

$(BUILD_DIR)/sanitize-arrays: arrays/array_matrix.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-strings: strings/bounded_text.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-layout: layout/layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-variant: variant/tagged_value.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-protocol: protocol/message.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-unterminated: unsafe/unterminated_string.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-flex-overflow: unsafe/flexible_underallocation.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

expected-failures: expect-unterminated expect-flex-overflow

expect-unterminated: $(BUILD_DIR)/unsafe-unterminated
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-unterminated \
		>$(BUILD_DIR)/failures/unterminated.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -Eq "AddressSanitizer.*stack-buffer-overflow" \
		$(BUILD_DIR)/failures/unterminated.log

expect-flex-overflow: $(BUILD_DIR)/unsafe-flex-overflow
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-flex-overflow \
		>$(BUILD_DIR)/failures/flexible-underallocation.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -Eq "AddressSanitizer.*heap-buffer-overflow" \
		$(BUILD_DIR)/failures/flexible-underallocation.log

clean:
	rm -rf $(BUILD_DIR)
