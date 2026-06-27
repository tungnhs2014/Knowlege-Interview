CC ?= cc

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wformat=2
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
UNSAFE_SANITIZER_FLAGS := -O1 -g3 -fsanitize=address \
	-fno-omit-frame-pointer
BUILD_DIR := build

.PHONY: all check layout allocation protocol inspect debug sanitize \
	expected-failures expect-use-after-free expect-out-of-bounds clean

all: layout allocation protocol

check: all
	./$(BUILD_DIR)/layout
	./$(BUILD_DIR)/allocation
	./$(BUILD_DIR)/protocol
	$(MAKE) sanitize
	./$(BUILD_DIR)/sanitize-layout
	./$(BUILD_DIR)/sanitize-allocation
	./$(BUILD_DIR)/sanitize-protocol
	$(MAKE) expected-failures

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

layout: $(BUILD_DIR)/layout

$(BUILD_DIR)/layout: layout/memory_layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

allocation: $(BUILD_DIR)/allocation

$(BUILD_DIR)/allocation: allocation/int_array.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

protocol: $(BUILD_DIR)/protocol

$(BUILD_DIR)/protocol: protocol/packet.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

inspect: layout
	size $(BUILD_DIR)/layout
	nm -S $(BUILD_DIR)/layout
	readelf -S -l -s $(BUILD_DIR)/layout
	objdump -h -t $(BUILD_DIR)/layout

debug: $(BUILD_DIR)/debug-layout $(BUILD_DIR)/debug-allocation \
	$(BUILD_DIR)/debug-protocol

$(BUILD_DIR)/debug-layout: layout/memory_layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/debug-allocation: allocation/int_array.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/debug-protocol: protocol/packet.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

sanitize: $(BUILD_DIR)/sanitize-layout $(BUILD_DIR)/sanitize-allocation \
	$(BUILD_DIR)/sanitize-protocol

$(BUILD_DIR)/sanitize-layout: layout/memory_layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-allocation: allocation/int_array.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-protocol: protocol/packet.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-use-after-free: unsafe/use_after_free.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-out-of-bounds: unsafe/out_of_bounds.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

expected-failures: expect-use-after-free expect-out-of-bounds

expect-use-after-free: $(BUILD_DIR)/unsafe-use-after-free
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	LC_ALL=C ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-use-after-free \
		>$(BUILD_DIR)/failures/use-after-free.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -q "AddressSanitizer.*heap-use-after-free" \
		$(BUILD_DIR)/failures/use-after-free.log

expect-out-of-bounds: $(BUILD_DIR)/unsafe-out-of-bounds
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	LC_ALL=C ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-out-of-bounds \
		>$(BUILD_DIR)/failures/out-of-bounds.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -q "AddressSanitizer.*heap-buffer-overflow" \
		$(BUILD_DIR)/failures/out-of-bounds.log

clean:
	rm -rf $(BUILD_DIR)
