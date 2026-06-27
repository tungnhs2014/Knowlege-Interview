# 10 - Resource Management In C++

## 1. Goal

After this lesson, you should be able to:

- distinguish storage, object lifetime, ownership, borrowing, and aliasing;
- explain RAII as a general resource-management technique;
- identify the owner and cleanup path for every resource;
- review raw `new`, `delete`, `malloc`, and `free` safely;
- design copy and move behavior for resource-owning classes;
- apply the Rule of Three, Rule of Five, and Rule of Zero;
- choose among `std::unique_ptr`, `std::shared_ptr`, and `std::weak_ptr`;
- adapt C resources with custom deleters and RAII wrappers;
- reason about cleanup during early return, exceptions, and partial construction;
- debug leaks, double release, use-after-free, and shared-ownership cycles.

This lesson uses C++17 for its main examples and C17 for allocation comparison.
Chapter 09, OOP In C++, is the prerequisite.

## 2. Why It Matters

A resource is anything that must eventually be released:

- dynamically allocated memory;
- a file or library handle;
- a socket or descriptor;
- a mutex lock;
- a registration or subscription;
- a transaction;
- a temporary device or service mode.

Resource-management defects often survive normal testing because the happy path
works. The failure appears later:

- an early return skips cleanup;
- an exception bypasses `delete`;
- a copied raw pointer causes two destructors to release one allocation;
- a callback keeps a reference after its owner dies;
- two `shared_ptr` objects keep each other alive forever;
- a destructor emits an exception during stack unwinding;
- an unbounded cleanup operation violates a real-time deadline.

The central review question is:

> Who owns this resource, and how is it released exactly once on every path?

Modern C++ answers that question by representing ownership with objects whose
destructors perform cleanup.

## 3. Mental Model

### 3.1 Storage Is Not Lifetime

Storage is memory in which an object may exist. Object lifetime is the period
during which that object has been initialized and may be used according to its
type.

Do not reduce the model to "stack versus heap":

- the C++ language specifies storage durations and object lifetimes;
- common platforms often implement automatic objects using a call stack;
- `new` obtains dynamic storage from the free store;
- exact memory segments, address ordering, and growth direction are
  implementation details.

An object can own a resource that is not memory:

```cpp
class LockGuard {
public:
    explicit LockGuard(Mutex& mutex)
        : mutex_{mutex}
    {
        mutex_.lock();
    }

    ~LockGuard()
    {
        mutex_.unlock();
    }

private:
    Mutex& mutex_;
};
```

The `LockGuard` object's lifetime controls the lock's ownership interval.

### 3.2 Ownership

Ownership is responsibility for ending a resource lifetime.

An owner must answer:

- What resource is represented?
- What state means "no resource"?
- Which operation acquires or adopts it?
- Which operation releases it?
- Can ownership be copied?
- Can ownership be moved?
- What happens when acquisition fails?
- Can cleanup fail?

There should normally be one clear owner for an exclusively owned resource.

### 3.3 Borrowing

A borrow permits access without cleanup responsibility.

Typical C++ spellings:

| Type | Typical meaning |
| --- | --- |
| `T&` | Required borrowed object |
| `const T&` | Required read-only borrow |
| `T*` | Nullable borrowed object |
| `const T*` | Nullable read-only borrow |
| `std::unique_ptr<T>` | Exclusive owner |
| `std::shared_ptr<T>` | Shared owner |
| `std::weak_ptr<T>` | Non-owning observer of shared lifetime |

The types do not solve every lifetime problem. A borrowed pointer or reference
still dangles if it outlives its owner.

### 3.4 Aliasing

Aliasing means that multiple handles refer to the same object:

```cpp
int value = 42;
int* first = &value;
int* second = first;
```

Both pointers are aliases. Neither owns `value`.

Copying an address does not automatically copy or share ownership. This is why
copying a raw owning pointer is dangerous: it duplicates the address without
defining who releases the resource.

### 3.5 The Cleanup Matrix

For every owner, reason through:

| Path | Required behavior |
| --- | --- |
| Normal return | Release exactly once |
| Early return | Release exactly once |
| Exception | Release fully acquired resources |
| Partial construction | Destroy fully constructed members |
| Move | Transfer ownership; source remains valid |
| Copy | Deep copy, shared ownership, or prohibited copy |
| Program shutdown | Respect dependency and destruction order |

This matrix is more useful than counting `new` and `delete` statements.

## 4. RAII

RAII means **Resource Acquisition Is Initialization**. Its practical rule is:

> Bind resource ownership to an object's lifetime.

The owner establishes its invariant during construction or adoption and
releases the resource in its destructor.

### 4.1 Manual Cleanup Is Path-Sensitive

```cpp
void process()
{
    int* data = new int[1024];

    if (!prepare(data)) {
        delete[] data;
        return;
    }

    transform(data); // If this throws, data leaks.
    delete[] data;
}
```

Every new return or throwing operation creates another cleanup path to review.

### 4.2 RAII Makes Cleanup Scope-Based

```cpp
#include <memory>

void process()
{
    auto data = std::make_unique<int[]>(1024);

    if (!prepare(data.get())) {
        return;
    }

    transform(data.get());
}
```

`data` is destroyed on normal return, early return, and exception propagation.
The cleanup rule is centralized in `std::unique_ptr`.

For a resizable buffer, a value type is usually even clearer:

```cpp
#include <vector>

void process()
{
    std::vector<int> data(1024);
    transform(data.data());
}
```

### 4.3 Destruction Order

Local objects are destroyed in reverse construction order:

```cpp
void work()
{
    Session session;
    Buffer buffer;
    Transaction transaction;
}
```

The order is:

1. `transaction`;
2. `buffer`;
3. `session`.

Class members are also destroyed in reverse declaration order. Declare members
so dependencies remain alive while dependent members are destroyed.

### 4.4 Partial Construction

If a constructor throws:

- the enclosing object's destructor is not called because its construction did
  not complete;
- fully constructed base classes and members are destroyed;
- members whose construction did not complete are not destroyed.

This makes member-owned RAII composition powerful:

```cpp
class Service {
public:
    Service()
        : connection_{open_connection()},
          cache_{load_cache()}
    {
    }

private:
    Connection connection_;
    Cache cache_;
};
```

If `load_cache()` throws, `connection_` is already fully constructed and is
destroyed automatically.

### 4.5 Cleanup Must Not Escape From Destructors

A destructor should not allow exceptions to escape:

```cpp
class OutputFile {
public:
    void close()
    {
        // Explicit operation may report flush/close failure.
    }

    ~OutputFile() noexcept
    {
        // Best-effort cleanup; do not emit an exception.
    }
};
```

When cleanup failure matters, provide an explicit operation such as `close()`,
`commit()`, or `finish()`. Keep the destructor as a no-throw fallback.

## 5. Copy, Move, And Special Members

### 5.1 Compiler-Generated Copying Is Memberwise

Consider a raw owner:

```cpp
class BadBuffer {
public:
    explicit BadBuffer(std::size_t size)
        : data_{new int[size]},
          size_{size}
    {
    }

    ~BadBuffer()
    {
        delete[] data_;
    }

private:
    int* data_;
    std::size_t size_;
};
```

The compiler-generated copy constructor copies `data_` as an address:

```cpp
BadBuffer first{10};
BadBuffer second = first; // Both objects now appear to own one allocation.
```

Both destructors call `delete[]` for the same address. The behavior is
undefined.

### 5.2 Shallow And Deep Copy

A shallow handle copy preserves aliasing. A deep copy creates an independent
resource.

Deep copy is appropriate when the class promises value semantics:

```cpp
#include <algorithm>
#include <cstddef>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_{new int[size]{}},
          size_{size}
    {
    }

    ~Buffer()
    {
        delete[] data_;
    }

    Buffer(const Buffer& other)
        : data_{new int[other.size_]},
          size_{other.size_}
    {
        std::copy(
            other.data_,
            other.data_ + other.size_,
            data_);
    }

    Buffer& operator=(const Buffer& other)
    {
        if (this != &other) {
            Buffer replacement{other};
            swap(replacement);
        }
        return *this;
    }

    void swap(Buffer& other) noexcept
    {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
    }

private:
    int* data_;
    std::size_t size_;
};
```

The copy constructor allocates first. If allocation fails, the source remains
unchanged and no incomplete `Buffer` object is produced.

### 5.3 Rule Of Three

If a class needs a custom:

- destructor;
- copy constructor; or
- copy assignment operator;

review all three. A raw copyable owner normally needs all three.

Copy assignment also has to release the target's previous resource without
losing exception safety.

### 5.4 Copy-And-Swap

The complete `Buffer` above uses a temporary copy and swap:

```cpp
Buffer& operator=(const Buffer& other)
{
    if (this != &other) {
        Buffer replacement{other};
        swap(replacement);
    }
    return *this;
}
```

The replacement is created before commit. If copying fails, `*this` remains
unchanged. The no-throw swap commits the new state, and `replacement` destroys
the previous state.

Copy-and-swap is simple and can provide the strong guarantee, but it is not
always the most efficient assignment strategy.

Do not combine a by-value `operator=(Buffer)` with a separate
`operator=(Buffer&&)`: assignment from an rvalue can become ambiguous. Use the
distinct `const Buffer&` and `Buffer&&` overloads shown in this lesson, or use
one by-value assignment without a separate move-assignment overload.

### 5.5 Move Construction

A move constructor transfers the resource relationship:

```cpp
Buffer(Buffer&& other) noexcept
    : data_{std::exchange(other.data_, nullptr)},
      size_{std::exchange(other.size_, 0)}
{
}
```

After the move:

- the destination owns the original allocation;
- the source owns nothing;
- both objects remain valid and destructible.

`std::move` does not move anything by itself. It casts an expression so that a
move overload may be selected:

```cpp
Buffer destination = std::move(source);
```

The selected move constructor performs the transfer.

### 5.6 Move Assignment

```cpp
Buffer& operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        delete[] data_;
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}
```

This manual implementation is educational. In normal application code, prefer
standard owner members that already implement correct move behavior.

### 5.7 Moved-From Objects

A moved-from object must remain valid. Its exact value is often unspecified
unless the type documents a stronger postcondition.

Safe general operations include:

- destruction;
- assignment of a new value;
- operations whose preconditions are known to hold.

Do not assume every moved-from object is empty. Do not assume it is unusable.

### 5.8 `noexcept` And Move

Mark a move operation `noexcept` when its implementation cannot throw:

```cpp
Buffer(Buffer&& other) noexcept;
```

This matters because containers may prefer copying during relocation when copy
is available and moving might throw. A false `noexcept` is dangerous: if an
exception exits that function, `std::terminate` is called.

### 5.9 Rule Of Five

A low-level movable resource owner may need:

1. destructor;
2. copy constructor;
3. copy assignment;
4. move constructor;
5. move assignment.

The Rule of Five is a review heuristic. It does not mean blindly hand-writing
all five. A class may deliberately delete copy operations:

```cpp
class UniqueHandle {
public:
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&&) noexcept = default;
    UniqueHandle& operator=(UniqueHandle&&) noexcept = default;
};
```

### 5.10 Rule Of Zero

The preferred design delegates resource management to standard owner types:

```cpp
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(size)
    {
    }

private:
    std::vector<int> data_;
};
```

No custom destructor, copy operation, or move operation is needed.
`std::vector` already implements correct value and resource semantics.

Rule of Zero reduces code, review surface, and exception-safety risk.

## 6. C Allocation APIs

C uses explicit allocation and cleanup:

```c
#include <stddef.h>
#include <stdlib.h>

int make_values(size_t count, int **out_values)
{
    if (out_values == NULL) {
        return 0;
    }

    int *values = calloc(count, sizeof *values);
    if (values == NULL && count != 0U) {
        return 0;
    }

    *out_values = values;
    return 1;
}

int main(void)
{
    int *values = NULL;
    if (!make_values(16U, &values)) {
        return 1;
    }

    free(values);
    return 0;
}
```

The contract must define that the caller owns the successful output and must
call `free`.

### 6.1 Safe `realloc` Pattern

`realloc` can fail without freeing the original allocation:

```c
if (new_count > SIZE_MAX / sizeof *values) {
    return 0;
}

int *replacement = realloc(values, new_count * sizeof *values);
if (replacement == NULL && new_count != 0U) {
    /* values still refers to the original allocation. */
    return 0;
}
values = replacement;
```

Do not overwrite the only owner before checking the result.

### 6.2 Multi-Step Cleanup In C

C often centralizes cleanup with one exit section:

```c
int perform_work(void)
{
    int result = 0;
    ResourceA *a = acquire_a();
    if (a == NULL) {
        goto cleanup;
    }

    ResourceB *b = acquire_b(a);
    if (b == NULL) {
        goto cleanup_a;
    }

    result = use_resources(a, b);
    release_b(b);

cleanup_a:
    release_a(a);
cleanup:
    return result;
}
```

This can be correct, but every acquisition and label must remain synchronized
during maintenance. C++ RAII moves that relationship into types.

## 7. C++ Allocation APIs

### 7.1 `new` And `delete`

```cpp
Widget* widget = new Widget{42};
delete widget;
```

`new` obtains storage and initializes an object. `delete` ends the object's
lifetime and releases the matching storage.

For arrays:

```cpp
Widget* widgets = new Widget[10];
delete[] widgets;
```

Mismatching `new` with `delete[]`, or `new[]` with `delete`, is undefined
behavior.

Primary application code should rarely contain naked `new` or `delete`.

### 7.2 Allocation Failure

Ordinary throwing `new` reports failure with `std::bad_alloc`. The nothrow form
returns null:

```cpp
#include <new>

Widget* widget = new (std::nothrow) Widget{42};
if (widget == nullptr) {
    return;
}
delete widget;
```

Choose failure handling according to the project policy. `std::nothrow` does
not make allocation deterministic or real-time safe.

### 7.3 Do Not Mix Families

These are all incorrect:

```cpp
void* raw = std::malloc(sizeof(Widget));
// delete static_cast<Widget*>(raw); // Undefined behavior.

Widget* object = new Widget{};
// std::free(object);                // Undefined behavior.
```

Match:

- `malloc`/`calloc`/`realloc` with `free`;
- `new` with `delete`;
- `new[]` with `delete[]`;
- library acquisition with its documented release function.

## 8. Smart Pointers

Smart pointers are ownership types. They are not replacements for every raw
pointer.

### 8.1 `std::unique_ptr`

`std::unique_ptr` represents exclusive ownership:

```cpp
#include <memory>

auto sensor = std::make_unique<Sensor>(42);
```

It cannot be copied:

```cpp
// auto second = sensor; // Error: copying ownership is disabled.
```

It can transfer ownership:

```cpp
auto second = std::move(sensor);
```

Afterward, `second` owns the object and `sensor` is empty.

Use `unique_ptr` by value to express ownership transfer:

```cpp
#include <stdexcept>

class Device {
public:
    explicit Device(std::unique_ptr<Sensor> sensor)
        : sensor_{std::move(sensor)}
    {
        if (!sensor_) {
            throw std::invalid_argument{"Device requires a sensor"};
        }
    }

private:
    std::unique_ptr<Sensor> sensor_;
};
```

The constructor establishes a non-null invariant. If an empty owner is a valid
domain state, model that state explicitly and make every operation handle it;
do not keep an implicit nullable member and dereference it unconditionally.

Borrow the pointed-to object when ownership is not changing:

```cpp
void inspect(const Sensor& sensor);

inspect(*second);
```

### 8.2 Polymorphic Ownership

```cpp
class Transport {
public:
    virtual ~Transport() = default;
    virtual void send() = 0;
};

class SerialTransport final : public Transport {
public:
    void send() override
    {
    }
};

std::unique_ptr<Transport> make_transport()
{
    return std::make_unique<SerialTransport>();
}
```

The base needs an appropriate virtual destructor because destruction occurs
through `unique_ptr<Transport>`.

### 8.3 `get`, `reset`, And `release`

- `get()` returns a borrowed raw pointer.
- `reset()` releases the current object and optionally adopts another.
- `release()` gives up ownership and returns the raw pointer without deleting
  it.

```cpp
Sensor* borrow = sensor.get();
sensor.reset();
// borrow now dangles.
```

`release()` is an ownership transfer escape hatch. The caller must immediately
place the returned pointer under another correct owner or release API.

### 8.4 `std::shared_ptr`

`std::shared_ptr` represents shared ownership:

```cpp
auto configuration = std::make_shared<Configuration>();

auto service_a = configuration;
auto service_b = configuration;
```

The managed object is destroyed after the last shared owner releases it.

Use shared ownership only when independent components truly co-own lifetime.
It is not the default answer for:

- nullable access;
- dependency injection;
- avoiding ownership decisions;
- making an object thread-safe.

Passing `shared_ptr` by value is meaningful when a function retains ownership:

```cpp
class Worker {
public:
    void retain(std::shared_ptr<Job> job)
    {
        job_ = std::move(job);
    }

private:
    std::shared_ptr<Job> job_;
};
```

When a function only observes the object, use `Job&`, `const Job&`, or a
nullable `Job*`.

### 8.5 Control Block And Thread Safety

A shared ownership group has a control block containing ownership bookkeeping.

Separate `shared_ptr` objects that share a control block can be manipulated by
different threads according to the library's guarantees. This does not make the
managed object's fields safe for concurrent access.

```cpp
auto state = std::make_shared<State>();

// Concurrent state->update() calls still need State's synchronization policy.
```

Do not use `use_count()` as a synchronization or uniqueness decision.

### 8.6 Producing Shared Ownership From `this`

Do not construct an independent owner from `this`:

```cpp
// std::shared_ptr<Node> self{this}; // Wrong: creates another control block.
```

When an object already managed by `shared_ptr` must produce another owner for
itself, use `std::enable_shared_from_this`:

```cpp
#include <memory>

class Node : public std::enable_shared_from_this<Node> {
public:
    std::shared_ptr<Node> self()
    {
        return shared_from_this();
    }
};

auto node = std::make_shared<Node>();
auto same_owner = node->self();
```

`shared_from_this()` requires the object to be associated with a compatible
shared-ownership control block. Calling it on an object that is not properly
shared-owned is an error.

### 8.7 Shared-Ownership Cycle

```cpp
struct Child;

struct Parent {
    std::shared_ptr<Child> child;
};

struct Child {
    std::shared_ptr<Parent> parent;
};
```

If both objects own each other, neither reference count reaches zero.

One direction should usually be non-owning:

```cpp
struct Child {
    std::weak_ptr<Parent> parent;
};
```

### 8.8 `std::weak_ptr`

`std::weak_ptr` observes an object managed by shared ownership without extending
its lifetime:

```cpp
std::weak_ptr<Job> observed = job;

if (auto locked = observed.lock()) {
    locked->run();
}
```

`lock()` atomically attempts to create temporary shared ownership. Checking
`expired()` and then using the object in a separate operation does not reserve
its lifetime.

### 8.9 `make_unique` And `make_shared`

Prefer factory helpers when their ownership and allocation behavior fits:

```cpp
auto unique = std::make_unique<Widget>(42);
auto shared = std::make_shared<Widget>(42);
```

Benefits include:

- immediate placement under an owner;
- concise code;
- reduced repetition;
- commonly one combined allocation for `make_shared`.

Trade-offs:

- custom deleter designs may require direct smart-pointer construction;
- `make_shared` combines object and control-block storage in common
  implementations, so outstanding `weak_ptr` objects may keep that storage
  allocated after the object is destroyed;
- special allocation strategies may need another factory.

## 9. Non-Memory RAII And Advanced Ownership

Many C APIs return handles that require a specific release function.

### 9.1 `FILE*` With `unique_ptr`

```cpp
#include <cstdio>
#include <memory>
#include <stdexcept>

struct FileCloser {
    void operator()(std::FILE* file) const noexcept
    {
        if (file != nullptr) {
            std::fclose(file);
        }
    }
};

using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

FilePtr open_file(const char* path)
{
    FilePtr file{std::fopen(path, "rb")};
    if (!file) {
        throw std::runtime_error{"fopen failed"};
    }
    return file;
}
```

The acquisition family and release family are encoded together.

### 9.2 Stateful Deleter

A release function may need context:

```cpp
struct HandleCloser {
    Library* library;

    void operator()(Handle* handle) const noexcept
    {
        if (handle != nullptr) {
            library->destroy(handle);
        }
    }
};
```

A stateful deleter can increase the size of `unique_ptr`. Do not assume every
`unique_ptr` has exactly raw-pointer size.

### 9.3 Dedicated Handle Class

For a non-pointer handle, a custom class is often clearer:

```cpp
#include <utility>

class Session {
public:
    explicit Session(int handle) noexcept
        : handle_{handle}
    {
    }

    ~Session() noexcept
    {
        reset();
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Session(Session&& other) noexcept
        : handle_{std::exchange(other.handle_, invalid)}
    {
    }

    Session& operator=(Session&& other) noexcept
    {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, invalid);
        }
        return *this;
    }

    bool valid() const noexcept
    {
        return handle_ != invalid;
    }

private:
    void reset() noexcept
    {
        if (valid()) {
            close_session(handle_);
            handle_ = invalid;
        }
    }

    static constexpr int invalid = -1;
    int handle_{invalid};
};
```

This type is:

- non-copyable because the resource has unique ownership;
- movable because ownership can transfer;
- safely destructible in the empty state.

### 9.4 Mutex Ownership

The standard library already provides RAII lock owners:

```cpp
#include <mutex>

class Counter {
public:
    void increment()
    {
        std::lock_guard<std::mutex> lock{mutex_};
        ++value_;
    }

private:
    std::mutex mutex_;
    int value_{0};
};
```

The lock is released when `lock` leaves scope, including early return or
exception propagation.

Use `std::unique_lock` when the operation needs deferred locking, timed locking,
or explicit unlock/relock behavior. The guard manages lock ownership; it does
not by itself define the complete thread-safety contract of the protected
object.

### 9.5 pImpl With `unique_ptr`

The pImpl idiom moves private representation into an implementation type. It
can reduce header dependencies and help preserve an ABI boundary, at the cost
of allocation, indirection, and additional code.

Header:

```cpp
#include <memory>

class Controller {
public:
    Controller();
    ~Controller();

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&) noexcept;
    Controller& operator=(Controller&&) noexcept;

    void run();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
```

Source file:

```cpp
class Controller::Impl {
public:
    void run()
    {
    }
};

Controller::Controller()
    : impl_{std::make_unique<Impl>()}
{
}

Controller::~Controller() = default;
Controller::Controller(Controller&&) noexcept = default;
Controller& Controller::operator=(Controller&&) noexcept = default;

void Controller::run()
{
    impl_->run();
}
```

Defining the destructor where `Impl` is complete allows `unique_ptr` to invoke
the correct destruction machinery. If value-style copying is required, define
an explicit deep-copy policy rather than copying the pointer.

### 9.6 Placement Construction And Allocator Awareness

Placement new constructs an object in existing storage:

```cpp
#include <cstddef>
#include <new>

alignas(Widget) std::byte storage[sizeof(Widget)];

Widget* widget =
    ::new (static_cast<void*>(storage)) Widget{42};

widget->run();
widget->~Widget();
```

Placement construction does not allocate storage. The caller must guarantee:

- sufficient size;
- correct alignment;
- valid object-lifetime transitions;
- exactly-once destruction;
- safe storage reuse.

This is controlled low-level code for pools, arenas, or specialized storage. It
is not ordinary construction at an arbitrary address.

An allocator supplies storage to allocator-aware containers. C++17's
polymorphic memory resources allow a runtime-selected allocation strategy:

```cpp
#include <cstddef>
#include <memory_resource>
#include <vector>

std::byte storage[1024];
std::pmr::monotonic_buffer_resource arena{
    storage,
    sizeof storage
};

std::pmr::vector<int> values{&arena};
values.push_back(42);
```

Here, `arena` must outlive `values`. A monotonic resource usually releases its
storage as a group rather than reclaiming individual allocations. This can be
useful for phase-based workloads, but it changes memory-reuse and peak-memory
behavior.

Custom allocators, pools, arenas, and small-object optimization are advanced
tools. Choose them only after measuring allocation latency, fragmentation,
locality, memory limits, and lifetime patterns.

## 10. Exception Safety

RAII guarantees cleanup only when the owner types are correct. Mutating
operations still need a state guarantee.

### 10.1 Basic Guarantee

If an operation fails:

- no resources leak;
- invariants remain valid;
- observable state may have changed.

### 10.2 Strong Guarantee

If an operation fails, observable state remains unchanged:

```cpp
void Configuration::replace(std::vector<Entry> replacement)
{
    validate(replacement);       // May throw before commit.
    entries_.swap(replacement);  // Commit using no-throw swap.
}
```

### 10.3 No-Throw Guarantee

The operation does not emit an exception:

```cpp
void Buffer::swap(Buffer& other) noexcept;
```

No-throw operations are particularly important for destructors, release
operations, and commit steps used by strong-guarantee algorithms.

### 10.4 RAII Works Without Exceptions

RAII also works with status returns:

```cpp
Status process()
{
    Session session = acquire_session();
    if (!session.valid()) {
        return Status::unavailable;
    }

    if (!prepare(session)) {
        return Status::prepare_failed;
    }

    return Status::ok;
}
```

Every return destroys `session`. Disabling exceptions does not remove the need
or benefit of RAII.

## 11. Practical Usage

### 11.1 Ownership In Function Signatures

```cpp
void render(const Image& image);                  // Required borrow.
void render_optional(const Image* image);         // Nullable borrow.
void install(std::unique_ptr<Plugin> plugin);     // Transfer ownership.
void retain(std::shared_ptr<Task> task);           // Share ownership.
std::unique_ptr<Plugin> create_plugin();           // Return unique ownership.
```

Ask whether the signature communicates the intended lifetime effect without
reading the implementation.

### 11.2 Value First

Prefer:

```cpp
class Report {
private:
    std::string title_;
    std::vector<Record> records_;
};
```

over:

```cpp
class Report {
private:
    std::string* title_;
    Record* records_;
    std::size_t count_;
};
```

The first version naturally follows Rule of Zero.

### 11.3 Embedded Usage

Embedded constraints may prohibit or restrict dynamic allocation after startup.
That changes the allocation policy, not the RAII principle.

RAII remains useful for:

- fixed-capacity buffer ownership;
- lock guards;
- scoped interrupt-state changes;
- peripheral-session guards;
- temporary registrations;
- restoring a device mode at scope exit.

Review:

- Is allocation allowed in this phase?
- Is allocation or destruction time bounded?
- Can the allocator fragment?
- Is cleanup safe in the current execution context?
- Is `shared_ptr` overhead and allocation justified?
- Can the resource be represented by a statically stored owner?

Do not hide allocation, locks, retries, or potentially slow destruction in a
timing-critical path.

### 11.4 Enterprise Usage

Common production patterns include:

- `unique_ptr` returned by polymorphic factories;
- Rule-of-Zero domain models;
- RAII wrappers around C libraries;
- shared ownership for asynchronous work that genuinely extends lifetime;
- `weak_ptr` for callbacks, caches, and back-references;
- pImpl to isolate implementation and reduce rebuild coupling;
- explicit close/commit APIs when destructor cleanup cannot report failure.

Review asynchronous captures carefully:

```cpp
executor.submit([this] {
    use_state(); // Unsafe if *this may die before execution.
});
```

Possible policies include:

- owner guarantees task completion before destruction;
- capture required values;
- capture shared ownership deliberately;
- capture `weak_ptr` and lock it when the task runs.

Each choice has different lifetime and shutdown behavior.

## 12. Required Comparisons

### 12.1 `malloc/free` Versus `new/delete`

| Aspect | `malloc/free` | `new/delete` |
| --- | --- | --- |
| Language/API | C library | C++ expressions |
| Result | Raw storage | Storage plus object initialization |
| Type | `void*` | Typed pointer |
| Cleanup | Releases storage | Ends lifetime and releases storage |
| Failure | Null | Usually `std::bad_alloc`; nothrow form returns null |
| Resize | `realloc` for suitable C storage | No general object equivalent |
| Preferred C++ use | Isolated C interoperability | Usually hidden behind owner/container |

Never mix allocation families.

### 12.2 Raw Pointer Versus Smart Pointer

| Aspect | Raw pointer/reference | Smart pointer |
| --- | --- | --- |
| Meaning | Usually borrow/view | Ownership policy |
| Cleanup | None inherent | Destructor-driven |
| Address copy | Copies alias | Follows owner-type semantics |
| Best use | Parameters and observers | Owning members and transfer |
| Main risk | Dangling or unclear lifetime | Wrong policy, cycle, escaped borrow |

Raw pointers are not inherently bad. Raw ownership without a clear contract is
the problem.

### 12.3 `unique_ptr` Versus `shared_ptr` Versus `weak_ptr`

| Aspect | `unique_ptr` | `shared_ptr` | `weak_ptr` |
| --- | --- | --- | --- |
| Ownership | Exclusive | Shared | None |
| Copyable | No | Yes | Yes |
| Movable | Yes | Yes | Yes |
| Extends lifetime | Yes, as sole owner | Yes | No |
| Typical use | Default dynamic owner | Genuine co-ownership | Observer/back-reference |
| Main hazard | Lost `release()` result | Cycle and unnecessary ownership | Use without successful `lock()` |

### 12.4 Manual Cleanup Versus RAII

| Path | Manual cleanup | RAII |
| --- | --- | --- |
| Normal return | Explicit call | Destructor |
| Early return | Each path must call cleanup | Destructor |
| Exception | Catch or duplicate cleanup | Stack unwinding |
| Composition | Manual order | Reverse construction order |
| Review focus | Every control-flow path | Owner invariant and lifetime |

### 12.5 Shallow Versus Deep Copy

| Aspect | Shallow handle copy | Deep copy |
| --- | --- | --- |
| Resource | Aliased | Independent |
| Raw unique owner | Usually incorrect | Supports value semantics |
| Cost | Often lower | May allocate and copy |
| Alternative | Delete copy or share deliberately | Prefer value members |

Memberwise copy is not universally shallow. A `std::vector` member performs its
own value-preserving copy.

### 12.6 Rule Of Three Versus Five Versus Zero

| Rule | Meaning | Use |
| --- | --- | --- |
| Three | Review destructor and two copy operations together | Manual copyable owner |
| Five | Also review two move operations | Manual movable owner |
| Zero | Let resource-owning members implement all five | Preferred application design |

## 13. Common Bugs

### 13.1 Leak On Early Exit

```cpp
Widget* widget = new Widget;
if (!configure(*widget)) {
    return; // Leak.
}
delete widget;
```

Use a scoped value or smart pointer.

### 13.2 Double Release

Two owners release one resource:

```cpp
std::shared_ptr<Widget> first{new Widget};
// std::shared_ptr<Widget> second{first.get()}; // Separate control block: wrong.
```

Copy `first` instead:

```cpp
std::shared_ptr<Widget> second = first;
```

### 13.3 Use After Owner Reset

```cpp
auto owner = std::make_unique<Widget>();
Widget* borrowed = owner.get();
owner.reset();
// borrowed->run(); // Undefined behavior.
```

### 13.4 Allocation-Family Mismatch

Using `free` on `new` storage or `delete` on `malloc` storage is undefined
behavior.

### 13.5 Broken Copy Assignment

```cpp
delete[] data_;
data_ = new int[other.size_]; // If this throws, the member is dangling.
```

Allocate replacement before committing, or use a standard owner.

### 13.6 Incorrect Move

A move operation that copies the pointer but does not empty the source creates
two apparent owners.

### 13.7 False `noexcept`

If a `noexcept` move operation performs an operation that can throw, an emitted
exception terminates the program.

### 13.8 Shared Cycle

Mutual `shared_ptr` members can keep an unreachable object graph alive.

### 13.9 `expired()` Check Race

```cpp
if (!weak.expired()) {
    // Object can expire before a later separate access.
}
```

Use:

```cpp
if (auto owner = weak.lock()) {
    owner->run();
}
```

### 13.10 Shared Ownership Is Not Object Synchronization

Reference counting does not protect mutations to the managed object. Concurrent
access still needs an appropriate synchronization design.

### 13.11 Throwing Destructor

An exception escaping a destructor during active stack unwinding causes
termination.

### 13.12 Misusing Placement New

Placement new requires:

- sufficient storage;
- correct alignment;
- explicit lifetime reasoning;
- matching destruction;
- a policy for storage reuse.

It is an advanced tool, not an ordinary replacement for `new`.

## 14. Debugging

### 14.1 Strict Build

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wnon-virtual-dtor -Werror -O0 -g3 \
    resource_example.cpp -o resource_example
```

Warnings can expose suspicious special-member behavior, conversions, and
polymorphic destruction.

### 14.2 AddressSanitizer And UndefinedBehaviorSanitizer

```bash
c++ -std=c++17 -O1 -g3 \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer \
    resource_example.cpp -o resource_example_sanitize

./resource_example_sanitize
```

Executed paths may reveal:

- use after free;
- double free;
- invalid access;
- selected lifetime and alignment defects;
- leaks on supported configurations.

A clean sanitizer run does not prove unexecuted paths are correct.

### 14.3 ThreadSanitizer

Use ThreadSanitizer for concurrent managed-object access:

```bash
c++ -std=c++17 -O1 -g3 \
    -fsanitize=thread \
    shared_state.cpp -o shared_state_tsan
```

Do not combine the conclusion "the reference count is safe" with "the object is
race-free."

### 14.4 Debugger Workflow

Useful breakpoints:

```text
break Buffer::Buffer
break Buffer::~Buffer
break Buffer::operator=
break FileCloser::operator()
run
bt
```

Inspect:

- owner object addresses;
- managed resource addresses;
- copy and move calls;
- source state after a move;
- destruction order;
- whether a custom deleter runs exactly once.

### 14.5 Contract Tests

Test:

- normal return;
- every early return;
- acquisition failure;
- exception after each acquisition;
- copy construction and assignment;
- move construction and assignment;
- self-assignment and self-move if supported;
- reset and empty-state destruction;
- `weak_ptr::lock()` before and after expiration;
- shared cycles;
- close/commit failure separately from destructor fallback.

An acquisition/release fake can verify exactly-once cleanup.

## 15. Best Practices

- Prefer values and Rule-of-Zero classes.
- Prefer scoped objects over dynamic allocation.
- Represent ownership explicitly.
- Treat raw pointers and references as borrows unless clearly documented
  otherwise.
- Use `std::unique_ptr` as the default dynamic owner.
- Use `std::shared_ptr` only for real shared lifetime.
- Use `std::weak_ptr::lock()` for shared-lifetime observation.
- Immediately place acquired resources under an RAII owner.
- Match every resource with its exact release API.
- Keep naked `new` and `delete` inside low-level ownership code, if used at all.
- Make destructors and release paths non-throwing.
- Provide explicit close/commit operations when cleanup failure matters.
- Default or delete special members when that expresses the contract.
- Mark move operations `noexcept` only when true.
- Leave moved-from objects valid.
- State exception guarantees for mutating operations.
- Avoid hidden allocation and slow destruction in bounded-latency paths.
- Measure allocator and reference-count costs on the target workload.
- Review asynchronous callbacks for dangling captures and shutdown order.

## 16. Interview Readiness

### Beginner

You should be able to answer:

1. What is RAII?
2. Why does RAII manage more than memory?
3. What is the difference between ownership and borrowing?
4. Why must allocation and deallocation families match?
5. Why is `unique_ptr` usually the default smart pointer?
6. What happens when an automatic RAII object leaves scope?

Short model answer:

> RAII binds a resource to an object's lifetime. Construction or adoption
> establishes ownership, and the destructor releases the resource. This gives
> one cleanup path for normal return, early return, and exception unwinding.

### Mid-Level

Be ready to explain:

- Rule of Three, Five, and Zero;
- memberwise copy versus semantic deep copy;
- why `std::move` is a cast;
- moved-from object requirements;
- when move operations should be `noexcept`;
- `unique_ptr` transfer through function boundaries;
- `shared_ptr` control blocks and cycles;
- `weak_ptr::lock()`;
- custom deleters;
- basic versus strong exception guarantees.

### Senior

Be ready to reason about:

- ownership encoded in a complete API;
- asynchronous lifetime extension and shutdown;
- explicit close/commit versus destructor cleanup;
- shared ownership justification and contention;
- pImpl ownership and incomplete types;
- allocation policy in embedded or real-time code;
- pools, arenas, and polymorphic allocators at a design level;
- exception guarantees for resource-owning operations;
- why reference-count safety differs from managed-object thread safety.

### Common Interview Traps

- "`std::move` moves the object."
- "A moved-from object cannot be used."
- "`shared_ptr` makes an object thread-safe."
- "`weak_ptr::expired()` reserves the object."
- "`unique_ptr` is guaranteed to be one pointer wide."
- "Nulling one pointer fixes every dangling alias."
- "Rule of Five means implementing every function manually."
- "All move operations must be `noexcept`, even when they can throw."
- "RAII is only needed when exceptions are enabled."

## 17. Practice

### Basic

1. Replace a raw owning array with `std::vector`.
2. Convert a factory returning `T*` into one returning `std::unique_ptr<T>`.
3. Label each parameter in an API as owner, required borrow, or nullable
   borrow.
4. Draw the cleanup matrix for a function with two resources and three early
   returns.

### Intermediate

1. Implement a movable, non-copyable RAII wrapper for a fake integer handle.
2. Write tests proving its release function runs exactly once.
3. Create a `shared_ptr` cycle and repair one edge with `weak_ptr`.
4. Write a deep-copying buffer, then replace it with `std::vector`.
5. Demonstrate cleanup when the second member constructor throws.

### Advanced

1. Design a pImpl class whose implementation is owned by `unique_ptr`.
2. Compare `make_shared` with separately allocated object/control-block
   ownership while a `weak_ptr` remains.
3. Design a startup-only allocation policy for an embedded service.
4. Review an asynchronous callback for dangling captures and shutdown races.
5. Specify basic, strong, and no-throw guarantees for a resource-owning type.
6. Design a small arena and document alignment, construction, destruction,
   exhaustion, and reset policy.

## 18. Summary

- Resource management is lifetime management, not merely memory allocation.
- Ownership means responsibility for cleanup.
- Borrowing permits access without cleanup responsibility.
- RAII connects resource ownership to object lifetime.
- Rule of Zero is the preferred application-level design.
- Manual owners require deliberate copy, move, and cleanup semantics.
- `unique_ptr` models exclusive ownership.
- `shared_ptr` models genuine shared ownership.
- `weak_ptr` observes shared lifetime without extending it.
- Smart pointers do not solve every borrowed-lifetime or concurrency problem.
- Exception safety requires both cleanup safety and valid state transitions.
- Every resource must have a documented owner and cleanup path on every exit.

## 19. Reference Notes

- C++ Core Guidelines: resource management, RAII, ownership, smart pointers,
  Rule of Zero, and explicit allocation guidance.
- cppreference C++: object lifetime, special member functions, `noexcept`,
  `std::move`, and smart-pointer behavior.
- ISO C++ standard or working draft for exact language and library rules.
- Compiler sanitizer documentation for supported diagnostics and limitations.
