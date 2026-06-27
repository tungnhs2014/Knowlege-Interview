CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -O2 -pthread
DBGFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -g -O1 -pthread

PROGRAMS := race_counter mutex_atomic_counter producer_consumer future_exception signal_shutdown scoped_lock_deadlock_fix

.PHONY: all clean tsan asan run-safe

all: $(PROGRAMS)

race_counter: race_counter.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

mutex_atomic_counter: mutex_atomic_counter.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

producer_consumer: producer_consumer.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

future_exception: future_exception.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

signal_shutdown: signal_shutdown.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

scoped_lock_deadlock_fix: scoped_lock_deadlock_fix.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

tsan:
	$(CXX) $(DBGFLAGS) -fsanitize=thread race_counter.cpp -o race_counter_tsan
	$(CXX) $(DBGFLAGS) -fsanitize=thread mutex_atomic_counter.cpp -o mutex_atomic_counter_tsan
	$(CXX) $(DBGFLAGS) -fsanitize=thread producer_consumer.cpp -o producer_consumer_tsan

asan:
	$(CXX) $(DBGFLAGS) -fsanitize=address,undefined producer_consumer.cpp -o producer_consumer_asan
	$(CXX) $(DBGFLAGS) -fsanitize=address,undefined future_exception.cpp -o future_exception_asan
	$(CXX) $(DBGFLAGS) -fsanitize=address,undefined signal_shutdown.cpp -o signal_shutdown_asan

run-safe: mutex_atomic_counter producer_consumer future_exception scoped_lock_deadlock_fix
	./mutex_atomic_counter
	./producer_consumer
	./future_exception
	./scoped_lock_deadlock_fix

clean:
	rm -f $(PROGRAMS) *_tsan *_asan

