CC ?= cc
CXX ?= c++
LC_ALL := C
export LC_ALL

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Werror
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Werror
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
CXX_SANITIZER_FLAGS := $(SANITIZER_FLAGS) -fsanitize=vptr
BUILD_DIR := build

.PHONY: all check debug sanitize symbols clean polymorphism slicing c-dispatch

all: polymorphism slicing c-dispatch

check: all
	./$(BUILD_DIR)/polymorphism
	./$(BUILD_DIR)/slicing
	./$(BUILD_DIR)/c-dispatch
	$(MAKE) sanitize
	@echo "All Chapter 09 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

polymorphism: $(BUILD_DIR)/polymorphism
slicing: $(BUILD_DIR)/slicing
c-dispatch: $(BUILD_DIR)/c-dispatch

$(BUILD_DIR)/polymorphism: polymorphism.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/slicing: slicing.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/c-dispatch: c_dispatch.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

debug: all

sanitize: \
	$(BUILD_DIR)/sanitize-polymorphism \
	$(BUILD_DIR)/sanitize-slicing \
	$(BUILD_DIR)/sanitize-c-dispatch
	./$(BUILD_DIR)/sanitize-polymorphism
	./$(BUILD_DIR)/sanitize-slicing
	./$(BUILD_DIR)/sanitize-c-dispatch

$(BUILD_DIR)/sanitize-polymorphism: polymorphism.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXX_SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-slicing: slicing.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXX_SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-c-dispatch: c_dispatch.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) $< -o $@

symbols: $(BUILD_DIR)/polymorphism
	nm -C $(BUILD_DIR)/polymorphism | grep -E 'Sensor|Alarm'

clean:
	rm -rf $(BUILD_DIR)
