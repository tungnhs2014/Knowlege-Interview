CC ?= cc
CXX ?= c++
LC_ALL := C
export LC_ALL

CPPFLAGS :=
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Werror
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wold-style-cast -Woverloaded-virtual -Werror
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -O1 -g3 -fsanitize=address,undefined \
	-fno-omit-frame-pointer
BUILD_DIR := build

.PHONY: all check debug sanitize symbols clean fundamentals lifetime c-api

all: fundamentals lifetime c-api

check: all
	./$(BUILD_DIR)/fundamentals
	./$(BUILD_DIR)/lifetime
	./$(BUILD_DIR)/c-api
	$(MAKE) sanitize
	@echo "All Chapter 08 examples passed."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

fundamentals: $(BUILD_DIR)/fundamentals
lifetime: $(BUILD_DIR)/lifetime
c-api: $(BUILD_DIR)/c-api

$(BUILD_DIR)/fundamentals: fundamentals.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/lifetime: lifetime.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@

$(BUILD_DIR)/counter.o: c_api/counter.cpp c_api/counter.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) -Ic_api -c $< -o $@

$(BUILD_DIR)/client.o: c_api/client.c c_api/counter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) -Ic_api -c $< -o $@

$(BUILD_DIR)/c-api: $(BUILD_DIR)/counter.o $(BUILD_DIR)/client.o
	$(CXX) $^ -o $@

debug: all

sanitize: \
	$(BUILD_DIR)/sanitize-fundamentals \
	$(BUILD_DIR)/sanitize-lifetime \
	$(BUILD_DIR)/sanitize-c-api
	./$(BUILD_DIR)/sanitize-fundamentals
	./$(BUILD_DIR)/sanitize-lifetime
	./$(BUILD_DIR)/sanitize-c-api

$(BUILD_DIR)/sanitize-fundamentals: fundamentals.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-lifetime: lifetime.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $< -o $@

$(BUILD_DIR)/sanitize-counter.o: c_api/counter.cpp c_api/counter.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) \
		-Ic_api -c $< -o $@

$(BUILD_DIR)/sanitize-client.o: c_api/client.c c_api/counter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) \
		-Ic_api -c $< -o $@

$(BUILD_DIR)/sanitize-c-api: \
	$(BUILD_DIR)/sanitize-counter.o \
	$(BUILD_DIR)/sanitize-client.o
	$(CXX) $(SANITIZER_FLAGS) $^ -o $@

symbols: $(BUILD_DIR)/counter.o
	nm $(BUILD_DIR)/counter.o
	nm $(BUILD_DIR)/counter.o | c++filt

clean:
	rm -rf $(BUILD_DIR)
