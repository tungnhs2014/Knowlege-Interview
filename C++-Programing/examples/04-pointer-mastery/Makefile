CC ?= cc
CXX ?= c++

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wformat=2
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wformat=2
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
UNSAFE_SANITIZER_FLAGS := -O1 -g3 -fsanitize=address \
	-fno-omit-frame-pointer
BUILD_DIR := build

.PHONY: all check basics api matrix cpp debug sanitize expected-failures \
	expect-stale-alias expect-one-past-write clean

all: basics api matrix cpp

check: all
	./$(BUILD_DIR)/basics
	./$(BUILD_DIR)/api
	./$(BUILD_DIR)/matrix
	./$(BUILD_DIR)/cpp-view
	$(MAKE) sanitize
	$(MAKE) expected-failures

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

basics: $(BUILD_DIR)/basics

$(BUILD_DIR)/basics: basics/pointer_array.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

api: $(BUILD_DIR)/api

$(BUILD_DIR)/api: api/output_callback.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

matrix: $(BUILD_DIR)/matrix

$(BUILD_DIR)/matrix: matrix/matrix_layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

cpp: $(BUILD_DIR)/cpp-view

$(BUILD_DIR)/cpp-view: cpp/bounded_view.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

sanitize: $(BUILD_DIR)/sanitize-basics $(BUILD_DIR)/sanitize-api \
	$(BUILD_DIR)/sanitize-matrix $(BUILD_DIR)/sanitize-cpp-view
	./$(BUILD_DIR)/sanitize-basics
	./$(BUILD_DIR)/sanitize-api
	./$(BUILD_DIR)/sanitize-matrix
	./$(BUILD_DIR)/sanitize-cpp-view

$(BUILD_DIR)/sanitize-basics: basics/pointer_array.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-api: api/output_callback.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-matrix: matrix/matrix_layout.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-cpp-view: cpp/bounded_view.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-stale-alias: unsafe/stale_alias.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/unsafe-one-past-write: unsafe/one_past_write.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(UNSAFE_SANITIZER_FLAGS) $< -o $@

expected-failures: expect-stale-alias expect-one-past-write

expect-stale-alias: $(BUILD_DIR)/unsafe-stale-alias
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	LC_ALL=C ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-stale-alias \
		>$(BUILD_DIR)/failures/stale-alias.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -q "AddressSanitizer.*heap-use-after-free" \
		$(BUILD_DIR)/failures/stale-alias.log

expect-one-past-write: $(BUILD_DIR)/unsafe-one-past-write
	mkdir -p $(BUILD_DIR)/failures
	@set +e; \
	LC_ALL=C ASAN_OPTIONS=halt_on_error=1 \
		./$(BUILD_DIR)/unsafe-one-past-write \
		>$(BUILD_DIR)/failures/one-past-write.log 2>&1; \
	status=$$?; \
	set -e; \
	test $$status -ne 0; \
	grep -q "AddressSanitizer.*stack-buffer-overflow" \
		$(BUILD_DIR)/failures/one-past-write.log

clean:
	rm -rf $(BUILD_DIR)
