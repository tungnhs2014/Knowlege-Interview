CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O0 -g
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all run sanitize strict clean

all: fsm_vs_state strategy_callbacks observer_subscription factory_adapter_facade command_ownership

fsm_vs_state: fsm_vs_state.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

strategy_callbacks: strategy_callbacks.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

observer_subscription: observer_subscription.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

factory_adapter_facade: factory_adapter_facade.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

command_ownership: command_ownership.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	./fsm_vs_state
	./strategy_callbacks
	./observer_subscription
	./factory_adapter_facade
	./command_ownership

sanitize:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) fsm_vs_state.cpp -o fsm_vs_state_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) strategy_callbacks.cpp -o strategy_callbacks_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) observer_subscription.cpp -o observer_subscription_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) factory_adapter_facade.cpp -o factory_adapter_facade_san
	$(CXX) $(CXXFLAGS) $(SANFLAGS) command_ownership.cpp -o command_ownership_san
	./fsm_vs_state_san
	./strategy_callbacks_san
	./observer_subscription_san
	./factory_adapter_facade_san
	./command_ownership_san

strict:
	$(CXX) $(CXXFLAGS) -Werror fsm_vs_state.cpp -o fsm_vs_state_strict
	$(CXX) $(CXXFLAGS) -Werror strategy_callbacks.cpp -o strategy_callbacks_strict
	$(CXX) $(CXXFLAGS) -Werror observer_subscription.cpp -o observer_subscription_strict
	$(CXX) $(CXXFLAGS) -Werror factory_adapter_facade.cpp -o factory_adapter_facade_strict
	$(CXX) $(CXXFLAGS) -Werror command_ownership.cpp -o command_ownership_strict

clean:
	rm -f fsm_vs_state strategy_callbacks observer_subscription factory_adapter_facade command_ownership
	rm -f fsm_vs_state_san strategy_callbacks_san observer_subscription_san factory_adapter_facade_san command_ownership_san
	rm -f fsm_vs_state_strict strategy_callbacks_strict observer_subscription_strict factory_adapter_facade_strict command_ownership_strict
