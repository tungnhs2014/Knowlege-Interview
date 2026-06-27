CXX ?= c++
LC_ALL := C
export LC_ALL

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wold-style-cast -Wnon-virtual-dtor -Werror
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
BUILD_DIR := build
SOURCES := rule_of_zero handle_raii ownership shared_weak

.PHONY: all check debug sanitize clean

all: $(SOURCES:%=$(BUILD_DIR)/%)

check: all
	@for program in $(SOURCES); do ./$(BUILD_DIR)/$$program; done
	$(MAKE) sanitize
	@echo "All Chapter 10 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

sanitize: $(SOURCES:%=$(BUILD_DIR)/sanitize-%)
	@for program in $(SOURCES); do ./$(BUILD_DIR)/sanitize-$$program; done

$(BUILD_DIR)/sanitize-%: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZER_FLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
