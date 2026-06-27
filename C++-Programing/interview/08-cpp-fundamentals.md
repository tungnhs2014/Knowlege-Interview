# 08 - C++ Fundamentals: Interview Pack

## How To Use This Pack

For each question:

1. Give the short answer first.
2. Explain the language mechanism.
3. Anchor the answer in code or an API contract.
4. Connect it to production design or debugging.
5. Identify traps instead of repeating slogans.
6. Handle follow-up questions without changing the original claim.

The examples use C++17 unless stated otherwise.

## Beginner Questions

### 1. What does C++ add beyond C?

**Short answer**

C++ adds language mechanisms such as references, classes, constructors,
destructors, function and operator overloading, namespaces, templates, and
stronger abstraction tools while retaining low-level control.

**Deep explanation**

C and C++ share much syntax and can interoperate through carefully designed
interfaces, but C++ is not simply “C with classes.” Its object model changes how
programs express initialization, lifetime, invariants, cleanup, and API
selection.

In C, a type often needs separate initialization and cleanup functions. C++ can
make initialization part of object creation and cleanup part of lifetime end.
References can express required borrowed access, overloads can group one
operation for several types, and namespaces organize names without prefixing
every symbol manually.

These mechanisms improve correctness only when used with explicit ownership and
lifetime contracts. C++ still permits raw pointers, unchecked conversions, and
undefined behavior.

**C/C++ code or API anchor**

```c
typedef struct {
    int channel;
} Sensor;

int sensor_init(Sensor *sensor, int channel);
void sensor_deinit(Sensor *sensor);
```

```cpp
class Sensor {
public:
    explicit Sensor(int channel)
        : channel_{channel}
    {
    }

    int channel() const
    {
        return channel_;
    }

private:
    int channel_;
};
```

**Production and debug angle**

At a C/C++ boundary, keep a narrow C API and hide C++ implementation details.
When debugging, determine whether a failure is caused by a C-style manual
contract, C++ lifetime behavior, or binary-interface mismatch.

**Common traps**

- Saying C++ is a strict superset of C.
- Assuming C++ requires object-oriented design.
- Equating “no garbage collector” with manual `new`/`delete`.
- Claiming classes automatically make code safe.

**Follow-up questions**

- Which valid C programs are not valid C++?
- When is C a better public ABI?
- Which C++ mechanisms directly affect object lifetime?

### 2. Compare a pointer and a reference.

**Short answer**

A pointer is an object that can store an address-like value, can normally be
null, and can be reseated. A reference is initialized as an alias and cannot be
reseated. Neither automatically owns the referred object.

**Deep explanation**

An lvalue reference `T&` must be initialized. Assignment through the reference
changes the referred object; it does not make the reference refer elsewhere.

A pointer can represent optional access with `nullptr`, can be reassigned, and
supports pointer arithmetic when that arithmetic is valid. A reference usually
expresses a required existing object.

Both can dangle when the referred object's lifetime ends. A reference is not
best described as a “const pointer”: implementations often use addresses, but
the language gives references different rules and semantics.

**C/C++ code or API anchor**

```cpp
void reset_required(Device& device);
void reset_optional(Device *device);

int first = 10;
int second = 20;

int& alias = first;
alias = second; // first becomes 20; alias is still bound to first
```

**Production and debug angle**

Choose the form based on nullability, borrowing, mutation, and ownership. During
a crash investigation, identify the owner and verify that every pointer or
reference is used only while the object is alive.

**Common traps**

- Saying references cannot dangle.
- Treating a reference as an owner.
- Assuming a reference has a specified address-sized representation.
- Using a pointer for a required dependency without a null policy.
- Claiming a reference can safely bind to every temporary.

**Follow-up questions**

- When should a parameter be `const T&`?
- Can a function return a reference?
- What is a reference member's lifetime requirement?

### 3. Compare `struct` and `class` in C++.

**Short answer**

They have the same main language capabilities. `struct` defaults to public
member and base access; `class` defaults to private member and base access.

**Deep explanation**

Both may contain data, member functions, constructors, destructors, static
members, friends, operators, templates, and inheritance.

The usual design convention is:

- `struct` for a simple passive aggregate with public state;
- `class` for a type that protects an invariant behind an interface.

That convention communicates intent, but it is not a language restriction.
Likewise, `struct` is not synonymous with aggregate: aggregate status is
determined by version-specific language rules.

**C/C++ code or API anchor**

```cpp
struct Point {
    int x;
    int y;
};

class Percentage {
public:
    explicit Percentage(int value)
        : value_{value}
    {
    }

    int value() const
    {
        return value_;
    }

private:
    int value_;
};
```

**Production and debug angle**

Code review should ask whether the type has an invariant. Public writable fields
are reasonable for a transparent data record but suspicious when fields must
remain mutually consistent.

**Common traps**

- Saying `struct` is C-style and `class` is object-oriented.
- Saying a `struct` cannot have private members.
- Assuming every `struct` is ABI-compatible with C.
- Treating aggregate initialization and `struct` as synonyms.

**Follow-up questions**

- When should a public data record become a class?
- Does access control change object layout?
- Can a C++ `struct` have a destructor?

### 4. What are constructors and destructors?

**Short answer**

A constructor initializes an object and should establish its invariant. A
destructor runs when the object's lifetime ends in the applicable context and
releases resources owned by that object.

**Deep explanation**

A constructor has the class name and no return type. It initializes bases and
members before its body runs. Constructors may be default, parameterized, copy,
move, converting, or delegating.

A destructor has the form `~Type()`. For a normal local object, it runs on scope
exit, including early return and exception unwinding. After the destructor body,
members and bases are destroyed in reverse construction order.

Constructors do not inherently allocate memory, and destructors do not
inherently free heap memory. They manage whatever invariant and ownership the
type defines.

**C/C++ code or API anchor**

```cpp
class Session {
public:
    explicit Session(int id)
        : id_{id}
    {
        // Establish invariant.
    }

    ~Session()
    {
        // Release resources owned by this Session.
    }

private:
    int id_;
};
```

**Production and debug angle**

Set breakpoints in constructors and destructors when investigating double
release, unexpected copies, premature destruction, or leaked ownership. Do not
depend on an exact number of copy/move calls because copy elision may apply.

**Common traps**

- Assigning members in the constructor body instead of initializing them.
- Believing a destructor is called for an object whose construction never
  completed.
- Owning a raw handle while keeping unsafe compiler-generated copying.
- Throwing from a destructor during stack unwinding.

**Follow-up questions**

- In what order are members initialized?
- What happens if construction throws?
- Why does resource ownership affect copy operations?

### 5. What is function overloading, and how is it different from overriding?

**Short answer**

Overloading provides multiple functions with the same name but different
parameter interfaces. Overriding replaces virtual behavior in a derived class.

**Deep explanation**

For an overloaded call, the compiler forms candidates, filters viable
functions, ranks conversion sequences, and requires one best viable function.
Return type alone cannot distinguish ordinary overloads.

Overriding requires inheritance and a virtual base function. It is primarily a
runtime-polymorphism topic and belongs to the OOP chapter.

Overloading should group one conceptual operation. Unrelated meanings hidden
behind one name make APIs harder to understand and can produce surprising
implicit conversions.

**C/C++ code or API anchor**

```cpp
void log(int value);
void log(double value); // overload

// int read();
// double read();       // invalid: return type alone is insufficient
```

**Production and debug angle**

When an overload call fails, list candidates, viable functions, and required
conversions. Do not immediately add a cast; the ambiguity may reveal a weak API.

**Common traps**

- Confusing overloading, overriding, and name hiding.
- Saying name mangling performs overload resolution.
- Expecting private candidates to disappear before overload selection.
- Overloading functions with unrelated semantics.

**Follow-up questions**

- What is an ambiguous overload?
- Can default arguments create ambiguity?
- Why can C not expose the same source-level overload set?

## Mid-Level Questions

### 6. Explain object lifetime and how it differs from storage duration.

**Short answer**

Storage duration describes how long storage exists. Object lifetime describes
when an object of a particular type exists in that storage and may be used as
that object.

**Deep explanation**

For an ordinary local class object, storage is available, initialization begins,
bases and members are initialized, the constructor body runs, and then the
object is usable. At lifetime end, the destructor body runs, followed by member
and base destruction.

An address may remain after lifetime ends, but using it to access the dead
object is invalid. Conversely, advanced C++ can reuse storage for another object,
so storage identity and object identity are not interchangeable.

Scope is where a name is visible. Linkage determines whether declarations can
refer to the same entity across scopes or translation units. These concepts
interact but are not synonyms.

**C/C++ code or API anchor**

```cpp
const int *borrow()
{
    int local = 42;
    return &local; // invalid: local's lifetime ends on return
}
```

```cpp
void use()
{
    static int persistent = 0; // block scope, static storage duration
    int temporary = 1;         // block scope, automatic storage duration
    ++persistent;
    (void)temporary;
}
```

**Production and debug angle**

For use-after-free or use-after-scope, draw an ownership/lifetime timeline. ASan
can detect selected executed lifetime violations, but a clean run does not prove
all borrows are valid.

**Common traps**

- Equating scope with lifetime.
- Assuming every local value is physically stored on a stack.
- Treating a surviving pointer value as proof the object still exists.
- Returning a pointer or reference to an automatic local object.

**Follow-up questions**

- When does a temporary's lifetime extend?
- What is static initialization order risk?
- Can storage be reused for another object?

### 7. Compare default, value, direct, copy, and list initialization.

**Short answer**

Initialization syntax affects whether scalars are initialized, which constructor
is selected, whether explicit constructors participate, and whether narrowing
is rejected.

**Deep explanation**

Important forms include:

- default initialization: `T object;`
- value initialization: `T object{};`
- direct initialization: `T object(args);`
- copy initialization: `T object = value;`
- direct-list initialization: `T object{args};`
- copy-list initialization: `T object = {args};`

For an ordinary local `int`, `int value;` leaves an indeterminate value, while
`int value{};` produces zero.

List initialization rejects many narrowing conversions. It also has overload
resolution rules that can prefer `std::initializer_list` constructors, so braces
and parentheses are not universally equivalent.

**C/C++ code or API anchor**

```cpp
int first;       // indeterminate
int second{};    // zero
int third(3);    // direct initialization
int fourth = 4;  // copy initialization
int fifth{5};    // direct-list initialization

// int invalid{3.5}; // rejected narrowing
```

**Production and debug angle**

Enable conversion and uninitialized-value diagnostics. When constructor
selection is surprising, reduce the call and inspect whether an
`initializer_list` overload is participating.

**Common traps**

- Saying braces are always superior and interchangeable with parentheses.
- Reading an uninitialized scalar.
- Believing an explicit cast validates range.
- Confusing default construction with zero initialization in every context.

**Follow-up questions**

- How does `explicit` affect copy initialization?
- Why can braces choose a different constructor?
- What does aggregate initialization mean?

### 8. How should a function choose between value, `T&`, `const T&`, and `T*`?

**Short answer**

Choose based on ownership/copy intent, required mutation, required borrowing,
and nullability, not merely on assumed machine size.

**Deep explanation**

Use value for small values, independent values, or when the function should own
or consume its own copy. Use `T&` for a required mutable borrow. Use `const T&`
for a required read-only borrow when copying is not desired. Use a pointer when
nullability or explicit address semantics are part of the contract.

Passing by value can copy, move, or be elided. The implementation may use
registers or other ABI mechanisms; “value means copy onto the stack” is not a
language guarantee.

`const T&` is not always fastest. For cheap scalar types it can add indirection,
and for values the function needs to own, pass-by-value can simplify copy/move
design.

**C/C++ code or API anchor**

```cpp
void set_retry_count(int count);             // cheap value
void normalize(Config& config);              // required mutation
bool is_valid(const Config& config);          // required read-only borrow
void attach_observer(Observer *observer);     // nullable borrow
```

**Production and debug angle**

Document whether a stored pointer/reference may outlive the call. Many API bugs
come from a function appearing to borrow temporarily but retaining the address.

**Common traps**

- Treating every large type as `const T&` without considering ownership.
- Using non-const references for hidden output parameters.
- Using pointers without a null policy.
- Assuming a reference is owning or address-sized.

**Follow-up questions**

- When is pass-by-value useful for a sink parameter?
- How would a view type change the API?
- Should output be returned or written through a parameter?

### 9. Why use member initializer lists, and what is the real initialization order?

**Short answer**

Member initializer lists initialize bases and members directly. Actual order is
base order followed by member declaration order, not the order written in the
initializer list.

**Deep explanation**

Before the constructor body begins, virtual bases, direct bases, and data
members are initialized according to language-defined order. The body can only
operate on already initialized members.

References, `const` members, and members without default constructors must be
initialized in the initializer list. Assigning in the body may perform needless
default construction plus assignment and cannot repair every type.

Write the initializer list in declaration order so the code visually matches
reality.

**C/C++ code or API anchor**

```cpp
class Range {
public:
    Range(int minimum, int span)
        : minimum_{minimum},
          maximum_{minimum_ + span}
    {
    }

private:
    int minimum_;
    int maximum_;
};
```

Actually broken declaration dependency:

```cpp
class Broken {
public:
    Broken(int value)
        : value_{value},
          doubled_{value_ * 2}
    {
    }

private:
    int doubled_;
    int value_;
};
```

Despite the initializer-list spelling, `doubled_` initializes first because it
is declared first. Its initializer reads `value_` before `value_` has been
initialized, producing undefined behavior.

**Production and debug angle**

Enable `-Wreorder` or equivalent diagnostics. During review, compare declaration
order, initializer dependencies, and reverse destruction order.

**Common traps**

- Believing initializer-list spelling controls order.
- Assigning reference or const members in the body.
- Calling virtual behavior from construction as though the most-derived object
  were fully active.
- Hiding dependency between members.

**Follow-up questions**

- What is a default member initializer?
- How does a delegating constructor work?
- What is the destruction order?

### 10. Why should converting constructors often be `explicit`?

**Short answer**

`explicit` prevents unintended implicit conversion through a constructor,
making call-site intent visible and reducing surprising overload selection.

**Deep explanation**

A constructor callable with one argument can be a converting constructor. If it
is not `explicit`, the compiler may create the class object while converting an
argument for assignment, initialization, or a function call.

Implicit conversion is appropriate only when the source and destination have a
natural, unsurprising relationship. Unit types, handles, IDs, and validated
domain values usually benefit from explicit construction.

`explicit` affects contexts differently. Direct initialization can use an
explicit constructor; copy initialization cannot use it as an implicit
conversion.

**C/C++ code or API anchor**

```cpp
class Milliseconds {
public:
    explicit Milliseconds(int value)
        : value_{value}
    {
    }

private:
    int value_;
};

void wait_for(Milliseconds timeout);

// wait_for(1000);            // rejected
wait_for(Milliseconds{1000}); // clear unit and intent
```

**Production and debug angle**

When adding a constructor changes which overload is called, inspect whether it
introduced a user-defined conversion. API reviews should treat implicit
conversion as part of the public contract.

**Common traps**

- Assuming every one-argument constructor is always harmful.
- Adding casts at call sites without examining the conversion design.
- Forgetting that defaulted extra parameters can make a constructor converting.
- Confusing `explicit` with runtime validation.

**Follow-up questions**

- Can conversion operators be explicit?
- How does list initialization interact with explicit constructors?
- When is implicit conversion desirable?

### 11. Explain overload resolution and a common ambiguity.

**Short answer**

The compiler finds candidates, filters viable functions, ranks conversion
sequences, and selects one unique best viable function. If no unique best
function exists, the call is ambiguous.

**Deep explanation**

An overload set may include functions found by ordinary lookup, member lookup,
and argument-dependent lookup. Viability checks argument count, defaults,
conversion availability, and other constraints.

Conversion ranking includes exact matches, promotions, standard conversions,
and user-defined conversions, but the full model has additional reference,
list-initialization, template, and tie-breaker rules.

Access control does not simply remove private declarations before overload
resolution. A selected inaccessible function can still make the call fail.

**C/C++ code or API anchor**

```cpp
void report(int);
void report(double);

long value = 1;
// report(value); // both may require standard conversions: ambiguous
```

A design repair may be:

```cpp
void report_integer(long value);
void report_real(double value);
```

**Production and debug angle**

Use the compiler diagnostic as a candidate list, then write the conversion for
each argument. If callers repeatedly need casts, redesign names or parameter
types.

**Common traps**

- Reducing all ranking to one oversimplified hierarchy.
- Expecting return type to participate in ordinary overload distinction.
- Saying mangled names choose the overload.
- Ignoring implicit constructors and conversion operators.

**Follow-up questions**

- How does ADL add candidates?
- Can a deleted function win overload resolution?
- What is the effect of default arguments?

### 12. What makes an operator overload well designed?

**Short answer**

An operator overload should preserve the familiar meaning, algebraic
expectations, and side-effect profile of the built-in operator. Use a named
function when the operation would be surprising.

**Deep explanation**

An overloaded operator is a function selected through operator syntax. It
cannot create new operator tokens, change precedence or associativity, change
the operator's operand count, or redefine an operator when every operand is a
built-in type.

Good overloads make a domain type behave naturally. Equality should compare
meaningful value identity. Addition should produce a sum-like result. Stream
insertion should write a representation to the supplied stream.

Keep conversion and symmetry in mind. A non-member overload is often suitable
when both operands should convert symmetrically. Friendship is justified only
when the operator genuinely needs private representation access.

**C/C++ code or API anchor**

```cpp
#include <limits>
#include <stdexcept>

struct Millivolts {
    int value;
};

constexpr Millivolts operator+(
    Millivolts left,
    Millivolts right)
{
    if ((right.value > 0
            && left.value > std::numeric_limits<int>::max() - right.value)
        || (right.value < 0
            && left.value < std::numeric_limits<int>::min() - right.value)) {
        throw std::overflow_error{"millivolt addition overflow"};
    }

    return Millivolts{left.value + right.value};
}
```

**Production and debug angle**

Test identities, boundaries, overflow policy, and consistency between related
operators. Surprising hidden I/O, allocation, locking, or mutation inside a
simple-looking operator is a maintainability warning.

**Common traps**

- Making `operator+` mutate its left operand.
- Overloading an operator only because the syntax is shorter.
- Assuming precedence can be changed.
- Creating inconsistent `==` and ordering behavior.
- Using friendship when public accessors are sufficient.

**Follow-up questions**

- When should an operator be a member?
- Why is symmetry relevant?
- Should integer overflow policy be part of a units type?

### 13. Explain static members and friendship.

**Short answer**

A static member belongs to the class rather than each object. Friendship grants
selected non-public access but does not make a function a member and is neither
inherited nor transitive.

**Deep explanation**

A static data member has one class-associated entity rather than one subobject
per instance. A static member function has no `this` pointer and can directly
access only static members.

A friend function or friend class can name private and protected members. This
can support symmetric non-member operators or tightly coupled implementation
helpers, but broad friendship weakens encapsulation and increases coupling.

An inline static data member can be defined in the class definition beginning
in C++17, subject to the applicable ODR rules.

**C/C++ code or API anchor**

```cpp
class Connection {
public:
    Connection()
    {
        ++active_count_;
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    ~Connection()
    {
        --active_count_;
    }

    static int active_count()
    {
        return active_count_;
    }

private:
    inline static int active_count_{0};
};
```

**Production and debug angle**

Shared static mutable state needs a concurrency policy. A counter like this is
not thread-safe. During review, ask whether friendship protects one coherent
operation or merely bypasses a poor public interface.

**Common traps**

- Treating a static member as per-object state.
- Accessing non-static members without an object.
- Assuming friendship is reciprocal, inherited, or transitive.
- Using a friend class as a default collaboration mechanism.
- Copying an object whose constructor/destructor update a static count without
  defining count semantics.

**Follow-up questions**

- When is a namespace-level function better than a static member function?
- How would you make the counter thread-safe?
- Why might a non-member operator be a friend?

## Senior Questions

### 14. Explain `inline` without starting with performance.

**Short answer**

`inline` gives a function or variable specific One Definition Rule behavior that
supports equivalent definitions in multiple translation units. Optimizer
call-site inlining is a separate decision.

**Deep explanation**

An inline function's definition must be reachable where required. Multiple
definitions are permitted only under the applicable ODR requirements, including
equivalent token sequences and consistent name lookup.

The optimizer may inline a function that lacks the keyword and may emit a normal
call for an inline function. Link-time optimization further separates optimizer
visibility from source spelling.

Class-body function definitions and `constexpr` functions are implicitly inline
under the applicable rules. Function templates are commonly defined in headers
for instantiation visibility, not because every template is automatically
declared inline.

**C/C++ code or API anchor**

```cpp
// clamp.hpp
#ifndef CLAMP_HPP
#define CLAMP_HPP

inline int clamp_low(int value)
{
    return value < 0 ? 0 : value;
}

#endif
```

**Production and debug angle**

ODR violations can produce link errors, silent behavior differences, or
configuration-dependent failures. Compare preprocessed definitions, macros,
include order, and compiler options across translation units.

**Common traps**

- Saying inline functions are exempt from the ODR.
- Treating `inline` as a guaranteed optimization request.
- Giving different macro-dependent definitions in different translation units.
- Assuming a linker will always diagnose an ODR violation.

**Follow-up questions**

- What are inline variables?
- How can macros create different definitions?
- How does link-time optimization relate to call-site inlining?

### 15. Design a stable C boundary around a C++ implementation.

**Short answer**

Expose a narrow C header with C language linkage, C-compatible values, opaque
handles, explicit ownership functions, and an error policy that prevents C++
exceptions from crossing the boundary.

**Deep explanation**

`extern "C"` specifies language linkage for supported declarations. A common
implementation effect is a C-compatible external symbol, but it is not a
general guarantee that C++ classes, templates, exceptions, RTTI, standard
containers, or object layouts form a portable C ABI.

An opaque handle hides the C++ type. Creation returns a status and handle;
operations validate pointers and translate exceptions/errors; destruction
releases the implementation object.

Versioning must define type sizes, alignment, calling convention, ownership,
thread safety, and compatibility policy.

**C/C++ code or API anchor**

```c
/* sensor_api.h */
#ifndef SENSOR_API_H
#define SENSOR_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SensorHandle SensorHandle;

int sensor_create(int channel, SensorHandle **out_handle);
int sensor_read(SensorHandle *handle, int *out_value);
void sensor_destroy(SensorHandle *handle);

#ifdef __cplusplus
}
#endif

#endif
```

**Production and debug angle**

Compile C sources as C and C++ sources as C++. Inspect symbols with `nm`,
`readelf`, `objdump`, `c++filt`, or platform equivalents. Check architecture,
visibility, calling convention, runtime library, and exception containment.

**Common traps**

- Exposing `std::string`, `std::vector`, or a C++ class directly.
- Letting exceptions cross into C.
- Omitting ownership/nullability contracts.
- Assuming every linker error is name mangling.
- Destroying an opaque handle with the wrong module/runtime.

**Follow-up questions**

- How would you report detailed errors?
- How would you version the ABI?
- How would you make the handle thread-safe?

### 16. Review a class that owns a raw resource.

**Short answer**

A destructor alone is insufficient. Copying, moving, failure behavior, and
ownership transfer must be designed together; otherwise compiler-generated
copying can duplicate the raw handle and cause double release.

**Deep explanation**

If a class owns a resource, its invariant includes exactly who releases it.
Memberwise copying of a raw handle creates two objects that appear to own the
same resource.

At minimum, disable copying until correct copy semantics exist. A movable
resource wrapper transfers ownership and leaves the source non-owning. In many
cases, use an existing RAII type or standard smart pointer with an appropriate
deleter instead of implementing raw ownership.

This topic leads to the Rule of Zero, Rule of Five, and exception safety in the
resource-management chapter.

**C/C++ code or API anchor**

```cpp
class File {
public:
    explicit File(const char *path)
        : handle_{std::fopen(path, "rb")}
    {
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    ~File()
    {
        if (handle_ != nullptr) {
            std::fclose(handle_);
        }
    }

private:
    std::FILE *handle_;
};
```

**Production and debug angle**

Use ASan or platform diagnostics for double free/close where detectable. Trace
creation, ownership transfer, copies, moves, and destruction. Confirm that
failure during construction cannot leak a partially acquired resource.

**Common traps**

- Assuming a destructor automatically makes copying safe.
- Using a shallow copy for unique ownership.
- Manually calling the destructor.
- Releasing a borrowed resource.
- Forgetting self-assignment and failure guarantees in custom assignment.

**Follow-up questions**

- How would you implement move operations?
- What is the Rule of Zero?
- Should construction fail with an exception or a factory result?

### 17. What lifetime risks exist when a class stores references or lambdas?

**Short answer**

Stored references and reference captures are non-owning borrows. The referred
objects must outlive every use of the containing object or closure.

**Deep explanation**

A reference member must be initialized and cannot be reseated. Copying the
containing object normally copies the binding, so both objects refer to the same
external object.

A lambda captured by reference stores borrowing behavior. Capturing `this`
stores a pointer to the current object, not ownership. Deferred execution,
queues, callbacks, and asynchronous work commonly outlive the captured scope.

Value capture can extend the captured value's lifetime because the closure owns
its copy, but copying may be expensive or may copy the wrong semantic state.

**C/C++ code or API anchor**

```cpp
auto make_bad_callback()
{
    int count = 0;
    return [&count]() {
        return ++count; // dangling after make_bad_callback returns
    };
}
```

Safer value capture:

```cpp
auto make_counter()
{
    int count = 0;
    return [count]() mutable {
        return ++count;
    };
}
```

**Production and debug angle**

Review every callback registration for cancellation and owner lifetime. ASan
may catch executed use-after-scope. Tests should invoke callbacks after the
original scope exits when validating ownership behavior.

**Common traps**

- Assuming `[&]` is merely a performance choice.
- Capturing `this` for work that can outlive the object.
- Storing a `const T&` parameter beyond the call.
- Believing const reference binding universally extends temporary lifetime.

**Follow-up questions**

- When is value capture undesirable?
- How should callback cancellation work?
- What does `[*this]` change in C++17?

### 18. Why should tests avoid exact copy/move counts?

**Short answer**

C++ permits and, in important C++17 cases, guarantees copy elision. Correct code
may construct the result directly, so observable copy/move counts vary with the
language case and implementation.

**Deep explanation**

Copy elision is not merely a generic optimizer removing arbitrary side effects.
The language defines specific cases where source and target can be treated as
one object or where a prvalue initializes the destination directly.

Tests that assert “one copy constructor call” can fail when code is improved,
the language mode changes, or elision applies. Tests should assert final value,
ownership, and destruction behavior.

When teaching lifetime, distinguish optional named return value optimization
from guaranteed C++17 prvalue construction cases.

**C/C++ code or API anchor**

```cpp
class Reading {
public:
    explicit Reading(int value)
        : value_{value}
    {
    }

    int value() const
    {
        return value_;
    }

private:
    int value_;
};

Reading make_reading()
{
    return Reading{42};
}
```

**Production and debug angle**

Constructor breakpoints may not trigger as expected. Inspect generated code only
when performance evidence requires it. Functional tests should verify
`make_reading().value() == 42`, not incidental constructor logging.

**Common traps**

- Calling all elision “compiler optimization.”
- Requiring optional NRVO for correctness.
- Assuming `std::move` on every return helps elision.
- Using constructor side effects as business behavior.

**Follow-up questions**

- What is guaranteed in C++17?
- What is NRVO?
- Why can `return std::move(local);` be counterproductive?

## Coding Tasks

### 19. Coding task: design a validated configuration type.

**Prompt**

Create a C++17 type for a sensor channel and sample period. Channel must be from
0 through 15, and period must be positive. The object must never expose an
invalid state.

**Short answer**

Use a class with private members, an explicit constructor or a checked factory,
member initializer lists, and const accessors.

**Deep explanation**

The task tests initialization, invariants, access control, conversion policy,
and error handling. If exceptions fit the project, construction can reject bad
input. If exceptions are disabled, use a factory returning status plus an
object/result, or validate before constructing an infallible representation.

**C/C++ code or API anchor**

```cpp
#include <stdexcept>

class SensorConfig {
public:
    SensorConfig(int channel, int period_ms)
        : channel_{channel},
          period_ms_{period_ms}
    {
        if (channel_ < 0 || channel_ > 15 || period_ms_ <= 0) {
            throw std::invalid_argument{"invalid sensor configuration"};
        }
    }

    int channel() const
    {
        return channel_;
    }

    int period_ms() const
    {
        return period_ms_;
    }

private:
    int channel_;
    int period_ms_;
};
```

**Production and debug angle**

Add tests for boundaries `0`, `15`, invalid `-1`, `16`, and zero/negative
periods. For embedded builds, confirm the exception policy and avoid hidden
allocation in diagnostics if the environment forbids it.

**Common traps**

- Public writable members.
- Constructing first and validating later.
- Leaving a default constructor that creates invalid state.
- Silently clamping malformed input without a product requirement.
- Using `unsigned` to hide negative-input validation.

**Follow-up questions**

- How would you design this without exceptions?
- Should channel and period be separate strong types?
- Is a default constructor meaningful?

### 20. Coding task: implement a C-compatible wrapper around a C++ counter.

**Prompt**

Expose a C API that creates, increments, reads, and destroys a private C++
counter implementation.

**Short answer**

Use an opaque C handle, `extern "C"` declarations, explicit ownership functions,
null checks, and exception containment.

**Deep explanation**

The C header must compile as both C and C++. The implementation can define the
opaque struct around a C++ object. Creation must not leak exceptions or partial
allocation. Destruction should accept the documented null policy.

**C/C++ code or API anchor**

```c
/* counter.h */
#ifndef COUNTER_H
#define COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CounterHandle CounterHandle;

enum CounterStatus {
    COUNTER_OK = 0,
    COUNTER_INVALID_ARGUMENT,
    COUNTER_ALLOCATION_FAILED,
    COUNTER_OVERFLOW
};

int counter_create(int initial, CounterHandle **out_handle);
int counter_increment(CounterHandle *handle);
int counter_value(const CounterHandle *handle, int *out_value);
void counter_destroy(CounterHandle *handle);

#ifdef __cplusplus
}
#endif

#endif
```

Implementation outline:

```cpp
#include "counter.h"

#include <limits>
#include <new>

struct CounterHandle {
    int value;
};

extern "C" int counter_create(
    int initial,
    CounterHandle **out_handle)
{
    if (out_handle == nullptr) {
        return COUNTER_INVALID_ARGUMENT;
    }

    *out_handle = new (std::nothrow) CounterHandle{initial};
    return *out_handle == nullptr
        ? COUNTER_ALLOCATION_FAILED
        : COUNTER_OK;
}

extern "C" int counter_increment(CounterHandle *handle)
{
    if (handle == nullptr) {
        return COUNTER_INVALID_ARGUMENT;
    }
    if (handle->value == std::numeric_limits<int>::max()) {
        return COUNTER_OVERFLOW;
    }

    ++handle->value;
    return COUNTER_OK;
}

extern "C" void counter_destroy(CounterHandle *handle)
{
    delete handle;
}
```

**Production and debug angle**

Compile the header from a C translation unit and link it against the C++
implementation. Add allocation failure, null argument, integer-boundary,
ownership, concurrency, and ABI-version tests.

**Common traps**

- Exposing a C++ class definition in the C header.
- Letting exceptions cross the boundary.
- Failing to initialize `*out_handle` on failure.
- Allocating in one runtime/module and destroying in another incompatible one.
- Omitting thread-safety policy.

**Follow-up questions**

- How would you avoid dynamic allocation?
- How would you add ABI versioning?
- How would you map C++ exceptions to stable C status codes?

## Debugging Scenarios

### 21. Debugging: constructor values are wrong despite a correct-looking initializer list.

**Prompt**

Review this class:

```cpp
class Window {
public:
    Window(int width)
        : area_{width_ * width_},
          width_{width}
    {
    }

private:
    int area_;
    int width_;
};
```

Why is `area_` unreliable, and how should it be fixed?

**Short answer**

Members initialize in declaration order. `area_` is declared before `width_`,
so `area_` reads `width_` before `width_` is initialized.

**Deep explanation**

Initializer-list spelling does not control order. The language initializes data
members in declaration order to keep destruction order predictable.

Fix the dependency by declaring `width_` first or computing from the constructor
parameter directly.

**C/C++ code or API anchor**

```cpp
class Window {
public:
    explicit Window(int width)
        : width_{width},
          area_{width_ * width_}
    {
    }

private:
    int width_;
    int area_;
};
```

**Production and debug angle**

Enable `-Wreorder` and uninitialized-use diagnostics. UBSan does not detect every
uninitialized read; MemorySanitizer or Valgrind may help on supported hosted
builds.

**Common traps**

- Reordering only the initializer list.
- Suppressing the warning.
- Assigning both values in the body.
- Adding a default initializer that hides the dependency.

**Follow-up questions**

- In what order are members destroyed?
- What is base-class initialization order?
- Would `area_{width * width}` also be safe?

### 22. Debugging: a stored callback crashes after the registering function returns.

**Prompt**

```cpp
std::function<int()> make_callback()
{
    int count = 0;
    return [&count]() {
        return ++count;
    };
}
```

Why does this fail?

**Short answer**

The closure stores a reference to `count`, but `count` dies when
`make_callback` returns. Invoking the callback uses a dangling reference.

**Deep explanation**

Reference capture borrows; it does not transfer ownership or extend lifetime.
Returning the closure lets it escape the scope of the referenced object.

Capture the state by value and use `mutable`, or place shared state in a
well-defined owning object when sharing is actually required.

**C/C++ code or API anchor**

```cpp
auto make_callback()
{
    int count = 0;
    return [count]() mutable {
        return ++count;
    };
}
```

**Production and debug angle**

ASan can report stack-use-after-scope on an executed path. Review callbacks for
reference captures, `this` captures, cancellation, and invocation after owner
destruction.

**Common traps**

- Replacing `[&count]` with `[&]`.
- Capturing `this` without solving lifetime.
- Assuming `std::function` owns referenced data.
- Hiding ownership in a global variable.

**Follow-up questions**

- What does `mutable` change?
- When would shared ownership be justified?
- What risks does `[this]` have?

### 23. Debugging: C code cannot link to a C++ function.

**Prompt**

A C++ file defines:

```cpp
int sensor_read(int channel, int *out_value)
{
    *out_value = channel * 10;
    return 0;
}
```

A C caller declares the same spelling but the linker reports an undefined
reference. Diagnose it.

**Short answer**

The declarations may have different language linkage. Put the declaration in a
shared header with `extern "C"` guards and include it in the C++ definition.

**Deep explanation**

The C++ implementation normally gives the function C++ language linkage and an
ABI-specific external symbol. The C translation unit expects its C declaration.
Matching source spelling is insufficient when linkage or calling convention
differs.

The shared header is the contract; do not duplicate declarations manually.

**C/C++ code or API anchor**

```c
#ifdef __cplusplus
extern "C" {
#endif

int sensor_read(int channel, int *out_value);

#ifdef __cplusplus
}
#endif
```

```cpp
#include "sensor_api.h"

int sensor_read(int channel, int *out_value)
{
    if (out_value == nullptr) {
        return 1;
    }
    *out_value = channel * 10;
    return 0;
}
```

**Production and debug angle**

Use `nm` and `c++filt` or platform equivalents to inspect symbols. Also verify
the object file is linked, architecture matches, visibility permits export, and
signatures are identical.

**Common traps**

- Adding `extern "C"` only at one duplicated declaration.
- Assuming every undefined symbol is caused by mangling.
- Exposing exceptions or C++ types after fixing the symbol.
- Compiling the `.c` file as C++ accidentally.

**Follow-up questions**

- Does `extern "C"` guarantee ABI compatibility for structs?
- Can C-linkage functions be overloaded?
- How would you hide a C++ class behind this API?

## Rapid-Fire Review

1. Give a precise C vs C++ comparison for initialization and cleanup.
2. Can a reference be reseated?
3. Does `const T&` imply ownership?
4. What is an lvalue reference?
5. What is the default access of a C++ `struct`?
6. What does each access specifier communicate?
7. Compare a default constructor and a parameterized constructor.
8. What is a default member initializer?
9. What is a delegating constructor?
10. What is a copy constructor?
11. What runs before a constructor body?
12. What is the actual member initialization order?
13. How does aggregate initialization differ from “any `struct` with braces”?
14. Compare a constructor with a C init function.
15. Compare a destructor with manual cleanup in C.
16. Compare function overloading with a C naming convention.
17. Can return type alone overload a function?
18. Does `inline` force call-site substitution?
19. Does `extern "C"` make `std::vector` C-compatible?
20. Can a friend function be a non-member?
21. What access does a friend class receive?
22. Is friendship inherited or transitive?
23. Can a lambda reference capture dangle?
24. Does `constexpr` guarantee every call runs at compile time?
25. Does list initialization reject every possible lossy conversion?
26. Is tail-call optimization guaranteed?
27. Can copy elision change observed constructor calls?
28. What role can ADL play in finding a non-member operator?

## Final Interview Checklist

A strong Chapter 08 candidate can:

- compare C and C++ without slogans;
- explain object lifetime separately from storage and scope;
- select value, reference, const reference, or pointer from an API contract;
- explain class invariants and access control;
- reason about constructor and destructor order;
- use `explicit`, default member initializers, and delegation deliberately;
- explain overload resolution before discussing mangled symbols;
- distinguish overloading from overriding;
- design unsurprising operator overloads;
- explain static members and narrow friendship;
- diagnose lambda capture lifetime bugs;
- explain `inline` as an ODR mechanism;
- design and debug a narrow C/C++ ABI boundary;
- avoid relying on copy counts or optional optimization;
- use compiler diagnostics and sanitizers as evidence, not proof.

## Reference Notes

- Exact reference binding, constructor/destructor, overload, language-linkage,
  copy-elision, and ODR rules depend on the applicable C++ standard edition.
- ABI symbol forms, warnings, sanitizers, debugger behavior, and inspection tools
  are implementation- and platform-specific.
