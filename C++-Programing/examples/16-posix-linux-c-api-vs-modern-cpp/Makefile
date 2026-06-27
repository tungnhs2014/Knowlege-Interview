CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O0 -g -pthread
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
TSANFLAGS := -fsanitize=thread -fno-omit-frame-pointer

.PHONY: all run sanitize tsan run-tsan strict clean

all: fd_raii pthread_vs_thread condition_variable_queue signal_shutdown

fd_raii: fd_raii.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

pthread_vs_thread: pthread_vs_thread.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

condition_variable_queue: condition_variable_queue.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

signal_shutdown: signal_shutdown.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	./fd_raii
	./pthread_vs_thread
	./condition_variable_queue
	./signal_shutdown

sanitize:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) fd_raii.cpp -o fd_raii_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) pthread_vs_thread.cpp -o pthread_vs_thread_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) condition_variable_queue.cpp -o condition_variable_queue_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) signal_shutdown.cpp -o signal_shutdown_san
	./fd_raii_san
	./pthread_vs_thread_san
	./condition_variable_queue_san
	./signal_shutdown_san

tsan:
	$(CXX) $(CXXFLAGS) $(TSANFLAGS) pthread_vs_thread.cpp -o pthread_vs_thread_tsan
	$(CXX) $(CXXFLAGS) $(TSANFLAGS) condition_variable_queue.cpp -o condition_variable_queue_tsan

run-tsan: tsan
	./pthread_vs_thread_tsan
	./condition_variable_queue_tsan

strict:
	$(CXX) $(CXXFLAGS) -Werror fd_raii.cpp -o fd_raii_strict
	$(CXX) $(CXXFLAGS) -Werror pthread_vs_thread.cpp -o pthread_vs_thread_strict
	$(CXX) $(CXXFLAGS) -Werror condition_variable_queue.cpp -o condition_variable_queue_strict
	$(CXX) $(CXXFLAGS) -Werror signal_shutdown.cpp -o signal_shutdown_strict

clean:
	rm -f fd_raii pthread_vs_thread condition_variable_queue signal_shutdown
	rm -f fd_raii_san pthread_vs_thread_san condition_variable_queue_san signal_shutdown_san
	rm -f pthread_vs_thread_tsan condition_variable_queue_tsan
	rm -f fd_raii_strict pthread_vs_thread_strict condition_variable_queue_strict signal_shutdown_strict
	rm -f fd_raii_demo.txt
