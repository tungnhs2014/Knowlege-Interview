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

PROGRAMS := strings errors assertions resize fake logger
SOURCES := \
	strings/bounded_copy.c \
	errors/errno_translation.c \
	assertions/assert_vs_runtime.c \
	analysis/checked_resize.c \
	testing/fake_sensor.c \
	logging/bounded_logger.c

.PHONY: all check debug sanitize analyze ndebug clean $(PROGRAMS)

all: $(PROGRAMS)

check: all
	./$(BUILD_DIR)/strings
	./$(BUILD_DIR)/errors
	./$(BUILD_DIR)/assertions
	./$(BUILD_DIR)/resize
	./$(BUILD_DIR)/fake
	./$(BUILD_DIR)/logger
	$(MAKE) ndebug
	$(MAKE) analyze
	$(MAKE) sanitize
	@echo "All Chapter 07 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

strings: $(BUILD_DIR)/strings
errors: $(BUILD_DIR)/errors
assertions: $(BUILD_DIR)/assertions
resize: $(BUILD_DIR)/resize
fake: $(BUILD_DIR)/fake
logger: $(BUILD_DIR)/logger

$(BUILD_DIR)/strings: strings/bounded_copy.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/errors: errors/errno_translation.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/assertions: assertions/assert_vs_runtime.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/resize: analysis/checked_resize.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/fake: testing/fake_sensor.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/logger: logging/bounded_logger.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

ndebug: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) -DNDEBUG \
		assertions/assert_vs_runtime.c -o $(BUILD_DIR)/assertions-ndebug
	./$(BUILD_DIR)/assertions-ndebug

sanitize: $(addprefix $(BUILD_DIR)/sanitize-,$(PROGRAMS))
	./$(BUILD_DIR)/sanitize-strings
	./$(BUILD_DIR)/sanitize-errors
	./$(BUILD_DIR)/sanitize-assertions
	./$(BUILD_DIR)/sanitize-resize
	./$(BUILD_DIR)/sanitize-fake
	./$(BUILD_DIR)/sanitize-logger

$(BUILD_DIR)/sanitize-strings: strings/bounded_copy.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-errors: errors/errno_translation.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-assertions: assertions/assert_vs_runtime.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-resize: analysis/checked_resize.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-fake: testing/fake_sensor.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-logger: logging/bounded_logger.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

analyze:
	@set -e; \
	for source in $(SOURCES); do \
		echo "ANALYZE $$source"; \
		$(CC) $(CPPFLAGS) $(CFLAGS) -fanalyzer -c "$$source" -o /dev/null; \
	done

clean:
	rm -rf $(BUILD_DIR)
