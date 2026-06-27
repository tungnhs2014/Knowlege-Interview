# 04 - Pointer Mastery

## 1. Goal

By the end of this chapter, you should be able to:

- explain the difference between a pointer object, its pointer value, and its
  pointee;
- use address-of, dereference, and member access safely;
- distinguish null, uninitialized, dangling, invalid, one-past, owning, and
  borrowed pointers;
- explain why an array is not a pointer and when array-to-pointer conversion
  occurs;
- apply pointer arithmetic only within its valid array domain;
- parse pointer-to-array, array-of-pointers, pointer-to-pointer, and function
  pointer declarations;
- use all three important const-pointer forms;
- design C pointer APIs with explicit nullability, bounds, ownership, and output
  contracts;
- compare pointers with references, arrays, bounded views, and smart pointers;
- recognize null dereference, use-after-free, buffer overflow, invalid release,
  aliasing, alignment, and callback-lifetime failures;
- debug pointer failures with warnings, sanitizers, Valgrind, and GDB;
- answer pointer questions from junior through senior interview level.

This is a `MUST` topic at `Deep` depth. Chapter 03, C Memory Model, is the
prerequisite.

## 2. Why Pointer Mastery Matters

Pointers connect source-level expressions to object identity, lifetime, and
memory access. They appear in:

- arrays, strings, buffers, and parsers;
- dynamic allocation;
- linked data structures;
- C library interfaces;
- callbacks and function tables;
- opaque handles;
- C++ polymorphism;
- embedded hardware abstraction;
- standard library implementations.

Pointer syntax is not the hardest part. The difficult questions are:

- Does the target object still exist?
- Is the pointer inside the correct array?
- Is the address suitably aligned?
- Is access allowed through this type?
- Is the pointer optional?
- Who owns the target?
- Who releases it, using which API?
- Can another alias outlive or invalidate it?
- Does a callback retain the pointer after the call?

A pointer may be non-null and still be unusable. A program can print the
expected result while already executing undefined behavior.

The central rule is:

> A pointer is safe to dereference only when its value designates a live object
> of an appropriate type, within bounds, with suitable alignment and permitted
> access.

## 3. Mental Model: Pointer Object, Value, And Pointee

Consider:

```c
int temperature = 25;
int *sensor = &temperature;
```

There are two objects:

1. `temperature`, an `int` object containing `25`;
2. `sensor`, a pointer object containing a pointer value that designates
   `temperature`.

```text
sensor                              temperature
+------------------+               +-----------+
| pointer value --------points---->|    25     |
+------------------+               +-----------+
```

`sensor` has its own address:

```text
&sensor       address of the pointer object
sensor        pointer value stored in sensor
*sensor       int object designated by that value
&temperature  address of temperature
```

For a valid pointer in this example:

```c
sensor == &temperature
*sensor == temperature
```

Copying the pointer creates another alias:

```c
int *alias = sensor;
```

```text
sensor ----+
           +----> temperature
alias  ----+
```

The `int` was not copied. Both pointer objects designate the same target.
Writing through either alias changes `temperature`.

```c
*alias = 30;
```

### 3.1 Validity is more than a numeric address

Do not reduce a pointer to "just an integer address." For an access to be valid,
the program must satisfy several conditions:

| Condition | Question |
| --- | --- |
| Lifetime | Does the target object still exist? |
| Bounds | Is the pointer within the correct array object? |
| Type | Is access permitted through this pointer type? |
| Alignment | Is the address correctly aligned for the target type? |
| Mutability | Does qualification permit this write? |
| Ownership | Is this code allowed to release the target? |

These conditions explain why null checks alone are not enough.

### 3.2 A pointer's state

Useful pointer states include:

| State | Meaning | Dereference? |
| --- | --- | --- |
| Valid object pointer | Designates a live object | Yes, if type and access are valid |
| Valid array-element pointer | Designates a live array element | Yes |
| One-past pointer | Designates the boundary after an array | No |
| Null pointer | Designates no object or function | No |
| Uninitialized pointer | Holds an indeterminate value | Do not read or dereference |
| Dangling pointer | Target lifetime has ended | No |
| Invalid pointer | Does not satisfy the operation's requirements | No |

The term `wild pointer` commonly means an uninitialized pointer. It is useful
vocabulary in reviews and interviews, but `uninitialized pointer` is more
precise.

## 4. Core Pointer Mechanism

### 4.1 Declaration and initialization

```c
int value = 42;
int *ptr = &value;
```

Read `int *ptr` as "`ptr` is a pointer to `int`."

Prefer one declaration per line:

```c
int *first;
int *second;
```

This avoids the common trap:

```c
int *first, second;
```

Only `first` is a pointer. `second` is an `int`.

Initialize every pointer immediately:

```c
int *target = NULL;       /* C */
int *other = &value;
```

```cpp
int *target = nullptr;    // C++11 and later
int *other = &value;
```

### 4.2 Address-of operator

The unary `&` operator produces a pointer to an object:

```c
#include <stdio.h>

int main(void)
{
    int value = 42;
    int *ptr = &value;

    printf("%d\n", *ptr);
    return 0;
}
```

For portable output with `printf`, convert an object pointer to `void *` for
the `%p` conversion:

```c
printf("%p\n", (void *)&value);
```

### 4.3 Dereference operator

The unary `*` operator accesses the object designated by a valid pointer:

```c
int value = 42;
int *ptr = &value;

int copy = *ptr;  /* read */
*ptr = 50;        /* write */
```

After the write, `value` is `50`.

The same `*` token has different roles:

```c
int *ptr;   /* declaration: ptr has pointer type */
*ptr = 1;   /* expression: dereference ptr */
```

### 4.4 Structure member access

Given a pointer to a structure:

```c
struct Sensor {
    int id;
    int reading;
};

struct Sensor sensor = {7, 25};
struct Sensor *ptr = &sensor;
```

These expressions are equivalent:

```c
(*ptr).reading
ptr->reading
```

Parentheses are required in `(*ptr).reading` because member access binds more
tightly than unary dereference.

## 5. Null, Uninitialized, Dangling, And Invalid Pointers

### 5.1 Null pointers

A null pointer explicitly designates no object or function.

```c
int *ptr = NULL;

if (ptr != NULL) {
    printf("%d\n", *ptr);
}
```

```cpp
int *ptr = nullptr;

if (ptr != nullptr) {
    std::cout << *ptr << '\n';
}
```

Dereferencing a null pointer is undefined behavior. It may crash, corrupt
state, or behave differently after optimization.

Use `nullptr` in modern C++ because it has a dedicated pointer-related type and
does not behave like an ordinary integer in overload resolution:

```cpp
#include <iostream>

void select(int)
{
    std::cout << "integer\n";
}

void select(int *)
{
    std::cout << "pointer\n";
}

int main()
{
    select(0);        // integer overload
    select(nullptr);  // pointer overload
}
```

In C, use the project's accepted null pointer convention, commonly `NULL`.
C23 also adds `nullptr`, but codebase language-version requirements matter.

### 5.2 Uninitialized pointers

```c
int *ptr;       /* indeterminate value */
/* *ptr = 10;  undefined behavior */
```

Do not print, compare, dereference, or release an uninitialized automatic
pointer as though it held a valid pointer value.

Initialize it:

```c
int *ptr = NULL;
```

Compiler warnings often detect simple cases, but path-dependent
uninitialized use may require static analysis or MemorySanitizer.

### 5.3 Dangling pointers

A dangling pointer still exists, but the target object's lifetime has ended.

```c
int *bad_result(void)
{
    int local = 42;
    return &local; /* wrong: local dies when the function returns */
}
```

Dynamic allocation can produce the same failure:

```c
int *ptr = malloc(sizeof *ptr);

if (ptr != NULL) {
    *ptr = 42;
    free(ptr);
    /* ptr is now dangling */
}
```

Assigning `NULL` after `free` can prevent reuse through that pointer object:

```c
free(ptr);
ptr = NULL;
```

It does not repair aliases:

```c
int *owner = malloc(sizeof *owner);
int *alias = owner;

free(owner);
owner = NULL;

/* alias is still dangling */
```

Ownership and alias lifetime must be correct before deallocation.

### 5.4 References can dangle too

C++ references are not nullable through ordinary well-formed source syntax,
but they do not guarantee lifetime:

```cpp
int &bad_reference()
{
    int local = 42;
    return local; // dangling reference
}
```

The compiler may warn, but using the returned reference is undefined behavior.

## 6. Pointers And Arrays

### 6.1 An array is not a pointer

```c
int values[4] = {10, 20, 30, 40};
int *ptr = values;
```

`values` is an array object containing four `int` elements. In the initializer
for `ptr`, the array expression converts to a pointer to its first element.

```text
values
+------+------+------+------+
|  10  |  20  |  30  |  40  |
+------+------+------+------+
   ^
   |
  ptr
```

The expressions below designate the first element:

```c
values
&values[0]
```

The expression `&values` has a different type: pointer to the entire array.

```c
int *element_ptr = values;       /* int * */
int (*array_ptr)[4] = &values;   /* pointer to array of 4 int */
```

Their represented starting location is commonly the same, but their types and
arithmetic are different:

```c
element_ptr + 1  /* advances by one int */
array_ptr + 1    /* advances by one complete int[4] array */
```

### 6.2 Important non-decay contexts

Array-to-pointer conversion does not occur in every context.

```c
int values[4];

sizeof values   /* size of the complete array */
&values         /* pointer to the complete array */
```

In C++, a reference can preserve array type and extent:

```cpp
#include <cstddef>

template <std::size_t N>
void clear(int (&values)[N])
{
    for (int &value : values) {
        value = 0;
    }
}
```

### 6.3 Array parameters are adjusted

These C declarations describe the same parameter type:

```c
void process(int values[]);
void process(int values[10]);
void process(int *values);
```

Inside the function, `values` is a pointer parameter. The written `10` does not
make the function know the caller's array extent.

Pass the bound:

```c
#include <stddef.h>
#include <stdio.h>

void print_values(const int *values, size_t count)
{
    if (values == NULL && count != 0U) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        printf("%d\n", values[i]);
    }
}

int main(void)
{
    const int values[] = {10, 20, 30};
    print_values(values, sizeof values / sizeof values[0]);
    return 0;
}
```

The contract permits a null pointer only when `count` is zero. An API may
instead reject null unconditionally; document the chosen rule.

## 7. Pointer Arithmetic And One-Past Pointers

### 7.1 Arithmetic counts elements

For a pointer to an array element:

```c
int values[4] = {10, 20, 30, 40};
int *ptr = values;

++ptr; /* now designates values[1] */
```

`ptr + n` advances by `n` elements, not `n` bytes.

```c
values[i] == *(values + i)
```

### 7.2 The valid domain

Pointer arithmetic must remain within one array object or its one-past
boundary:

```text
valid positions for int values[4]:

&values[0]  &values[1]  &values[2]  &values[3]  values + 4
    valid        valid       valid       valid      one-past
```

The one-past pointer is useful as a sentinel:

```c
const int *current = values;
const int *end = values + 4;

while (current != end) {
    printf("%d\n", *current);
    ++current;
}
```

Do not dereference `end`.

Forming or using a pointer outside the permitted domain can be undefined
behavior even if the program does not immediately dereference it.

### 7.3 Pointer subtraction

Subtract pointers only when both belong to the same array object or its
one-past boundary:

```c
#include <stddef.h>

int values[8];
int *first = &values[2];
int *last = &values[7];
ptrdiff_t distance = last - first; /* 5 elements */
```

Do not subtract pointers into unrelated objects or allocations.

### 7.4 Pointer comparison

Equality comparison is useful for null checks and identity checks.

Ordered comparison such as `<` is meaningful for positions within the same
array domain. Do not use it as a portable way to order arbitrary allocation
addresses.

Bad design:

```c
if (allocation_a < allocation_b) {
    /* assumes unrelated addresses have a useful language-level order */
}
```

Use an index, key, sequence number, or documented container position instead.

## 8. Const And Pointers

The location of `const` changes what is protected.

### 8.1 Pointer to const

```c
const int *ptr;
```

`ptr` may designate different `int` objects, but the object cannot be modified
through `ptr`.

```c
int first = 10;
int second = 20;
const int *ptr = &first;

ptr = &second; /* allowed */
/* *ptr = 30;  not allowed */
```

### 8.2 Const pointer

```c
int * const ptr = &value;
```

The stored pointer value cannot be changed, but the target may be modified:

```c
int value = 10;
int other = 20;
int * const ptr = &value;

*ptr = 30;     /* allowed */
/* ptr = &other; not allowed */
```

### 8.3 Const pointer to const

```c
const int * const ptr = &value;
```

Neither reassignment nor modification through `ptr` is allowed.

| Declaration | Modify through pointer? | Reassign pointer? |
| --- | --- | --- |
| `const int *ptr` | No | Yes |
| `int * const ptr` | Yes | No |
| `const int * const ptr` | No | No |

Read declarations from the variable name outward:

```text
const int *ptr
          ^ ptr is a pointer
             to a const int
```

### 8.4 Top-level const on parameters

A pointer parameter is passed by value. This function cannot reassign its local
copy of the pointer:

```c
void reset_local(int * const ptr)
{
    *ptr = 0;
}
```

The top-level `const` is an implementation detail of the function body. It does
not prevent the caller from changing its own pointer object.

Pointee const is part of the useful interface contract:

```c
int sum_values(const int *values, size_t count);
```

It tells callers that the function does not modify elements through `values`.

### 8.5 Casting away const

Removing qualification does not make an originally const object writable:

```c
const int value = 42;
int *ptr = (int *)&value;
/* *ptr = 10; undefined behavior */
```

If the original object is non-const, a temporary const-qualified access path
may be converted back carefully. In normal API design, avoid casts that erase
const because they hide contract mistakes.

## 9. Pointer To Pointer

### 9.1 Two levels of indirection

```c
int value = 42;
int *ptr = &value;
int **ptr_to_ptr = &ptr;
```

```text
ptr_to_ptr --------> ptr --------> value
                       *ptr          42

*ptr_to_ptr == ptr
**ptr_to_ptr == value
```

### 9.2 Replacing a caller's pointer

C passes every argument by value, including pointer values:

```c
void wrong_reset(int *ptr)
{
    ptr = NULL; /* changes only the local copy */
}
```

To change the caller's pointer object, pass its address:

```c
void reset_pointer(int **out)
{
    if (out != NULL) {
        *out = NULL;
    }
}
```

Allocation output parameters are a common use:

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int create_values(size_t count, int **out_values)
{
    if (out_values == NULL) {
        return -1;
    }

    *out_values = NULL;

    if (count == 0U) {
        return 0;
    }

    if (count > SIZE_MAX / sizeof **out_values) {
        return -1;
    }

    int *values = malloc(count * sizeof *values);
    if (values == NULL) {
        return -1;
    }

    *out_values = values;
    return 0;
}
```

The API contract should state:

- whether `out_values` may be null;
- the value of `*out_values` on failure;
- who owns the allocation on success;
- which function releases it.

### 9.3 `T **` is not a two-dimensional array type

These types describe different layouts:

```c
int **rows;         /* pointer to pointer */
int (*matrix)[4];   /* pointer to array of 4 int */
int *flat;          /* pointer into a flat sequence */
```

An array of separately allocated rows may use `int **`, but a contiguous
`int[3][4]` decays to `int (*)[4]`, not `int **`.

Passing one as the other through a cast does not make the layouts compatible.

## 10. Pointer To Array Versus Array Of Pointers

Parentheses decide the declaration.

### 10.1 Pointer to array

```c
int values[4] = {1, 2, 3, 4};
int (*ptr)[4] = &values;

printf("%d\n", (*ptr)[2]);
```

`ptr` is one pointer. It designates a complete array of four `int`.

This is useful for multidimensional arrays:

```c
#include <stddef.h>
#include <stdio.h>

void print_matrix(size_t rows, const int matrix[][3])
{
    for (size_t row = 0; row < rows; ++row) {
        for (size_t column = 0; column < 3U; ++column) {
            printf("%d ", matrix[row][column]);
        }
        putchar('\n');
    }
}

int main(void)
{
    const int matrix[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    print_matrix(2U, matrix);
    return 0;
}
```

The adjusted parameter type is pointer to array of three `const int`.

### 10.2 Array of pointers

```c
int first = 10;
int second = 20;
int third = 30;

int *items[3] = {&first, &second, &third};
```

`items` is an array containing three pointer objects. Each pointer may designate
a different object.

| Type | Meaning | Typical use |
| --- | --- | --- |
| `int (*)[4]` | Pointer to an array of four `int` | Contiguous rows |
| `int *[4]` | Array of four pointers to `int` | Independent objects or ragged rows |
| `int **` | Pointer to a pointer to `int` | Pointer replacement or pointer sequence |

## 11. `void *`, Alignment, And Typed Access

### 11.1 Generic object pointers in C

`void *` can carry a pointer to an object without retaining its pointed-to
type:

```c
int value = 42;
void *storage = &value;
int *typed = storage; /* implicit in C */

printf("%d\n", *typed);
```

You cannot directly dereference `void *` because `void` has no object size or
value representation to access:

```c
/* *storage; invalid */
```

In C++, conversion back to an object pointer is explicit:

```cpp
int value = 42;
void *storage = &value;
int *typed = static_cast<int *>(storage);
```

### 11.2 Type erasure does not erase the contract

Before dereferencing a converted pointer, the program still needs:

- a live object of an appropriate type;
- sufficient storage;
- correct alignment;
- valid bounds;
- correct mutability;
- an ownership and lifetime rule.

This is unsafe:

```c
unsigned char bytes[sizeof(int)];
int *value = (int *)bytes;
/* *value = 42; alignment and object-access rules may be violated */
```

For byte transfer, prefer `memcpy` between properly declared objects:

```c
#include <stdint.h>
#include <string.h>

uint32_t source = UINT32_C(0x12345678);
unsigned char bytes[sizeof source];

memcpy(bytes, &source, sizeof source);
```

Byte representation is separate from portable serialization. External formats
must define byte order and field widths.

### 11.3 Object and function pointers are different categories

Portable C and C++ code must not assume that object pointers and function
pointers have interchangeable representations.

Use a correctly typed function pointer for callbacks.

## 12. Function Pointers And Callbacks

### 12.1 Basic function pointer

```c
#include <stdio.h>

int add(int left, int right)
{
    return left + right;
}

int main(void)
{
    int (*operation)(int, int) = add;
    printf("%d\n", operation(2, 3));
    return 0;
}
```

Read:

```c
int (*operation)(int, int);
```

as "`operation` is a pointer to a function taking two `int` arguments and
returning `int`."

Parentheses are required. Without them:

```c
int *operation(int, int);
```

declares a function returning `int *`.

### 12.2 Use a type alias

```c
typedef int (*binary_operation)(int, int);

int apply(binary_operation operation, int left, int right)
{
    if (operation == NULL) {
        return 0;
    }

    return operation(left, right);
}
```

The callback's parameter and return types must match. Calling through an
incompatible function pointer type is undefined behavior.

### 12.3 Callback with context

A context pointer lets a C callback use caller-specific state without global
variables:

```c
#include <stddef.h>
#include <stdio.h>

typedef void (*value_callback)(int value, void *context);

void for_each_value(const int *values,
                    size_t count,
                    value_callback callback,
                    void *context)
{
    if (callback == NULL || (values == NULL && count != 0U)) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        callback(values[i], context);
    }
}

struct Sum {
    int total;
};

void add_to_sum(int value, void *context)
{
    struct Sum *sum = context;
    if (sum != NULL) {
        sum->total += value;
    }
}

int main(void)
{
    const int values[] = {2, 4, 6};
    struct Sum sum = {0};

    for_each_value(values, 3U, add_to_sum, &sum);
    printf("%d\n", sum.total);
    return 0;
}
```

For a synchronous callback, `sum` remains alive for the whole call.

For a stored or asynchronous callback, the contract must answer:

- How long is `context` retained?
- Who owns it?
- How is the callback unregistered?
- Can invocation race with destruction?
- Is the callback reentrant?
- On which execution context is it invoked?

A callback function may be valid while its context pointer is dangling.

### 12.4 C++ callable comparison

A noncapturing lambda can convert to a compatible function pointer:

```cpp
int (*square)(int) = [](int value) {
    return value * value;
};
```

A capturing lambda carries state and cannot generally convert to a plain
function pointer:

```cpp
int offset = 5;
auto add_offset = [offset](int value) {
    return value + offset;
};
```

Templates and `std::function` support richer callables, but have different
type, lifetime, allocation, and performance tradeoffs. They belong to the
Modern C++ chapter.

## 13. Passing Data Through Pointers

### 13.1 The pointer itself is copied

```c
void set_value(int *ptr)
{
    if (ptr != NULL) {
        *ptr = 42;
    }
}
```

The function receives a copy of the caller's pointer value. It can modify the
pointee, but assigning the local pointer does not change the caller's pointer.

### 13.2 Required versus optional parameters

In C, pointer parameters are common for both required and optional inputs.
Document the distinction:

```c
/* required: ptr must designate an int */
void required_value(int *ptr);

/* optional: NULL means no value */
void optional_value(const int *ptr);
```

In C++, references are often clearer for required parameters:

```cpp
void required_value(int &value);
void optional_value(const int *value);
```

This convention is useful, not magical. A reference may still dangle if its
target's lifetime is wrong.

### 13.3 Output parameters

Use output parameters when required by a C API or when return status and result
must be separate:

```c
int divide(int dividend, int divisor, int *out_result)
{
    if (out_result == NULL || divisor == 0) {
        return -1;
    }

    *out_result = dividend / divisor;
    return 0;
}
```

Define whether the output is unchanged, cleared, or unspecified on failure.

In C++, a result object, `std::optional`, or expected-style type may express the
contract more clearly. Those alternatives are covered later.

### 13.4 Bounded buffer interfaces

Avoid pointer-only interfaces:

```c
void process(unsigned char *data); /* where is the bound? */
```

Prefer:

```c
void process(unsigned char *data, size_t size);
```

For output buffers, distinguish capacity from produced length:

```c
int encode(unsigned char *output,
           size_t capacity,
           size_t *out_size);
```

The function contract must describe:

- whether `output` may be null when capacity is zero;
- maximum bytes written;
- output state on failure;
- overlap restrictions;
- whether a terminator is included for text.

## 14. Ownership And Borrowing

### 14.1 Raw pointer syntax does not express ownership

These declarations look similar:

```c
struct Buffer *create_buffer(size_t size);
void inspect_buffer(const struct Buffer *buffer);
void destroy_buffer(struct Buffer *buffer);
```

Their roles differ:

| Pointer | Role |
| --- | --- |
| Return from `create_buffer` | Owning pointer |
| Parameter to `inspect_buffer` | Borrowed pointer |
| Parameter to `destroy_buffer` | Ownership-ending operation |

The API must state:

- who allocates;
- who owns;
- whether ownership transfers;
- how long borrows remain valid;
- which function releases the resource.

### 14.2 Single owner, multiple observers

```text
owner --------------------> object
observer_a ------------------^
observer_b ------------------^
```

The owner controls lifetime. Observers must not release the object and must not
outlive the owner's resource.

When the owner releases the object:

```text
owner -> null
observer_a -> dangling
observer_b -> dangling
```

Resetting only the owner cannot update every observer.

### 14.3 Invalidation without explicit deallocation

Pointers can become invalid when:

- an automatic object leaves scope;
- a C++ container reallocates;
- `realloc` successfully replaces an allocation;
- an object is destroyed;
- a callback registration is removed and its context destroyed;
- a temporary's lifetime ends.

Review lifetime, not only `free` and `delete`.

### 14.4 Opaque handles in C

A C header can hide implementation details:

```c
struct Sensor;

struct Sensor *sensor_create(int id);
int sensor_read(const struct Sensor *sensor, int *out_value);
void sensor_destroy(struct Sensor *sensor);
```

This design can provide:

- encapsulation;
- one allocation family;
- a clear owner;
- controlled access;
- implementation changes without exposing structure layout.

The caller must still follow the lifetime contract.

## 15. Dynamic Allocation Boundaries

### 15.1 C allocation

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int *allocate_values(size_t count)
{
    if (count == 0U || count > SIZE_MAX / sizeof(int)) {
        return NULL;
    }

    int *values = malloc(count * sizeof *values);
    return values;
}
```

In C, do not cast the result of `malloc`. The implicit conversion from `void *`
is defined, and an unnecessary cast can hide a missing declaration in older
code.

Pair:

```text
malloc/calloc/realloc -> free
```

### 15.2 C++ allocation

Raw C++ allocation syntax is:

```cpp
int *single = new int{42};
int *array = new int[10]{};

delete single;
delete[] array;
```

Pair exactly:

```text
new   -> delete
new[] -> delete[]
```

Mismatching an allocation and release family is undefined behavior.

Production C++ should usually prefer:

```cpp
auto single = std::make_unique<int>(42);
std::vector<int> values(10);
```

RAII and smart pointers are covered deeply in Chapter 10. Here, remember:

- raw pointer does not itself state ownership;
- `std::unique_ptr` normally represents exclusive ownership;
- `std::shared_ptr` represents shared ownership when that design is genuinely
  required;
- raw pointers and references remain useful as non-owning access paths.

### 15.3 `realloc` invalidates old aliases on success

```c
int *values = malloc(4U * sizeof *values);
int *alias = values;

int *resized = realloc(values, 8U * sizeof *values);
if (resized != NULL) {
    values = resized;
    /* alias must not be used; the old allocation may have moved */
}
```

Use a temporary result so failure does not lose the original allocation.
Design code so interior pointers and aliases do not survive resizing.

## 16. C And C++ Comparison

| Topic | C | C++ | Practical guidance |
| --- | --- | --- | --- |
| Required alias parameter | Pointer plus documented non-null precondition | Reference often expresses required access | Lifetime must still be valid |
| Optional parameter | Nullable pointer | Nullable pointer, optional wrapper where appropriate | Make absence explicit |
| Null pointer | Commonly `NULL` | Prefer `nullptr` | Follow language version |
| Generic object pointer | `void *` converts to/from object pointers | Explicit conversion back to typed pointer | Prefer typed interfaces |
| Buffer view | Pointer plus count/end | `std::span`, iterator pair, container reference | Carry the bound |
| Dynamic array | Allocation plus explicit size and cleanup | Prefer `std::vector` | Avoid owning raw arrays |
| Exclusive ownership | Documented owning pointer | Prefer `std::unique_ptr` | Ownership should be visible |
| Callback | Function pointer plus context | Function pointer, lambda, template callable, `std::function` | Match lifetime and cost needs |
| Pointer replacement | `T **out` | Return value, reference to pointer, or ownership type | Prefer the clearest contract |

### 16.1 Pointer versus reference

| Property | Pointer | C++ reference |
| --- | --- | --- |
| Ordinary null state | Yes | No |
| Reseatable | Yes | No |
| Explicit dereference | Yes | No |
| Arithmetic | For valid array domains | No |
| Can dangle | Yes | Yes |
| Common meaning | Optional access, range position, C interface | Required alias |

Use a reference when C++ code requires an existing object and reseating is not
needed. Use a pointer when absence, reseating, array traversal, C compatibility,
or pointer identity is part of the design.

### 16.2 Pointer versus array

| Property | Pointer | Array |
| --- | --- | --- |
| Stores | Pointer value | Elements |
| Reseatable | Yes, unless const pointer | No |
| `sizeof` | Size of pointer object | Size of complete array in non-decay context |
| Owns elements | Not implied | The array object contains its elements |
| Extent available from type | Usually no | Yes before decay |

### 16.3 Raw pointer versus smart pointer

| Property | Raw pointer | Smart pointer |
| --- | --- | --- |
| Ownership encoded | No | Yes |
| Automatic release | No | Yes, by RAII |
| Nullable | Yes | Usually yes |
| Suitable observer | Yes | Usually use raw pointer/reference |
| Pointer arithmetic | Possible for arrays | Generally not an ownership design tool |

Smart pointers do not make all lifetime bugs impossible. Observers can still
dangle, `shared_ptr` cycles can leak, and an incorrect ownership model remains
incorrect.

## 17. Practical Usage

### 17.1 Protocol parser

A parser should receive both pointer and extent:

```c
#include <stddef.h>
#include <stdint.h>

int read_u16_be(const unsigned char *data,
                size_t size,
                uint16_t *out_value)
{
    if (data == NULL || out_value == NULL || size < 2U) {
        return -1;
    }

    *out_value = (uint16_t)((uint16_t)data[0] << 8)
               | (uint16_t)data[1];
    return 0;
}
```

This avoids:

- unaligned typed access;
- native-endian assumptions;
- out-of-bounds reads;
- structure-overlay assumptions.

### 17.2 Ring buffer

A ring buffer commonly owns an array while read/write pointers or indices
describe current positions.

Prefer indices when wraparound and capacity arithmetic are clearer:

```c
struct RingBuffer {
    unsigned char *data;
    size_t capacity;
    size_t read_index;
    size_t write_index;
};
```

Pointer arithmetic must not wander outside the allocated array and then wrap
back. Calculate a valid index first.

### 17.3 Embedded callback table

```c
typedef void (*state_handler)(void *context);

struct StateEntry {
    state_handler handler;
    void *context;
};
```

Review:

- exact callback type;
- null handling;
- context alignment and type;
- context lifetime;
- whether invocation can occur after shutdown;
- whether callbacks run from a restricted execution context.

This remains a language and API-design example. Hardware-driver details are
outside this chapter.

### 17.4 C++ bounded view

```cpp
#include <cstddef>
#include <span>

int sum(std::span<const int> values)
{
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}
```

`std::span` is non-owning. It carries a pointer and extent, but the underlying
elements must outlive the span and all uses of it.

## 18. Common Pointer Bugs

### 18.1 Null dereference

```c
int *ptr = NULL;
*ptr = 42; /* undefined behavior */
```

Prevention:

- validate optional pointers;
- use required references in suitable C++ interfaces;
- establish invariants at API boundaries.

### 18.2 Out-of-bounds access

```c
int values[4] = {0};
values[4] = 1; /* undefined behavior */
```

Index `4` is the one-past position and cannot be accessed.

Prevention:

- carry bounds;
- validate before pointer formation and dereference;
- test zero, one, exact-capacity, and over-capacity cases.

### 18.3 Buffer overflow

```c
void fill(int *values, size_t count)
{
    for (size_t i = 0; i <= count; ++i) { /* wrong: <= */
        values[i] = 0;
    }
}
```

An off-by-one error can overwrite another object or allocator metadata.

### 18.4 Use-after-free

```c
int *ptr = malloc(sizeof *ptr);
if (ptr != NULL) {
    free(ptr);
    *ptr = 42; /* undefined behavior */
}
```

The write may appear to work until the allocator reuses the block.

### 18.5 Double or invalid release

```c
int *ptr = malloc(sizeof *ptr);
free(ptr);
free(ptr); /* undefined behavior */
```

Also invalid:

- freeing an automatic or static object;
- freeing an interior pointer;
- using `free` for `new`;
- using `delete` for `malloc`;
- using `delete` for `new[]`.

### 18.6 Lost allocation

```c
int *ptr = malloc(10U * sizeof *ptr);
ptr = malloc(20U * sizeof *ptr); /* first allocation is lost */
```

The program overwrote its only owner before releasing the first allocation.

### 18.7 Stale alias

```c
int *owner = malloc(sizeof *owner);
int *observer = owner;

free(owner);
owner = NULL;

/* observer is non-null but dangling */
```

### 18.8 String literal modification

This is wrong in C:

```c
char *text = "ready";
text[0] = 'R'; /* undefined behavior */
```

Use:

```c
const char *read_only_text = "ready";
char mutable_text[] = "ready";
```

C++ string literals have const array element type, so assigning one to
`char *` is rejected by conforming modern C++ compilers.

### 18.9 Wrong callback type

Do not cast a function merely to satisfy a callback parameter:

```c
/* Casting an incompatible function pointer does not make the call valid. */
```

Write an adapter with the exact required signature.

### 18.10 Misaligned or incompatible typed access

```c
unsigned char bytes[8];
int *ptr = (int *)(bytes + 1);
/* *ptr may violate alignment and object-access rules */
```

Use a declared object, an aligned storage strategy with correct lifetime rules,
or `memcpy` for byte transfer.

### 18.11 Pointer arithmetic across objects

Two local variables written next to each other are not an array:

```c
int first = 1;
int second = 2;
int *ptr = &first;

/* ++ptr does not portably make ptr point to second. */
```

Source declaration order and nearby addresses do not create an array
relationship.

## 19. Debugging Workflow

### 19.1 Start with warnings

For C:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  -Wcast-align -Wcast-qual -g pointer_case.c -o pointer_case
```

For C++:

```bash
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  -Wcast-align -Wcast-qual -g pointer_case.cpp -o pointer_case
```

Warning support differs by compiler. Enable the strongest practical profile
for the project and investigate warnings rather than suppressing them blindly.

### 19.2 AddressSanitizer

```bash
cc -std=c17 -fsanitize=address -fno-omit-frame-pointer -g \
  pointer_case.c -o pointer_case
./pointer_case
```

AddressSanitizer is effective for:

- out-of-bounds access;
- use-after-free;
- many invalid free cases;
- stack-use-after-scope on supported configurations.

Read the first reported invalid access and its allocation/free stack traces.
Later failures may be consequences.

### 19.3 UndefinedBehaviorSanitizer

```bash
cc -std=c17 -fsanitize=undefined -fno-omit-frame-pointer -g \
  pointer_case.c -o pointer_case
./pointer_case
```

UBSan can detect some alignment and invalid-operation failures. No sanitizer
proves that every pointer operation is valid.

### 19.4 Uninitialized reads

AddressSanitizer is not a complete uninitialized-memory detector. In a
supported Clang environment, MemorySanitizer can help:

```bash
clang -std=c17 -fsanitize=memory -fno-omit-frame-pointer -g \
  pointer_case.c -o pointer_case
```

It requires compatible instrumented dependencies to provide reliable results.

### 19.5 Valgrind

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./pointer_case
```

Valgrind Memcheck can report:

- invalid reads and writes;
- use of uninitialized values;
- invalid releases;
- leaks and their allocation sites.

It has higher runtime overhead than compiler sanitizers.

### 19.6 GDB

Useful commands:

```text
break function_name
run
bt
frame 1
info locals
p ptr
p *ptr
ptype ptr
x/16xb ptr
x/8dw ptr
watch ptr
watch *ptr
```

Ask separately:

1. What is the pointer value?
2. Where was it assigned?
3. What object should it designate?
4. Is that object still alive?
5. What is the valid extent?
6. Who released or invalidated it?

Do not dereference a suspicious pointer in GDB until you understand whether
the address is readable.

### 19.7 Optimization-sensitive failures

Undefined behavior can change when optimization changes:

```bash
cc -O0 -g pointer_case.c -o case_o0
cc -O2 -g pointer_case.c -o case_o2
```

A bug that disappears at `-O0` is not fixed. Compare sanitizer reports and
generated behavior while searching for the first invalid operation.

### 19.8 Ownership trace

For a difficult pointer, write down:

```text
allocation/creation site:
owner:
borrowers:
valid extent:
expected type/alignment:
invalidation events:
release site and API:
```

This small ledger often exposes unclear lifetime faster than staring at the
crash line.

## 20. Best Practices

- Initialize every pointer immediately.
- Use `nullptr` in modern C++.
- Treat nullability as an API decision, not an accidental possibility.
- Carry a count, end pointer, sentinel, or bounded view with every range.
- Validate sizes before forming pointers or allocation sizes.
- Preserve `const` for read-only access.
- Use one declaration per pointer variable.
- Prefer `sizeof *ptr` in C allocation expressions.
- Document owner, borrower, transfer, and release behavior.
- Keep one clear owner for manually managed resources.
- Do not release memory through a borrowed pointer.
- Do not retain a borrowed pointer beyond the owner's lifetime.
- Use `T **out` only when replacing the caller's pointer is part of the API.
- Define output values on every success and failure path.
- Pair allocation and release families exactly.
- Use a temporary result with `realloc`.
- Avoid retained aliases and interior pointers across resizing.
- Prefer contiguous storage unless ragged allocation is a real requirement.
- Use exact callback types and explicit context pointers in C.
- Unregister callbacks before destroying retained context.
- Avoid pointer casts that merely silence the compiler.
- Use `memcpy` for representation transfer instead of incompatible typed
  dereference.
- Prefer standard containers, `std::span`, and RAII ownership in production
  C++.
- Run warnings and sanitizers continuously, not only after a production crash.

## 21. Controlled Advanced Topics

### 21.1 Strict aliasing

The language restricts which lvalue types may access an object's stored value.
This lets optimizers reason about whether differently typed accesses can refer
to the same object.

Unsafe type punning:

```c
float value = 1.0F;
uint32_t bits = *(uint32_t *)&value; /* not a portable solution */
```

Portable representation copy:

```c
#include <stdint.h>
#include <string.h>

float value = 1.0F;
uint32_t bits;

_Static_assert(sizeof bits == sizeof value,
               "example requires equal-sized types");
memcpy(&bits, &value, sizeof bits);
```

The copied bits are still implementation-dependent as a floating-point
representation, but the copy avoids incompatible typed access.

### 21.2 `restrict` in C

`restrict` is a promise about how an object is accessed during an execution:

```c
void add_arrays(size_t count,
                int * restrict output,
                const int * restrict left,
                const int * restrict right)
{
    for (size_t i = 0; i < count; ++i) {
        output[i] = left[i] + right[i];
    }
}
```

The caller must satisfy the non-aliasing access contract. Passing overlapping
regions when the function's restricted accesses conflict can produce undefined
behavior.

Use `restrict` only when:

- the performance benefit matters;
- the contract can be documented;
- callers can realistically satisfy it;
- tests and review cover overlap assumptions.

It is not a runtime check and not a general property that no aliases exist
anywhere.

### 21.3 Pointer provenance

Modern language work increasingly describes pointers as carrying an
association with their originating object or allocation, not only a numeric
address.

The practical rules are:

- derive pointers through valid language operations;
- keep arithmetic within the correct object;
- do not fabricate object access through arbitrary integer arithmetic;
- be conservative with pointer/integer round trips;
- consult the selected language standard and implementation documentation for
  low-level allocator, runtime, or hardware work.

Deep provenance rules remain an expert topic and evolve across standard
versions.

## 22. Interview Readiness

### 22.1 What is a pointer?

A strong answer:

> A pointer is an object whose value can designate another object or function,
> or represent a null state. Dereferencing accesses the designated target.
> Valid use depends on lifetime, bounds, type, alignment, and access rules, not
> merely on the pointer being non-null.

### 22.2 Pointer versus array

> An array is an object containing a fixed sequence of elements. A pointer is a
> separate object containing a pointer value. Array expressions often convert
> to a pointer to the first element, which is why the two are easily confused,
> but `sizeof`, address-of, type, ownership, and arithmetic expose the
> difference.

### 22.3 What is a one-past pointer?

> A one-past pointer represents the boundary immediately after an array. It is
> useful for iteration and same-array comparison or subtraction, but it does
> not designate an element and must not be dereferenced.

### 22.4 Why can a non-null pointer be invalid?

It may:

- dangle after lifetime ends;
- lie outside the valid array domain;
- be misaligned;
- have an incompatible access type;
- point to read-only storage for a write;
- retain a stale value after `realloc`;
- refer to callback context that has been destroyed.

### 22.5 Double pointer use cases

Good examples:

- replacing a caller's pointer;
- returning an allocated object through a C output parameter;
- traversing an array of pointers;
- managing an opaque handle through an API.

Clarify that `T **` is not automatically a 2D array.

### 22.6 Const pointer variations

```text
const int *p        pointer to const int
int * const p       const pointer to int
const int * const p const pointer to const int
```

Explain separately whether the pointer may be reseated and whether the pointee
may be modified through that access path.

### 22.7 Function pointer syntax

```c
int (*operation)(int, int);
```

`operation` points to a function taking two `int` parameters and returning
`int`. The parentheses distinguish it from a function returning `int *`.

### 22.8 Why is modifying a string literal undefined behavior?

String literals are not mutable program storage. In C, a narrow string literal
has array type that historically permits conversion to `char *`, but attempting
to modify it is undefined behavior. Use `const char *` for read-only access or
initialize a character array for mutable text. In C++, string literal elements
are const.

### 22.9 Why is setting a pointer to null after release insufficient?

It changes only that pointer object. Other copied pointers may still contain
the old value and become dangling. Correct ownership and borrow lifetimes are
the real solution.

### 22.10 Senior-level discussion

Be ready to discuss:

- owner/view separation;
- callback context lifetime and cancellation;
- pointer invalidation after container growth or `realloc`;
- same-array requirements for arithmetic and ordered comparison;
- alignment and typed access after `void *`;
- strict aliasing and `restrict`;
- why arbitrary pointer/integer conversions are nonportable;
- how optimization changes undefined-behavior symptoms;
- when raw pointers are appropriate in modern C++.

## 23. Practice

### Basic

1. Draw pointer object, pointer value, and pointee for `int *ptr = &value`.
2. Write a program that modifies an `int` through a pointer.
3. Demonstrate the three const-pointer forms.
4. Print an array using pointer plus count.
5. Explain the types of `values`, `&values[0]`, and `&values`.
6. Parse `int (*p)[4]` and `int *p[4]`.
7. Write and call a basic function pointer.

### Intermediate

1. Implement `find_value(const int *, size_t, int, size_t *)`.
2. Implement a C allocation function using `int **out_values`.
3. Define exact output state and ownership behavior for every error path.
4. Implement a callback iterator with function pointer and `void *context`.
5. Build both a contiguous matrix and separately allocated row matrix.
6. Refactor a pointer-only buffer API to pointer plus extent.
7. Write a C++ alternative using `std::span<const int>`.
8. Trigger and diagnose an out-of-bounds write with AddressSanitizer.

### Advanced

1. Review a parser and document every pointer's owner, extent, and lifetime.
2. Design an opaque C sensor handle with create/read/destroy operations.
3. Add callback registration and define safe unregistration and teardown.
4. Demonstrate why a stale alias remains invalid after the owner is reset.
5. Diagnose a use-after-free under both `-O0` and `-O2`.
6. Replace incompatible type punning with `memcpy`.
7. Write a `restrict`-qualified array function and document its overlap
   preconditions.
8. Compare raw pointer, reference, `std::span`, and `std::unique_ptr` interface
   choices for one API.

## 24. Summary

- A pointer object stores a pointer value; the pointee is a separate object.
- Non-null does not mean valid.
- Dereference requires valid lifetime, bounds, type, alignment, and access.
- An array is not a pointer, although array expressions often decay to a
  pointer to the first element.
- Pointer arithmetic belongs to one array object and its one-past boundary.
- A one-past pointer is a sentinel, not an element.
- `T **`, `T (*)[N]`, and flat `T *` layouts are different.
- Pointer-to-const, const-pointer, and const-pointer-to-const protect different
  things.
- Raw pointers do not express ownership.
- Resetting one pointer after release does not repair aliases.
- Function callbacks require an exact signature and a valid context lifetime.
- `void *` erases static type information, not alignment, lifetime, bounds, or
  ownership requirements.
- Allocation and release families must match.
- C APIs need explicit pointer, extent, ownership, and failure contracts.
- Modern C++ should normally use containers and RAII for ownership while using
  raw pointers or references for appropriate non-owning access.
- Warnings, sanitizers, Valgrind, and GDB help find pointer bugs, but clear
  contracts prevent them.

## Reference Notes

- ISO C rules for pointer conversions, array-to-pointer conversion, arithmetic,
  qualification, string literals, and object lifetime.
- ISO C++ rules for pointers, references, arrays, null pointer values, object
  lifetime, and callable types.
- SEI CERT C guidance for null dereference, out-of-bounds pointers, freed
  memory, alignment, incompatible typed access, and string literals.
- C++ Core Guidelines for ownership, interfaces, bounds, RAII, and smart
  pointers.
