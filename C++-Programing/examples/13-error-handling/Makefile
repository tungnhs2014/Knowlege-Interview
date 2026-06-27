CC ?= cc
CXX ?= g++
BUILD := build

CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
SAN_FLAGS := -O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer

C_TARGETS := c_status_errno
CXX_TARGETS := raii_exception_safety noexcept_move stream_states result_style
TARGETS := $(C_TARGETS) $(CXX_TARGETS)

.PHONY: all run check sanitize debug clean

all: $(TARGETS:%=$(BUILD)/%)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/c_status_errno: c_status_errno.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/%: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	for target in $(TARGETS); do ./$(BUILD)/$$target; done

check: run sanitize

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(filter-out -O2,$(CFLAGS)) $(SAN_FLAGS)" \
		CXXFLAGS="$(filter-out -O2,$(CXXFLAGS)) $(SAN_FLAGS)" all
	for target in $(TARGETS); do ./$(BUILD)/$$target; done

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(filter-out -O2,$(CFLAGS)) -O0 -g3" \
		CXXFLAGS="$(filter-out -O2,$(CXXFLAGS)) -O0 -g3" all

clean:
	rm -rf $(BUILD)
