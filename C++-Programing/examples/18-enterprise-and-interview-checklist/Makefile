CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
TARGET := review_checklist_demo
SOURCES := review_checklist_demo.cpp

.PHONY: all run sanitize strict debug clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -O0 -g $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

sanitize: $(SOURCES)
	$(CXX) $(CXXFLAGS) -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -o $(TARGET)_san
	./$(TARGET)_san

strict: $(SOURCES)
	$(CXX) $(CXXFLAGS) -Werror -O0 -g $(SOURCES) -o $(TARGET)_strict

debug: $(SOURCES)
	$(CXX) $(CXXFLAGS) -O0 -g $(SOURCES) -o $(TARGET)_dbg

clean:
	rm -f $(TARGET) $(TARGET)_san $(TARGET)_strict $(TARGET)_dbg $(TARGET)_tsan
