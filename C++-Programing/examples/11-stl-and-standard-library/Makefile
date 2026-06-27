CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
DEBUGFLAGS := -O0 -g
OPTFLAGS ?= -O2
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
DEBUG_ITER_FLAGS := -D_GLIBCXX_DEBUG

BUILD_DIR := build

TARGETS := \
	$(BUILD_DIR)/container_selection \
	$(BUILD_DIR)/erase_remove \
	$(BUILD_DIR)/associative_lookup \
	$(BUILD_DIR)/algorithms_files \
	$(BUILD_DIR)/invalidation_demo

.PHONY: all check run sanitize debug debug-iter clean

all: $(TARGETS)

check: all run sanitize debug-iter

run: all
	./$(BUILD_DIR)/container_selection
	./$(BUILD_DIR)/erase_remove
	./$(BUILD_DIR)/associative_lookup
	./$(BUILD_DIR)/algorithms_files
	./$(BUILD_DIR)/invalidation_demo

sanitize:
	$(MAKE) clean
	$(MAKE) OPTFLAGS="$(DEBUGFLAGS) $(SANFLAGS)" all
	./$(BUILD_DIR)/container_selection
	./$(BUILD_DIR)/erase_remove
	./$(BUILD_DIR)/associative_lookup
	./$(BUILD_DIR)/algorithms_files
	./$(BUILD_DIR)/invalidation_demo

debug:
	$(MAKE) clean
	$(MAKE) OPTFLAGS="$(DEBUGFLAGS)" all

debug-iter:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) $(DEBUG_ITER_FLAGS)" OPTFLAGS="$(DEBUGFLAGS)" all
	./$(BUILD_DIR)/invalidation_demo

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/container_selection: container_selection.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(BUILD_DIR)/erase_remove: erase_remove.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(BUILD_DIR)/associative_lookup: associative_lookup.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(BUILD_DIR)/algorithms_files: algorithms_files.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(BUILD_DIR)/invalidation_demo: invalidation_demo.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
