#include <iostream>
#include <string_view>

enum class Event {
    start,
    fault,
    reset
};

enum class SimpleState {
    idle,
    active,
    error
};

SimpleState next_state(SimpleState state, Event event) {
    switch (state) {
    case SimpleState::idle:
        return event == Event::start ? SimpleState::active : SimpleState::idle;
    case SimpleState::active:
        return event == Event::fault ? SimpleState::error : SimpleState::active;
    case SimpleState::error:
        return event == Event::reset ? SimpleState::idle : SimpleState::error;
    }
    return SimpleState::error;
}

std::string_view name(SimpleState state) {
    switch (state) {
    case SimpleState::idle:
        return "idle";
    case SimpleState::active:
        return "active";
    case SimpleState::error:
        return "error";
    }
    return "unknown";
}

class Context;

class State {
public:
    virtual void on_event(Context& context, Event event) const = 0;
    virtual std::string_view name() const = 0;
    virtual ~State() = default;
};

class Context {
public:
    explicit Context(const State& state) : state_(&state) {}

    void transition_to(const State& state) {
        state_ = &state;
    }

    void on_event(Event event) {
        state_->on_event(*this, event);
    }

    std::string_view state_name() const {
        return state_->name();
    }

private:
    const State* state_;
};

class IdleState final : public State {
public:
    void on_event(Context& context, Event event) const override;
    std::string_view name() const override { return "idle"; }
};

class ActiveState final : public State {
public:
    void on_event(Context& context, Event event) const override;
    std::string_view name() const override { return "active"; }
};

class ErrorState final : public State {
public:
    void on_event(Context& context, Event event) const override;
    std::string_view name() const override { return "error"; }
};

const IdleState idle_state;
const ActiveState active_state;
const ErrorState error_state;

void IdleState::on_event(Context& context, Event event) const {
    if (event == Event::start) {
        context.transition_to(active_state);
    }
}

void ActiveState::on_event(Context& context, Event event) const {
    if (event == Event::fault) {
        context.transition_to(error_state);
    }
}

void ErrorState::on_event(Context& context, Event event) const {
    if (event == Event::reset) {
        context.transition_to(idle_state);
    }
}

int main() {
    SimpleState simple = SimpleState::idle;
    simple = next_state(simple, Event::start);
    simple = next_state(simple, Event::fault);
    std::cout << "simple FSM: " << name(simple) << '\n';

    Context context(idle_state);
    context.on_event(Event::start);
    context.on_event(Event::fault);
    std::cout << "State pattern: " << context.state_name() << '\n';

    // Learning note: the simple FSM is better for tiny stable machines.
    // The State pattern only earns its keep when state-specific behavior grows.
}
