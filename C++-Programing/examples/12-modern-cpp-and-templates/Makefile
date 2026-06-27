CXX ?= g++
STD ?= c++20
WARN := -Wall -Wextra -Wpedantic -Wconversion -Wshadow
OPT ?= -O2
BUILD := build
TARGETS := move_logging lambda_lifetime ownership_callbacks vocabulary_types templates_concepts

all: $(TARGETS:%=$(BUILD)/%)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%: %.cpp | $(BUILD)
	$(CXX) -std=$(STD) $(WARN) $(OPT) $< -o $@

run: all
	for target in $(TARGETS); do ./$(BUILD)/$$target; done

check: run sanitize

sanitize:
	$(MAKE) clean
	$(MAKE) OPT="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" all
	for target in $(TARGETS); do ./$(BUILD)/$$target; done

debug:
	$(MAKE) clean
	$(MAKE) OPT="-O0 -g" all

clean:
	rm -rf $(BUILD)

.PHONY: all run check sanitize debug clean
