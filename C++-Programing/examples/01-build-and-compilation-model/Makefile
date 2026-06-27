CXX = g++
CC = gcc
AR = ar

CPPFLAGS :=
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror
SANITIZERS := -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD := build

.PHONY: all check stages separate static shared mixed inline debug sanitize
.PHONY: inspect expected-failures expect-missing-definition
.PHONY: expect-static-order-failure expect-loader-failure clean

all: separate static shared mixed inline

check: stages all debug sanitize expected-failures
	$(BUILD)/stages/app
	$(BUILD)/separate/app
	$(BUILD)/static/app
	LD_LIBRARY_PATH=$(BUILD)/shared $(BUILD)/shared/app
	$(BUILD)/mixed/app
	$(BUILD)/inline/app
	$(BUILD)/debug/app
	$(BUILD)/sanitize/app
	@echo "All Chapter 01 examples passed."

stages: $(BUILD)/stages/main.ii $(BUILD)/stages/main.s \
	$(BUILD)/stages/main.o $(BUILD)/stages/sensor.o $(BUILD)/stages/app

$(BUILD)/stages/main.ii: cpp/main.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -E $< -o $@

$(BUILD)/stages/main.s: cpp/main.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -S $< -o $@

$(BUILD)/stages/main.o: cpp/main.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/stages/sensor.o: cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/stages/app: $(BUILD)/stages/main.o $(BUILD)/stages/sensor.o
	$(CXX) $^ -o $@

separate: $(BUILD)/separate/app

$(BUILD)/separate/main.o: cpp/main.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/separate/sensor.o: cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/separate/app: $(BUILD)/separate/main.o $(BUILD)/separate/sensor.o
	$(CXX) $^ -Wl,-Map,$(@D)/app.map -o $@

static: $(BUILD)/static/app

$(BUILD)/static/main.o: cpp/main.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/static/sensor.o: cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/static/libsensor.a: $(BUILD)/static/sensor.o
	$(AR) rcs $@ $<

$(BUILD)/static/app: $(BUILD)/static/main.o $(BUILD)/static/libsensor.a
	$(CXX) $(BUILD)/static/main.o -L$(BUILD)/static -lsensor -o $@

shared: $(BUILD)/shared/app

$(BUILD)/shared/sensor.o: cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $< -o $@

$(BUILD)/shared/libch01sensor.so.1.0: $(BUILD)/shared/sensor.o
	$(CXX) -shared $< -Wl,-soname,libch01sensor.so.1 -o $@
	ln -sfn libch01sensor.so.1.0 $(BUILD)/shared/libch01sensor.so.1
	ln -sfn libch01sensor.so.1 $(BUILD)/shared/libch01sensor.so

$(BUILD)/shared/app: cpp/main.cpp cpp/sensor.hpp \
	$(BUILD)/shared/libch01sensor.so.1.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) cpp/main.cpp \
		-L$(BUILD)/shared -lch01sensor -o $@

mixed: $(BUILD)/mixed/app

$(BUILD)/mixed/sensor_c.o: mixed/sensor_c.c mixed/sensor_c.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/mixed/main.o: mixed/main.cpp mixed/sensor_c.h
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/mixed/app: $(BUILD)/mixed/main.o $(BUILD)/mixed/sensor_c.o
	$(CXX) $^ -o $@

inline: $(BUILD)/inline/app

$(BUILD)/inline/left.o: inline/left.cpp inline/math_utils.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/inline/right.o: inline/right.cpp inline/math_utils.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/inline/main.o: inline/main.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/inline/app: $(BUILD)/inline/main.o $(BUILD)/inline/left.o \
	$(BUILD)/inline/right.o
	$(CXX) $^ -o $@

debug: $(BUILD)/debug/app

$(BUILD)/debug/app: cpp/main.cpp cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O0 -g cpp/main.cpp cpp/sensor.cpp -o $@

sanitize: $(BUILD)/sanitize/app

$(BUILD)/sanitize/app: cpp/main.cpp cpp/sensor.cpp cpp/sensor.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O1 -g $(SANITIZERS) \
		cpp/main.cpp cpp/sensor.cpp -o $@

inspect: separate shared
	nm -C $(BUILD)/separate/app
	readelf -S $(BUILD)/separate/app
	readelf -Ws $(BUILD)/separate/app
	readelf -r $(BUILD)/separate/main.o
	readelf -d $(BUILD)/shared/app

expected-failures: expect-missing-definition expect-static-order-failure \
	expect-loader-failure
	@echo "All intentional failures were observed."

expect-missing-definition: $(BUILD)/separate/main.o
	@mkdir -p $(BUILD)/failures
	@if $(CXX) $< -o $(BUILD)/failures/missing-definition \
		>$(BUILD)/failures/missing-definition.log 2>&1; then \
		echo "ERROR: missing-definition link unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -q "undefined reference" \
		$(BUILD)/failures/missing-definition.log
	@echo "Observed expected missing-definition failure."

expect-static-order-failure: $(BUILD)/static/main.o \
	$(BUILD)/static/libsensor.a
	@mkdir -p $(BUILD)/failures
	@if $(CXX) -L$(BUILD)/static -lsensor $(BUILD)/static/main.o \
		-o $(BUILD)/failures/static-order \
		>$(BUILD)/failures/static-order.log 2>&1; then \
		echo "ERROR: wrong-order static link unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -q "undefined reference" $(BUILD)/failures/static-order.log
	@echo "Observed expected static-library order failure."

expect-loader-failure: $(BUILD)/shared/app
	@mkdir -p $(BUILD)/failures
	@if env -u LD_LIBRARY_PATH $(BUILD)/shared/app \
		>$(BUILD)/failures/loader.log 2>&1; then \
		echo "ERROR: loader test unexpectedly succeeded"; \
		exit 1; \
	fi
	@grep -Eq "shared librar|cannot open shared object" \
		$(BUILD)/failures/loader.log
	@echo "Observed expected runtime loader failure."

clean:
	rm -rf $(BUILD)
