CC := gcc
CXX := g++

CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O0 -g
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O0 -g
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all run sanitize strict clean

all: c_manual_array cpp_modern_replacements c_api_boundary

c_manual_array: c_manual_array.c
	$(CC) $(CFLAGS) $< -o $@

cpp_modern_replacements: cpp_modern_replacements.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

c_api_boundary: c_api_boundary.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	./c_manual_array
	./cpp_modern_replacements
	./c_api_boundary

sanitize:
	$(CC) $(CFLAGS) $(SANFLAGS) c_manual_array.c -o c_manual_array_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) cpp_modern_replacements.cpp -o cpp_modern_replacements_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) c_api_boundary.cpp -o c_api_boundary_san
	./c_manual_array_san
	./cpp_modern_replacements_san
	./c_api_boundary_san

strict:
	$(CC) $(CFLAGS) -Werror c_manual_array.c -o c_manual_array_strict
	$(CXX) $(CXXFLAGS) -Werror cpp_modern_replacements.cpp -o cpp_modern_replacements_strict
	$(CXX) $(CXXFLAGS) -Werror c_api_boundary.cpp -o c_api_boundary_strict

clean:
	rm -f c_manual_array cpp_modern_replacements c_api_boundary
	rm -f c_manual_array_san cpp_modern_replacements_san c_api_boundary_san
	rm -f c_manual_array_strict cpp_modern_replacements_strict c_api_boundary_strict

