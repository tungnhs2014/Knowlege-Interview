# 10 - Resource Management In C++: Interview Pack

## How To Use This Pack

Answer each question in this order:

1. state the ownership and cleanup contract;
2. explain normal, failure, copy, move, and destruction paths;
3. anchor the answer in C or C++ code;
4. identify production and debugging consequences;
5. challenge convenient but incorrect slogans.

## Beginner Questions

### 1. What is RAII, and why is it not only about memory?

**Short answer**

RAII binds resource ownership to object lifetime. An object acquires or adopts
a resource, maintains an ownership invariant, and releases the resource in its
destructor. The resource can be memory, a file, a lock, a socket, a
registration, or any acquire/release pair.

**Deep explanation**

Manual cleanup is path-sensitive: every return, error, and exception must call
the correct release function. RAII moves that responsibility into one type.
Automatic objects and fully constructed members are destroyed at scope exit,
including early return and stack unwinding.

RAII does not require exceptions. Status-returning code receives the same
scope-based cleanup benefit. It also does not imply dynamic allocation:
`std::lock_guard` owns a lock without owning heap memory.

**C/C++ code or API anchor**

```cpp
void update()
{
    std::lock_guard<std::mutex> lock{mutex};
    change_state();
} // unlocks here on every exit
```

Standard RAII types include `std::vector`, `std::string`, file streams,
`std::unique_ptr`, `std::shared_ptr`, `std::lock_guard`, and
`std::unique_lock`.

**Production and debug angle**

During review, list every acquired resource and find its owner object. Test
normal return, every early return, and failure after each acquisition. Break on
constructors, destructors, and release functions to verify reverse-order
cleanup.

**Common traps**

- Saying RAII means "call `new` in a constructor."
- Assuming RAII matters only when exceptions are enabled.
- Treating a destructor as a suitable place to report recoverable close errors.
- Forgetting that slow destruction can violate a bounded-latency requirement.

**Follow-up questions**

- What happens when the second member constructor throws?
- How would you report a failed `close()` operation?
- Give a heap-free embedded use of RAII.

### 2. Explain ownership, borrowing, and aliasing.

**Short answer**

Ownership is responsibility for ending a resource lifetime. Borrowing grants
access without cleanup responsibility. Aliasing means multiple handles refer to
the same object; aliases do not automatically share ownership.

**Deep explanation**

A well-designed API makes lifetime effects visible. `T&` commonly expresses a
required borrow, `T*` a nullable borrow, `std::unique_ptr<T>` exclusive
ownership, and `std::shared_ptr<T>` shared ownership.

Copying a raw address creates another alias. It does not establish who must
delete the object. A borrow becomes dangling when the owner releases or moves
the resource.

**C/C++ code or API anchor**

```cpp
void inspect(const Image& image);              // borrow
void install(std::unique_ptr<Plugin> plugin); // transfer
void retain(std::shared_ptr<Task> task);       // share
```

```cpp
auto owner = std::make_unique<Image>();
Image* borrowed = owner.get();
owner.reset();
// borrowed now dangles
```

**Production and debug angle**

Annotate each parameter, return value, and stored member as owner or borrow.
When debugging use-after-free, inspect both the owner object's address and the
managed object's address, then identify the operation that ended ownership.

**Common traps**

- Calling every raw pointer an owner.
- Assuming `const` extends lifetime.
- Assuming null checks detect dangling non-null pointers.
- Returning a borrow without documenting the owner's required lifetime.

**Follow-up questions**

- When is `T*` better than `std::shared_ptr<T>`?
- What ownership does `unique_ptr::get()` return?
- How would you express optional ownership transfer?

### 3. Compare `malloc/free`, `new/delete`, and modern C++ ownership.

**Short answer**

`malloc` and related C APIs allocate raw storage and pair with `free`.
`new` obtains storage and initializes C++ objects; it pairs with `delete`.
Modern C++ usually puts resources directly into values, containers, or RAII
owners instead of exposing either pair in application code.

**Deep explanation**

Ordinary `new` reports allocation failure with `std::bad_alloc`; its nothrow
form returns null. `malloc` returns null on failure and does not run
constructors. `delete` ends object lifetime and releases matching storage;
`free` only releases storage from the C allocation family.

Mixing families or mismatching scalar and array forms is undefined behavior.
Runtime size does not require `new[]`; `std::vector` is usually the correct
owner.

**C/C++ code or API anchor**

```c
int *values = calloc(count, sizeof *values);
if (values == NULL && count != 0U) {
    return ERROR_NO_MEMORY;
}
free(values);
```

```cpp
std::vector<int> values(count);
```

**Production and debug angle**

ASan can expose mismatched release, double free, and use-after-free on executed
paths. At C boundaries, document which side owns a successful output and the
exact destroy function it must call.

**Common traps**

- Saying `delete` on `malloc` storage merely skips a destructor.
- Overwriting the only pointer with `realloc` before checking failure.
- Catching `bad_alloc` locally when the function cannot recover.
- Assuming `std::nothrow` makes allocation deterministic.

**Follow-up questions**

- Why can `realloc` move an allocation?
- What is the difference between storage and object lifetime?
- When is explicit allocation reasonable in C++?

### 4. Compare raw pointers, `unique_ptr`, `shared_ptr`, and `weak_ptr`.

**Short answer**

Raw pointers and references normally express borrowing. `unique_ptr` expresses
exclusive ownership, `shared_ptr` genuine co-ownership, and `weak_ptr`
non-owning observation of an object managed by shared ownership.

**Deep explanation**

`unique_ptr` is movable but not copyable, making ownership transfer explicit.
`shared_ptr` copies add owners through one control block. `weak_ptr` observes
that control block without extending object lifetime and obtains temporary
ownership with `lock()`.

Choose the weakest lifetime policy that meets the requirement. Shared ownership
is not a substitute for deciding who owns a dependency.

**C/C++ code or API anchor**

```cpp
auto unique = std::make_unique<Job>();
auto transferred = std::move(unique);

auto shared = std::make_shared<Job>();
std::weak_ptr<Job> observed = shared;
if (auto locked = observed.lock()) {
    locked->run();
}
```

**Production and debug angle**

Review `release()`, stored `get()` results, multiple control blocks, and cycles.
Use `use_count()` only as a diagnostic hint, not a synchronization or
uniqueness decision.

**Common traps**

- Using `shared_ptr` for every nullable pointer.
- Claiming `unique_ptr` is guaranteed to be exactly pointer-sized.
- Calling `expired()` and assuming the object stays alive afterward.
- Assuming a smart pointer prevents every dangling borrowed reference.

**Follow-up questions**

- When should a function accept `shared_ptr` by value?
- Why can a stateful deleter enlarge `unique_ptr`?
- What is the cost of a shared ownership cycle?

## Mid-Level Questions

### 5. Explain the Rule of Three, Rule of Five, and Rule of Zero.

**Short answer**

The Rule of Three says a class with a custom destructor or copy operation must
review all three. The Rule of Five adds move construction and move assignment.
The Rule of Zero is preferred: compose standard owner members so no custom
special member is needed.

**Deep explanation**

A raw unique owner copied memberwise duplicates an address and causes double
release. A copyable value-like owner needs deep-copy semantics. A unique owner
should delete copying. A movable owner transfers the resource and leaves the
source valid.

The rules are review heuristics, not commands to hand-write functions. Default
or delete operations when that best expresses the contract.

**C/C++ code or API anchor**

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(size)
    {
    }

private:
    std::vector<int> data_; // Rule of Zero
};
```

```cpp
class Session {
public:
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) noexcept;
    Session& operator=(Session&&) noexcept;
};
```

**Production and debug angle**

Use type traits and compiler diagnostics to inspect copy/move support. Test
copy construction, copy assignment, move construction, move assignment,
self-assignment, empty-state destruction, and exactly-once release.

**Common traps**

- Implementing all five without deciding value or ownership semantics.
- Calling compiler-generated copying universally "shallow."
- Forgetting that defining one special member can affect generation of others.
- Writing manual ownership when `vector` or `unique_ptr` already fits.

**Follow-up questions**

- When should copying be deleted?
- How does pImpl affect special members?
- What guarantee should copy assignment provide?

### 6. Debug this resource-owning copy bug.

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t n)
        : data_{new int[n]},
          size_{n}
    {
    }

    ~Buffer()
    {
        delete[] data_;
    }

private:
    int* data_;
    std::size_t size_;
};
```

**Short answer**

The compiler-generated copy operations copy `data_` as an address. Two
`Buffer` objects then release the same allocation, causing undefined behavior.
Delete copying, implement deep copy, or preferably replace the raw owner with
`std::vector<int>`.

**Deep explanation**

The class advertises value-like copyability but stores unique ownership in a
raw pointer. A correct deep copy allocates independent storage. Copy assignment
must not delete the target's old storage before replacement allocation
succeeds.

Copy-and-swap or allocate-before-commit can provide the strong guarantee.
Rule of Zero removes the entire custom lifetime implementation.

**C/C++ code or API anchor**

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t n)
        : data_(n)
    {
    }

private:
    std::vector<int> data_;
};
```

ASan build:

```bash
c++ -std=c++17 -fsanitize=address,undefined -g buffer.cpp
```

**Production and debug angle**

The failure may appear at destruction rather than at the bad copy. Break on the
copy constructor, assignment, and destructor; print both resource addresses.
ASan commonly reports double free or heap-use-after-free.

**Common traps**

- Fixing only the copy constructor while leaving copy assignment unsafe.
- Setting one pointer to null and ignoring aliases.
- Adding a self-assignment check before fixing exception safety.
- Replacing raw ownership with `shared_ptr` without requiring shared semantics.

**Follow-up questions**

- Implement deep copy with the strong guarantee.
- How should a move constructor empty the source?
- Why is `vector` the simpler production answer?

### 7. What does `std::move` do, and what is valid after a move?

**Short answer**

`std::move` is a cast that permits move overload selection; it does not transfer
anything itself. The selected operation defines the move. A moved-from object
must remain valid, but its value is often unspecified.

**Deep explanation**

A named rvalue-reference variable is still an lvalue expression, so moving from
it again requires `std::move`. Typical move constructors transfer a handle and
place the source in an empty state, but moves can be type-defined and need not
be constant time.

After a move, destruction and assignment are valid. Other operations are valid
when their preconditions hold. Do not assume all types become empty.

**C/C++ code or API anchor**

```cpp
Session(Session&& other) noexcept
    : handle_{std::exchange(other.handle_, invalid)}
{
}
```

```cpp
auto destination = std::move(source);
source = Session{new_handle}; // assignment to valid moved-from object
```

**Production and debug angle**

Inspect the source after move and verify it cannot release the transferred
resource. Use `static_assert(std::is_nothrow_move_constructible_v<T>)` only
when the API requires that property.

**Common traps**

- Saying `std::move` performs the move.
- Using an object after move without checking operation preconditions.
- Moving from `const` and expecting a typical non-const move constructor.
- Adding `std::move` to a local return and inhibiting NRVO.

**Follow-up questions**

- Why is a named `T&&` expression an lvalue?
- When should move be marked `noexcept`?
- Can a move allocate or throw?

### 8. Design a safe RAII wrapper for a C resource.

**Short answer**

Encode the valid/empty state, exact release function, unique ownership, move
transfer, and non-throwing destruction in one type. Delete copying unless the C
API provides a meaningful duplication operation.

**Deep explanation**

The wrapper should acquire through a factory or constructor, reject or represent
failure explicitly, release exactly once, and leave moved-from objects empty.
If close failure matters, provide an explicit `close()` while retaining a
best-effort no-throw destructor.

A `unique_ptr` custom deleter works for pointer-shaped resources. A dedicated
class is clearer for integer or tagged handles.

**C/C++ code or API anchor**

```cpp
struct FileCloser {
    void operator()(std::FILE* file) const noexcept
    {
        if (file != nullptr) {
            std::fclose(file);
        }
    }
};

using FilePtr = std::unique_ptr<std::FILE, FileCloser>;
```

**Production and debug angle**

Use a fake C API that counts acquisitions and releases. Test acquisition
failure, early return, exception, move, reset, and destruction. Verify the
deleter contains all context required by the release API.

**Common traps**

- Pairing the handle with the wrong destroy function.
- Allowing accidental copying.
- Throwing from the deleter or destructor.
- Returning a raw handle and losing the ownership contract.

**Follow-up questions**

- How would you represent a fallible close?
- When does a stateful deleter affect object size?
- How would you wrap caller-provided storage instead?

## Senior Questions

### 9. Review an API that accepts smart pointers in every parameter.

**Short answer**

Smart-pointer parameters should express lifetime effects, not merely access.
Use `unique_ptr` by value for transfer, `shared_ptr` by value for retained
co-ownership, and `T&` or `T*` when the function only borrows the object.

**Deep explanation**

Accepting `const shared_ptr<T>&` for observation couples callers to one
ownership mechanism without sharing ownership. Accepting `unique_ptr<T>&`
usually exposes the owner's container rather than the object. A narrow
borrowed interface reduces coupling and makes retention impossible.

An API review must also cover return ownership, callback captures, cancellation,
shutdown order, and whether a stored borrow can outlive its owner.

**C/C++ code or API anchor**

```cpp
void inspect(const Job& job);                   // borrow
void inspect_optional(const Job* job);          // nullable borrow
void install(std::unique_ptr<Job> job);         // transfer
void schedule(std::shared_ptr<Job> job);        // retain/share
std::unique_ptr<Job> create_job();              // return ownership
```

**Production and debug angle**

Trace ownership at API boundaries and asynchronous queues. A leak may indicate
unintended retention; a crash may indicate a queued borrow. Heap profiles and
shutdown tests are often more useful than inspecting `use_count()` alone.

**Common traps**

- Replacing all raw pointers with `shared_ptr`.
- Passing `shared_ptr` by value when the function does not retain it.
- Capturing `this` in delayed work without a lifetime policy.
- Hiding ownership transfer behind a raw pointer return.

**Follow-up questions**

- How would a callback safely observe an optional owner?
- When is converting `unique_ptr` to `shared_ptr` appropriate?
- How should cancellation interact with resource lifetime?

### 10. Explain shared ownership cycles and smart-pointer thread safety.

**Short answer**

A cycle of strong `shared_ptr` edges prevents reference counts from reaching
zero. Replace non-owning or back-reference edges with `weak_ptr`. Reference
count operations do not make the managed object thread-safe.

**Deep explanation**

All strong edges claim ownership. If an unreachable graph owns itself, ordinary
scope exit cannot reclaim it. The design should identify roots and distinguish
ownership edges from observation edges.

Separate smart-pointer objects sharing one control block support concurrent
ownership bookkeeping according to library guarantees. Concurrent mutation of
the object still requires locks, atomics, immutability, or confinement.

**C/C++ code or API anchor**

```cpp
struct Parent;

struct Child {
    std::weak_ptr<Parent> parent;
};

struct Parent {
    std::shared_ptr<Child> child;
};
```

```cpp
if (auto parent = child.parent.lock()) {
    parent->notify();
}
```

**Production and debug angle**

Use heap profiling and destruction counters to find retained graphs. Run
ThreadSanitizer for races in managed state. Do not infer race freedom from a
stable reference count.

**Common traps**

- Using `expired()` followed by a separate access.
- Treating `use_count() == 1` as synchronized uniqueness.
- Making every graph edge strong.
- Constructing `shared_ptr(this)` and creating another control block.

**Follow-up questions**

- When is `enable_shared_from_this` appropriate?
- How would you design an observer callback list?
- What remains allocated while `weak_ptr` objects survive?

### 11. State the exception guarantees of a resource-owning operation.

**Short answer**

The basic guarantee preserves invariants and leaks no resources. The strong
guarantee additionally leaves observable state unchanged on failure. The
no-throw guarantee emits no exception. RAII supplies cleanup safety, but it
does not automatically supply the strong state guarantee.

**Deep explanation**

Strong-guarantee code performs potentially throwing work before committing:
construct replacement state, validate it, then commit with a no-throw swap or
equivalent operation. Some operations cannot economically provide the strong
guarantee and should document the basic guarantee.

`noexcept` must reflect reality. If an exception exits a `noexcept` function,
the program calls `std::terminate`.

**C/C++ code or API anchor**

```cpp
void Configuration::replace(std::vector<Entry> replacement)
{
    validate(replacement);       // may throw before commit
    entries_.swap(replacement);  // no-throw commit
}
```

```cpp
void swap(Buffer& other) noexcept;
```

**Production and debug angle**

Inject failures after each acquisition and mutation. Verify release counts,
invariants, and observable state. Review destructors and custom deleters for
hidden throwing operations.

**Common traps**

- Calling any code with RAII "strongly exception-safe."
- Deleting old state before replacement allocation succeeds.
- Marking move `noexcept` only for performance.
- Requiring the strong guarantee for every operation regardless of cost.

**Follow-up questions**

- How does partial construction clean up members?
- Compare copy-and-swap with allocate-before-commit.
- Why can a throwing move affect container relocation?

### 12. Choose an allocation strategy for a constrained or high-throughput system.

**Short answer**

Start with lifetime, capacity, latency, fragmentation, locality, and failure
requirements. Prefer scoped/static storage and fixed-capacity owners when
bounded behavior is required. Use pools, arenas, or `std::pmr` only when
measurement shows the general allocator is unsuitable.

**Deep explanation**

An arena can make allocation cheap and release a phase's storage together, but
it may retain memory until reset and requires the resource to outlive all
objects using it. A pool can bound object size and allocation cost but adds
capacity and exhaustion policy. General dynamic allocation offers flexibility
but may not meet deterministic constraints.

RAII still governs object destruction and scope even when storage comes from a
fixed buffer. Placement construction additionally requires size, alignment,
object-lifetime, and destruction correctness.

**C/C++ code or API anchor**

```cpp
std::byte storage[4096];
std::pmr::monotonic_buffer_resource arena{
    storage,
    sizeof storage
};

std::pmr::vector<Record> records{&arena};
```

**Production and debug angle**

Measure peak memory, allocation count, latency distribution, fragmentation,
cache behavior, and exhaustion handling on the target. Test that the memory
resource outlives every allocator-aware container.

**Common traps**

- Introducing a custom allocator before measuring.
- Assuming arena allocation removes destructor requirements.
- Letting containers outlive their memory resource.
- Calling any nothrow allocation real-time safe.

**Follow-up questions**

- How should arena exhaustion be reported?
- When is startup-only allocation sufficient?
- What trade-off does pImpl add to allocation and locality?

## Coding Tasks

### Task 1. Rule Of Zero Refactor

Refactor a raw owning buffer into a value type using `std::vector`. Preserve
copy and move behavior, and explain why custom special members disappear.

### Task 2. Movable C Handle

Implement a non-copyable, movable wrapper around a fake integer handle. Its
release function must run exactly once. Add tests for move construction, move
assignment, empty destruction, and acquisition failure.

### Task 3. Ownership-Oriented API

Design signatures for:

- observing a required object;
- observing an optional object;
- transferring a plugin;
- retaining a background task;
- returning a newly created polymorphic object.

Explain every lifetime effect.

### Task 4. Repair A Shared Cycle

Create parent/child objects with an intentional strong cycle, demonstrate that
destructors do not run, then replace the back-reference with `weak_ptr` and use
`lock()` safely.

## Debugging Scenarios

### Scenario 1. Crash At Program Shutdown

Two copied resource owners release the same address. Inspect generated copy
operations, run ASan, and replace raw ownership or implement the correct copy
policy.

### Scenario 2. Object Never Destructs

Inspect the `shared_ptr` graph for a cycle or unintended retained owner. Do not
use `use_count()` as the fix; identify which edges should be non-owning.

### Scenario 3. Rare Use-After-Free In A Callback

The callback captured `this` or a borrowed pointer. Define whether the task
must extend ownership, observe with `weak_ptr`, copy values, or be joined before
owner destruction.

### Scenario 4. Termination During Cleanup

A destructor, deleter, or falsely declared `noexcept` move emits an exception.
Move fallible reporting to an explicit operation and keep cleanup non-throwing.

## Rapid-Fire Review

- Does `std::move` move by itself? No.
- Is a moved-from object invalid? No; it is valid with type-defined or
  unspecified state.
- Does `shared_ptr` synchronize the object? No.
- Is `weak_ptr::expired()` a lifetime lock? No.
- Is every raw pointer an owner? No.
- May `unique_ptr` have a stateful deleter? Yes.
- Can Rule of Zero support deep value copying? Yes, through value members.
- Does RAII require exceptions? No.
- Is allocation-family mismatch defined? No, it is undefined behavior.
- Should every operation provide the strong guarantee? No; document the
  strongest practical guarantee.

## Final Interview Checklist

- Name the owner and every borrow.
- State the release function and empty state.
- Walk normal, early-return, exception, copy, move, and destruction paths.
- Distinguish storage from object lifetime.
- Prefer Rule of Zero and values.
- Justify shared ownership rather than defaulting to it.
- Separate reference-count safety from object thread safety.
- State exception and latency guarantees.
- Use sanitizers and failure injection, but do not treat a clean run as proof.

