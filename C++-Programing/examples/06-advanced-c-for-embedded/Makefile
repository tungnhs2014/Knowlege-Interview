CC ?= cc
LC_ALL := C
export LC_ALL

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wformat=2 -Werror
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
BUILD_DIR := build

PROGRAMS := bits macros callbacks fsm hal protocol ring

.PHONY: all check debug sanitize preprocess assembly clean $(PROGRAMS)

all: $(PROGRAMS)

check: all
	./$(BUILD_DIR)/bits
	./$(BUILD_DIR)/macros
	./$(BUILD_DIR)/callbacks
	./$(BUILD_DIR)/fsm
	./$(BUILD_DIR)/hal
	./$(BUILD_DIR)/protocol
	./$(BUILD_DIR)/ring
	$(MAKE) sanitize
	@echo "All Chapter 06 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

bits: $(BUILD_DIR)/bits
macros: $(BUILD_DIR)/macros
callbacks: $(BUILD_DIR)/callbacks
fsm: $(BUILD_DIR)/fsm
hal: $(BUILD_DIR)/hal
protocol: $(BUILD_DIR)/protocol
ring: $(BUILD_DIR)/ring

$(BUILD_DIR)/bits: bits/bit_masks.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/macros: macros/macro_vs_inline.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/callbacks: callbacks/callback_context.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/fsm: fsm/table_fsm.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/hal: hal/fakeable_hal.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/protocol: protocol/endian_codec.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/ring: ring_buffer/ring_buffer.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

sanitize: $(addprefix $(BUILD_DIR)/sanitize-,$(PROGRAMS))
	./$(BUILD_DIR)/sanitize-bits
	./$(BUILD_DIR)/sanitize-macros
	./$(BUILD_DIR)/sanitize-callbacks
	./$(BUILD_DIR)/sanitize-fsm
	./$(BUILD_DIR)/sanitize-hal
	./$(BUILD_DIR)/sanitize-protocol
	./$(BUILD_DIR)/sanitize-ring

$(BUILD_DIR)/sanitize-bits: bits/bit_masks.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-macros: macros/macro_vs_inline.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-callbacks: callbacks/callback_context.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-fsm: fsm/table_fsm.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-hal: hal/fakeable_hal.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-protocol: protocol/endian_codec.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-ring: ring_buffer/ring_buffer.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

preprocess: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -std=c17 -E macros/macro_vs_inline.c \
		-o $(BUILD_DIR)/macro_vs_inline.i

assembly: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -O2 -S bits/bit_masks.c \
		-o $(BUILD_DIR)/bit_masks.s

clean:
	rm -rf $(BUILD_DIR)
