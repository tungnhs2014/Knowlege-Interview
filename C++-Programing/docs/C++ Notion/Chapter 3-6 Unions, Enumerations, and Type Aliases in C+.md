# 3.6. Unions, Enumerations, and Type Aliases in C++

---

## Table of Contents

### Unions

1. What are Unions?
2. Union Declaration and Usage
3. Union Memory Layout
4. Union vs Structure
5. Nested Unions
6. Anonymous Unions
7. Tagged Unions
8. Use Cases and Applications

### Enumerations

1. What are Enumerations?
2. Traditional Enums
3. Enum Class (C++11)
4. Enum Values and Underlying Types
5. Scoped vs Unscoped Enums

### Type Aliases

1. typedef Keyword
2. using Declaration (C++11)
3. typedef vs using
4. Advanced Type Aliasing

---

# UNIONS

## 1. What are Unions?

### 1.1 Definition

**A union is a user-defined data type where all members share the same memory location.** Unlike structures where each member has its own memory, union members overlap. Only one member can hold a value at any given time.

**Core Concept:**

```cpp
#include <iostream>
using namespace std;

union Data {
    int i;
    float f;
    char c;
};

int main() {
    Data d;

    // WHY: All members share same memory
    d.i = 10;
    cout << "After storing int: " << d.i << endl;

    d.f = 3.14;  // Overwrites the int
    cout << "After storing float: " << d.f << endl;
    // cout << d.i;  // Garbage value now!

    d.c = 'A';  // Overwrites the float
    cout << "After storing char: " << d.c << endl;

    return 0;
}
```

**Output:**

```
After storing int: 10
After storing float: 3.14
After storing char: A
```

### 1.2 Why Unions Exist

**Purpose:**

1. **Memory Optimization**
    - Save memory by sharing space
    - Only one member active at a time
2. **Type Punning**
    - Reinterpret data in different formats
    - Low-level bit manipulation
3. **Hardware Register Mapping**
    - Model hardware with multiple interpretations
    - Embedded systems programming
4. **Protocol Implementation**
    - Network packets with variant fields
    - Communication protocols

**Real-World Applications:**

- Embedded systems (limited memory)
- Device drivers (hardware registers)
- Network programming (packet headers)
- Graphics programming (pixel formats)
- Compiler implementations (AST nodes)

### 1.3 The Key Principle

**Memory Sharing Rule:**

```cpp
#include <iostream>
using namespace std;

union Example {
    int x;      // 4 bytes
    char c;     // 1 byte
    double d;   // 8 bytes
};

int main() {
    Example e;

    // WHY: All members at same address
    cout << "Address of e: " << &e << endl;
    cout << "Address of e.x: " << &e.x << endl;
    cout << "Address of e.c: " << (void*)&e.c << endl;
    cout << "Address of e.d: " << &e.d << endl;

    return 0;
}
```

**Output (addresses will be same):**

```
Address of e: 0x7ffd12345678
Address of e.x: 0x7ffd12345678
Address of e.c: 0x7ffd12345678
Address of e.d: 0x7ffd12345678
```

---

## 2. Union Declaration and Usage

### 2.1 Basic Syntax

```cpp
union union_name {
    data_type member1;
    data_type member2;
    ...
};
```

**Example:**

```cpp
#include <iostream>
using namespace std;

union Number {
    int intVal;
    float floatVal;
    double doubleVal;
};

int main() {
    Number num;

    num.intVal = 42;
    cout << "Integer: " << num.intVal << endl;

    num.floatVal = 3.14f;
    cout << "Float: " << num.floatVal << endl;

    return 0;
}
```

### 2.2 Initialization

```cpp
#include <iostream>
using namespace std;

union Value {
    int i;
    float f;
    char c;
};

int main() {
    // WHY: Initialize first member only
    Value v1 = {100};  // Initializes i
    cout << "v1.i: " << v1.i << endl;

    // C++20: Designated initializer
    // Value v2 = {.f = 3.14f};

    return 0;
}
```

### 2.3 Accessing Members

```cpp
#include <iostream>
using namespace std;

union Student {
    int rollNo;
    float height;
    char grade;
};

int main() {
    Student s;

    // WHY: Set and read one member at a time
    s.rollNo = 101;
    cout << "Roll No: " << s.rollNo << endl;

    s.height = 5.8f;  // Overwrites rollNo
    cout << "Height: " << s.height << endl;

    s.grade = 'A';  // Overwrites height
    cout << "Grade: " << s.grade << endl;

    return 0;
}
```

---

## 3. Union Memory Layout

### 3.1 Size of Union

**Rule:** Union size = size of largest member (+ padding for alignment)

```cpp
#include <iostream>
using namespace std;

union Small {
    char c;     // 1 byte
    int i;      // 4 bytes
};

union Large {
    char c;     // 1 byte
    int arr[10]; // 40 bytes
};

int main() {
    cout << "sizeof(Small): " << sizeof(Small) << " bytes" << endl;
    cout << "sizeof(Large): " << sizeof(Large) << " bytes" << endl;

    cout << "\nComponent sizes:" << endl;
    cout << "char: " << sizeof(char) << endl;
    cout << "int: " << sizeof(int) << endl;
    cout << "int[10]: " << sizeof(int[10]) << endl;

    return 0;
}
```

**Output:**

```
sizeof(Small): 4 bytes
sizeof(Large): 40 bytes

Component sizes:
char: 1
int: 4
int[10]: 40
```

### 3.2 Memory Visualization

```cpp
#include <iostream>
using namespace std;

union Data {
    int i;      // 4 bytes
    char c[4];  // 4 bytes
};

int main() {
    Data d;
    d.i = 0x41424344;  // ASCII: 'A'=0x41, 'B'=0x42, etc.

    // WHY: View same memory as different types
    cout << "As int: 0x" << hex << d.i << endl;
    cout << "As chars: ";
    for (int j = 0; j < 4; j++) {
        cout << d.c[j] << " ";
    }
    cout << endl;

    return 0;
}
```

---

## 4. Union vs Structure

### 4.1 Comparison Table

| Feature | Union | Structure |
| --- | --- | --- |
| **Memory** | Shared by all members | Separate for each member |
| **Size** | Size of largest member | Sum of all members + padding |
| **Access** | One member at a time | All members simultaneously |
| **Use case** | Memory optimization | Data grouping |
| **Initialization** | First member only | All members possible |

### 4.2 Code Comparison

```cpp
#include <iostream>
using namespace std;

struct MyStruct {
    int i;
    char c;
    float f;
};

union MyUnion {
    int i;
    char c;
    float f;
};

int main() {
    cout << "Structure size: " << sizeof(MyStruct) << " bytes" << endl;
    cout << "Union size: " << sizeof(MyUnion) << " bytes" << endl;

    MyStruct s;
    s.i = 10;
    s.c = 'A';
    s.f = 3.14f;

    cout << "\nStruct (all accessible):" << endl;
    cout << "i=" << s.i << ", c=" << s.c << ", f=" << s.f << endl;

    MyUnion u;
    u.i = 10;
    cout << "\nUnion (only last written):" << endl;
    cout << "After u.i=10: " << u.i << endl;

    u.f = 3.14f;  // Overwrites i
    cout << "After u.f=3.14: " << u.f << endl;
    // cout << u.i;  // Garbage!

    return 0;
}
```

**Output:**

```
Structure size: 12 bytes
Union size: 4 bytes

Struct (all accessible):
i=10, c=A, f=3.14

Union (only last written):
After u.i=10: 10
After u.f=3.14: 3.14
```

### 4.3 When to Use Each

**Use Union When:**

- Memory is constrained
- Only one interpretation needed at a time
- Implementing variant types
- Low-level bit manipulation

**Use Structure When:**

- Need all data simultaneously
- Modeling real-world entities
- Multiple attributes per object
- Memory not a critical concern

---

## 5. Nested Unions

### 5.1 Union Inside Structure

```cpp
#include <iostream>
using namespace std;

struct Employee {
    int id;
    char name[50];

    // WHY: Nested union for payment type
    union Payment {
        float hourlyRate;
        float salary;
    } payment;
};

int main() {
    Employee e1;
    e1.id = 101;

    // WHY: Employee is hourly
    e1.payment.hourlyRate = 25.50;
    cout << "ID: " << e1.id << endl;
    cout << "Hourly Rate: $" << e1.payment.hourlyRate << endl;

    Employee e2;
    e2.id = 102;

    // WHY: Employee is salaried
    e2.payment.salary = 50000;
    cout << "\nID: " << e2.id << endl;
    cout << "Salary: $" << e2.payment.salary << endl;

    return 0;
}
```

### 5.2 Structure Inside Union

```cpp
#include <iostream>
using namespace std;

union Data {
    int singleValue;

    struct Range {
        int min;
        int max;
    } range;
};

int main() {
    Data d;

    // Option 1: Single value
    d.singleValue = 42;
    cout << "Single: " << d.singleValue << endl;

    // Option 2: Range
    d.range.min = 10;
    d.range.max = 100;
    cout << "Range: " << d.range.min << " to " << d.range.max << endl;

    return 0;
}
```

---

## 6. Anonymous Unions

### 6.1 Declaration Without Name

```cpp
#include <iostream>
using namespace std;

struct Pixel {
    union {  // WHY: Anonymous union
        struct { unsigned char r, g, b, a; };
        unsigned int color;
    };
};

int main() {
    Pixel p;

    // WHY: Access union members directly
    p.r = 255;
    p.g = 128;
    p.b = 64;
    p.a = 255;

    cout << "RGBA: (" << (int)p.r << ", " << (int)p.g << ", "
         << (int)p.b << ", " << (int)p.a << ")" << endl;
    cout << "Color value: " << p.color << endl;

    return 0;
}
```

### 6.2 Benefits and Restrictions

**Benefits:**

- Direct access to members (no union name needed)
- Cleaner syntax
- Common in system programming

**Restrictions:**

- Members must have unique names
- Cannot have constructors/destructors
- Must be in class/struct scope (C++)

```cpp
#include <iostream>
using namespace std;

struct Vector3D {
    union {
        struct { float x, y, z; };
        float coords[3];
    };

    void display() {
        cout << "(" << x << ", " << y << ", " << z << ")" << endl;
        cout << "Array: [" << coords[0] << ", "
             << coords[1] << ", " << coords[2] << "]" << endl;
    }
};

int main() {
    Vector3D v;
    v.x = 1.0;
    v.y = 2.0;
    v.z = 3.0;

    v.display();

    return 0;
}
```

---

## 7. Tagged Unions

### 7.1 Type-Safe Unions

**Problem:** How to know which member is currently valid?

**Solution:** Add a "tag" field to track active member

```cpp
#include <iostream>
using namespace std;

enum class ValueType {
    INT,
    FLOAT,
    STRING
};

struct TaggedValue {
    ValueType type;  // WHY: Tag to track active member

    union {
        int intVal;
        float floatVal;
        char stringVal[32];
    };
};

int main() {
    TaggedValue val1;
    val1.type = ValueType::INT;
    val1.intVal = 42;

    // WHY: Check tag before accessing
    if (val1.type == ValueType::INT) {
        cout << "Integer value: " << val1.intVal << endl;
    }

    TaggedValue val2;
    val2.type = ValueType::FLOAT;
    val2.floatVal = 3.14f;

    if (val2.type == ValueType::FLOAT) {
        cout << "Float value: " << val2.floatVal << endl;
    }

    return 0;
}
```

### 7.2 std::variant (C++17) - Modern Alternative

```cpp
#include <iostream>
#include <variant>
#include <string>
using namespace std;

int main() {
    // WHY: Type-safe union replacement
    variant<int, float, string> value;

    value = 42;
    cout << "Int: " << get<int>(value) << endl;

    value = 3.14f;
    cout << "Float: " << get<float>(value) << endl;

    value = string("Hello");
    cout << "String: " << get<string>(value) << endl;

    return 0;
}
```

---

## 8. Use Cases and Applications

### 8.1 Embedded Systems - Register Mapping

```cpp
#include <iostream>
using namespace std;

// WHY: Model hardware register with multiple interpretations
union StatusRegister {
    unsigned int raw;  // All 32 bits

    struct {
        unsigned int bit0 : 1;
        unsigned int bit1 : 1;
        unsigned int bit2 : 1;
        unsigned int reserved : 29;
    } bits;
};

int main() {
    StatusRegister reg;
    reg.raw = 0x05;  // Binary: 101

    cout << "Raw value: 0x" << hex << reg.raw << endl;
    cout << "Bit 0: " << reg.bits.bit0 << endl;
    cout << "Bit 1: " << reg.bits.bit1 << endl;
    cout << "Bit 2: " << reg.bits.bit2 << endl;

    return 0;
}
```

### 8.2 Type Punning (Be Careful!)

```cpp
#include <iostream>
using namespace std;

union FloatInt {
    float f;
    int i;
};

int main() {
    FloatInt fi;
    fi.f = 3.14f;

    // WHY: View float bits as integer
    cout << "Float: " << fi.f << endl;
    cout << "Bits (as int): 0x" << hex << fi.i << endl;

    // ⚠️ WARNING: Type punning can be undefined behavior!
    // Use memcpy or bit_cast (C++20) for portable code

    return 0;
}
```

### 8.3 Network Packet Header

```cpp
#include <iostream>
using namespace std;

struct PacketHeader {
    union {
        unsigned int flags;

        struct {
            unsigned int version : 4;
            unsigned int type : 4;
            unsigned int length : 8;
            unsigned int reserved : 16;
        } fields;
    };
};

int main() {
    PacketHeader header;
    header.fields.version = 4;
    header.fields.type = 2;
    header.fields.length = 64;
    header.fields.reserved = 0;

    cout << "Raw flags: 0x" << hex << header.flags << endl;
    cout << "Version: " << dec << header.fields.version << endl;
    cout << "Type: " << header.fields.type << endl;
    cout << "Length: " << header.fields.length << endl;

    return 0;
}
```

---

# ENUMERATIONS

## 9. What are Enumerations?

### 9.1 Definition

**An enumeration (enum) is a user-defined type consisting of a set of named integer constants called enumerators.** Enums make code more readable and maintainable by giving meaningful names to integral values.

**Core Concept:**

```cpp
#include <iostream>
using namespace std;

// WHY: Named constants instead of magic numbers
enum Color {
    RED,      // 0
    GREEN,    // 1
    BLUE,     // 2
    YELLOW    // 3
};

int main() {
    Color favColor = RED;

    if (favColor == RED) {
        cout << "Favorite color is red!" << endl;
    }

    cout << "Color value: " << favColor << endl;  // Prints 0

    return 0;
}
```

### 9.2 Why Enums Exist

**Purpose:**

1. **Readability**
    - `direction = NORTH` vs `direction = 0`
    - Self-documenting code
2. **Type Safety** (enum class)
    - Prevent invalid values
    - Compiler catches errors
3. **Maintainability**
    - Change values in one place
    - No magic numbers scattered
4. **Switch Statements**
    - Compiler can warn about missing cases
    - Clear intent

**Real-World Applications:**

- State machines (IDLE, RUNNING, STOPPED)
- Direction enums (NORTH, SOUTH, EAST, WEST)
- Days of week, months
- Error codes, status codes
- Configuration options

---

## 10. Traditional Enums

### 10.1 Basic Declaration

```cpp
#include <iostream>
using namespace std;

enum Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

int main() {
    Direction dir = NORTH;

    cout << "Direction: " << dir << endl;  // Prints 0

    switch (dir) {
        case NORTH:
            cout << "Moving North" << endl;
            break;
        case SOUTH:
            cout << "Moving South" << endl;
            break;
        case EAST:
            cout << "Moving East" << endl;
            break;
        case WEST:
            cout << "Moving West" << endl;
            break;
    }

    return 0;
}
```

### 10.2 Default Values

**Rule:** First enumerator = 0, rest increment by 1

```cpp
#include <iostream>
using namespace std;

enum Month {
    JANUARY,    // 0
    FEBRUARY,   // 1
    MARCH,      // 2
    APRIL,      // 3
    MAY,        // 4
    JUNE        // 5
};

int main() {
    cout << "JANUARY: " << JANUARY << endl;
    cout << "MARCH: " << MARCH << endl;

    return 0;
}
```

**Output:**

```
JANUARY: 0
MARCH: 2

```

### 10.3 Custom Values

```cpp
#include <iostream>
using namespace std;

enum HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500
};

enum Level {
    LOW = 1,
    MEDIUM = 5,
    HIGH = 10
};

int main() {
    HttpStatus status = OK;
    Level level = MEDIUM;

    cout << "HTTP Status: " << status << endl;
    cout << "Level: " << level << endl;

    return 0;
}
```

### 10.4 Automatic Increment After Custom Value

```cpp
#include <iostream>
using namespace std;

enum Priority {
    LOWEST = 1,
    LOW,        // 2
    NORMAL = 5,
    HIGH,       // 6
    CRITICAL    // 7
};

int main() {
    cout << "LOWEST: " << LOWEST << endl;
    cout << "LOW: " << LOW << endl;
    cout << "NORMAL: " << NORMAL << endl;
    cout << "HIGH: " << HIGH << endl;
    cout << "CRITICAL: " << CRITICAL << endl;

    return 0;
}
```

**Output:**

```
LOWEST: 1
LOW: 2
NORMAL: 5
HIGH: 6
CRITICAL: 7

```

---

## 11. Enum Class (C++11)

### 11.1 Scoped Enumerations

**Problem with Traditional Enums:** Enumerators pollute enclosing scope

```cpp
// Traditional enum - PROBLEM
enum Color { RED, GREEN, BLUE };
enum TrafficLight { RED, YELLOW, GREEN };  // ❌ ERROR: Redefinition!
```

**Solution: enum class**

```cpp
#include <iostream>
using namespace std;

enum class Color {
    RED,
    GREEN,
    BLUE
};

enum class TrafficLight {
    RED,      // ✅ OK: Different scope
    YELLOW,
    GREEN
};

int main() {
    // WHY: Must use scope resolution
    Color c = Color::RED;
    TrafficLight t = TrafficLight::RED;

    // ❌ Color c2 = RED;  // ERROR: RED not in scope

    cout << "Valid enum class usage" << endl;

    return 0;
}
```

### 11.2 Type Safety

```cpp
#include <iostream>
using namespace std;

enum class DayOfWeek {
    MONDAY,
    TUESDAY,
    WEDNESDAY
};

int main() {
    DayOfWeek day = DayOfWeek::MONDAY;

    // ❌ Traditional enum: implicit conversion to int
    // if (day == 0) { ... }  // ERROR with enum class!

    // ✅ Must explicitly cast
    if (static_cast<int>(day) == 0) {
        cout << "It's Monday!" << endl;
    }

    // ❌ Cannot compare different enum classes
    // enum class Month { JAN, FEB };
    // if (day == Month::JAN) { ... }  // ERROR!

    return 0;
}
```

### 11.3 Underlying Type Specification

```cpp
#include <iostream>
using namespace std;

// WHY: Specify storage type explicitly
enum class Status : unsigned char {
    OK,
    ERROR,
    PENDING
};

enum class LargeEnum : long long {
    VALUE1 = 1000000000000LL
};

int main() {
    cout << "sizeof(Status): " << sizeof(Status) << " byte" << endl;
    cout << "sizeof(LargeEnum): " << sizeof(LargeEnum) << " bytes" << endl;

    return 0;
}
```

---

## 12. Enum Values and Underlying Types

### 12.1 Getting Integer Value

```cpp
#include <iostream>
using namespace std;

enum class Level {
    BEGINNER = 1,
    INTERMEDIATE = 5,
    EXPERT = 10
};

int main() {
    Level playerLevel = Level::INTERMEDIATE;

    // WHY: Cast to get underlying value
    int value = static_cast<int>(playerLevel);
    cout << "Player level value: " << value << endl;

    return 0;
}
```

### 12.2 Forward Declaration

```cpp
// Header file (.h)
enum class Result : int;  // Forward declaration

// Can use Result* before full definition
void processResult(Result* r);

// Full definition elsewhere
enum class Result : int {
    SUCCESS = 0,
    FAILURE = 1
};
```

### 12.3 Using enum (C++20)

```cpp
#include <iostream>
using namespace std;

enum class Color {
    RED,
    GREEN,
    BLUE
};

int main() {
    using enum Color;  // C++20: Bring names into scope

    Color c = RED;  // No need for Color::RED

    switch (c) {
        case RED:
            cout << "Red" << endl;
            break;
        case GREEN:
            cout << "Green" << endl;
            break;
        case BLUE:
            cout << "Blue" << endl;
            break;
    }

    return 0;
}
```

---

## 13. Scoped vs Unscoped Enums

### 13.1 Comparison Table

| Feature | Unscoped enum | enum class (Scoped) |
| --- | --- | --- |
| **Scope** | Enumerators in enclosing scope | Enumerators in enum scope |
| **Syntax** | `RED` | `Color::RED` |
| **Type safety** | Implicitly converts to int | No implicit conversion |
| **Comparison** | Can compare with int | Must explicit cast |
| **Name collision** | Possible | Prevented |
| **Underlying type** | Implementation-defined | Can specify explicitly |
| **C++11** | Pre-C++11 | C++11 onwards |

### 13.2 Migration Example

```cpp
#include <iostream>
using namespace std;

// Old style (unscoped)
enum OldColor {
    OLD_RED,
    OLD_GREEN,
    OLD_BLUE
};

// New style (scoped)
enum class NewColor {
    RED,
    GREEN,
    BLUE
};

int main() {
    // Old: Direct access, implicit conversion
    OldColor oc = OLD_RED;
    int val1 = oc;  // ✅ Implicit conversion
    if (oc == 0) {  // ✅ Compare with int
        cout << "Old Red" << endl;
    }

    // New: Scoped access, explicit conversion
    NewColor nc = NewColor::RED;
    // int val2 = nc;  // ❌ ERROR
    int val2 = static_cast<int>(nc);  // ✅ Explicit cast
    // if (nc == 0) { ... }  // ❌ ERROR
    if (static_cast<int>(nc) == 0) {  // ✅ Must cast
        cout << "New Red" << endl;
    }

    return 0;
}
```

### 13.3 Best Practices

**Prefer enum class for:**

- New code (C++11+)
- Type safety important
- Avoiding name collisions
- API design

**Use traditional enum for:**

- Legacy compatibility
- Bitwise flags (with explicit int conversion)
- When implicit conversion desired

---

# PART 3.8 - TYPE ALIASES

## 14. typedef Keyword

### 14.1 Basic Syntax

```cpp
typedef existing_type new_name;
```

**Examples:**

```cpp
#include <iostream>
using namespace std;

// WHY: Create shorter or more meaningful names
typedef unsigned long long ull;
typedef int* IntPtr;

int main() {
    ull bigNumber = 123456789012345ULL;
    IntPtr ptr;

    cout << "Big number: " << bigNumber << endl;

    return 0;
}
```

### 14.2 Common Use Cases

**For Readability:**

```cpp
#include <iostream>
#include <vector>
using namespace std;

// WHY: Simplify complex type names
typedef vector<int> IntVector;
typedef vector<string> StringVector;

int main() {
    IntVector numbers = {1, 2, 3, 4, 5};
    StringVector words = {"Hello", "World"};

    for (int n : numbers) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

**For Portability:**

```cpp
// WHY: Change type in one place
typedef int32_t GameScore;
typedef float Coordinate;

GameScore playerScore = 100;
Coordinate x = 10.5f, y = 20.3f;
```

### 14.3 typedef with Structures

```cpp
#include <iostream>
using namespace std;

// Method 1: Separate typedef
struct Point {
    int x, y;
};
typedef struct Point Point2D;

// Method 2: Combined definition
typedef struct {
    float x, y, z;
} Point3D;

int main() {
    Point2D p1 = {10, 20};
    Point3D p2 = {1.0, 2.0, 3.0};

    cout << "2D Point: (" << p1.x << ", " << p1.y << ")" << endl;
    cout << "3D Point: (" << p2.x << ", " << p2.y << ", " << p2.z << ")" << endl;

    return 0;
}
```

### 14.4 typedef with Function Pointers

```cpp
#include <iostream>
using namespace std;

// WHY: Simplify function pointer syntax
typedef int (*Operation)(int, int);

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

int main() {
    Operation op = add;
    cout << "5 + 3 = " << op(5, 3) << endl;

    op = multiply;
    cout << "5 * 3 = " << op(5, 3) << endl;

    return 0;
}
```

---

## 15. using Declaration (C++11)

### 15.1 Modern Syntax

```cpp
using new_name = existing_type;
```

**Examples:**

```cpp
#include <iostream>
#include <vector>
using namespace std;

// WHY: Clearer than typedef
using IntVector = vector<int>;
using StringVector = vector<string>;
using IntPtr = int*;

int main() {
    IntVector numbers = {1, 2, 3};
    IntPtr ptr = &numbers[0];

    cout << "First element: " << *ptr << endl;

    return 0;
}
```

### 15.2 Template Aliases

**typedef Limitation:**

```cpp
// ❌ Cannot do this with typedef:
// typedef vector<T> Vec<T>;  // ERROR!
```

**using Solution:**

```cpp
#include <iostream>
#include <vector>
using namespace std;

// WHY: Template aliases possible with using
template<typename T>
using Vec = vector<T>;

template<typename K, typename V>
using Map = std::map<K, V>;

int main() {
    Vec<int> numbers = {1, 2, 3};
    Vec<string> words = {"Hello", "World"};

    for (auto n : numbers) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

### 15.3 Complex Type Simplification

```cpp
#include <iostream>
#include <functional>
using namespace std;

// WHY: Simplify complex template types
using Callback = function<void(int)>;
using Transform = function<int(int)>;

void process(int value, Callback cb) {
    cb(value);
}

int main() {
    Callback printNum = [](int x) {
        cout << "Number: " << x << endl;
    };

    process(42, printNum);

    return 0;
}
```

---

## 16. typedef vs using

### 16.1 Syntax Comparison

```cpp
#include <iostream>
#include <vector>
using namespace std;

// typedef syntax
typedef vector<int> IntVec1;
typedef int* IntPtr1;

// using syntax (C++11)
using IntVec2 = vector<int>;
using IntPtr2 = int*;

int main() {
    IntVec1 v1 = {1, 2, 3};
    IntVec2 v2 = {4, 5, 6};

    // Both work identically
    cout << "v1 size: " << v1.size() << endl;
    cout << "v2 size: " << v2.size() << endl;

    return 0;
}
```

### 16.2 Comparison Table

| Feature | typedef | using |
| --- | --- | --- |
| **Syntax** | `typedef old new` | `using new = old` |
| **Readability** | Less clear for complex types | More intuitive |
| **Template aliases** | ❌ Not supported | ✅ Supported |
| **Consistency** | Different from variable syntax | Similar to assignment |
| **C++ version** | C++98 | C++11 |
| **Recommendation** | Legacy code | New code |

### 16.3 Why using is Preferred

```cpp
#include <iostream>
#include <map>
#include <string>
using namespace std;

// typedef: Order feels backwards
typedef map<string, int> StringIntMap_typedef;

// using: More natural "alias = type"
using StringIntMap_using = map<string, int>;

// typedef: Cannot do templates
// typedef vector<T> Vec<T>;  // ❌ ERROR

// using: Template aliases work
template<typename T>
using Vec = vector<T>;

int main() {
    StringIntMap_using scores = {{"Alice", 95}, {"Bob", 87}};

    Vec<int> numbers = {1, 2, 3};
    Vec<string> words = {"Hello", "World"};

    cout << "Scores:" << endl;
    for (auto& [name, score] : scores) {
        cout << name << ": " << score << endl;
    }

    return 0;
}
```

---

## 17. Advanced Type Aliasing

### 17.1 Member Type Aliases

```cpp
#include <iostream>
#include <vector>
using namespace std;

template<typename T>
class Container {
public:
    using value_type = T;
    using iterator = typename vector<T>::iterator;

    void add(value_type val) {
        data.push_back(val);
    }

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }

private:
    vector<T> data;
};

int main() {
    Container<int> cont;
    cont.add(10);
    cont.add(20);

    // Use member type alias
    Container<int>::value_type sum = 0;
    for (Container<int>::iterator it = cont.begin();
         it != cont.end(); ++it) {
        sum += *it;
    }

    cout << "Sum: " << sum << endl;

    return 0;
}
```

### 17.2 Conditional Type Aliases

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: Choose type based on condition
template<bool Condition>
using IntOrDouble = typename conditional<Condition, int, double>::type;

int main() {
    IntOrDouble<true> x = 42;      // int
    IntOrDouble<false> y = 3.14;   // double

    cout << "x: " << x << endl;
    cout << "y: " << y << endl;

    return 0;
}
```

### 17.3 Real-World Example

```cpp
#include <iostream>
#include <memory>
#include <vector>
using namespace std;

// WHY: Clean API with type aliases
class GameEngine {
public:
    using EntityID = unsigned int;
    using ComponentPtr = shared_ptr<class Component>;
    using EntityList = vector<EntityID>;

    EntityID createEntity() {
        static EntityID nextID = 0;
        return nextID++;
    }

    void displayEntity(EntityID id) {
        cout << "Entity ID: " << id << endl;
    }
};

int main() {
    GameEngine engine;

    GameEngine::EntityID player = engine.createEntity();
    GameEngine::EntityID enemy = engine.createEntity();

    engine.displayEntity(player);
    engine.displayEntity(enemy);

    return 0;
}
```

---

## Summary

### Key Takeaways

**Unions:**

1. **Shared memory** - All members occupy same location, only one active at a time
2. **Size = largest member** - Union size equals size of biggest member
3. **Memory optimization** - Save space when only one interpretation needed
4. **Use tagged unions** - Track active member with separate type field
5. **Anonymous unions** - Access members directly without union name
6. **Modern alternative** - std::variant (C++17) provides type-safe unions

**Enumerations:**
7. **Named constants** - Give meaningful names to integral values
8. **Traditional enum** - Enumerators in enclosing scope, implicit int conversion
9. **enum class (C++11)** - Scoped enumerators, no implicit conversion, type safe
10. **Default values** - Start at 0, increment by 1 unless specified
11. **Prefer enum class** - Better type safety and avoids name collisions

**Type Aliases:**
12. **typedef (legacy)** - Create aliases for existing types, backwards syntax
13. **using (modern)** - Clearer syntax, supports template aliases (C++11+)
14. **Template aliases** - Only possible with using, not typedef
15. **Prefer using** - More readable and consistent with modern C++

### Interview Essential Points

**Q: What is a union and how does it differ from a structure?**

A: A union is a user-defined type where all members share the same memory location, while a structure allocates separate memory for each member. Union size equals the largest member's size; structure size is the sum of all members plus padding. In a union, only one member can hold a valid value at a time - writing to one member overwrites others. Unions are used for memory optimization (embedded systems), type punning (viewing same data differently), and modeling hardware registers. Structures are used when you need all data simultaneously. Example: `union {int i; float f;}` uses 4 bytes, `struct {int i; float f;}` uses 8 bytes.

**Q: When would you use a union in practice?**

A: Unions are primarily used in: (1) Embedded systems with limited memory - save space when only one interpretation needed at a time. (2) Hardware register mapping - model registers that can be accessed as whole or individual bits using bit fields. (3) Network protocol implementation - packet headers with variant fields. (4) Type punning - viewing same memory as different types (though use with caution due to strict aliasing rules). (5) Tagged unions - with a separate type field to track which member is active. Modern C++ prefers std::variant (C++17) over unions for type-safe variant types, but unions remain important for low-level programming and C compatibility.

**Q: Explain the difference between traditional enum and enum class.**

A: Traditional enum (unscoped): Enumerators pollute enclosing scope, implicitly convert to int, can cause name collisions. Syntax: `Color c = RED;`. enum class (scoped, C++11+): Enumerators within enum scope, no implicit conversion to int, prevents name collisions, stronger type safety. Syntax: `Color c = Color::RED;`. Example of problem with traditional: `enum Color {RED}; enum Light {RED};` causes error. With enum class: `enum class Color {RED}; enum class Light {RED};` works fine. enum class requires explicit cast to int: `static_cast<int>(value)`. Modern C++ strongly prefers enum class for type safety and preventing accidental misuse.

**Q: How do you specify the underlying type for an enum?**

A: For enum class, use colon syntax: `enum class Status : unsigned char {OK, ERROR};`. This makes Status occupy only 1 byte instead of default int (4 bytes). Useful for memory-constrained environments or binary compatibility. Traditional enums can also specify underlying type in C++11+: `enum Color : short {RED, GREEN, BLUE};`. If not specified, compiler chooses an integral type (usually int) that can represent all enumerator values. Specifying underlying type also enables forward declaration: `enum class Result : int;` can be declared before full definition, useful for reducing header dependencies.

**Q: What is the difference between typedef and using for type aliases?**

A: Both create aliases for existing types, but using (C++11+) is preferred. Key differences: (1) Syntax - typedef uses backwards C syntax `typedef old new`, using is more intuitive `using new = old`. (2) Template aliases - typedef CANNOT create template aliases, using CAN: `template<typename T> using Vec = vector<T>;`. (3) Readability - using reads more naturally like variable assignment. (4) Consistency - using aligns with other C++ declarations. Example: `typedef vector<int> IntVec;` vs `using IntVec = vector<int>;` both work for simple cases, but only using supports `template<typename T> using Ptr = T*;`. Modern C++ code should use using for all type aliases.

**Q: Explain tagged unions and why they're needed.**

A: Tagged unions add a separate "tag" field to track which union member is currently valid. Problem: unions don't remember which member was last written - accessing wrong member gives garbage. Solution: struct containing an enum (the tag) and a union. Before accessing union, check the tag. Example: `struct {enum Type {INT, FLOAT} type; union {int i; float f;} value;}`. Check: `if(tag == INT) use value.i`. This provides type safety - you know which member is valid. C++17's std::variant is the modern standard library solution that does this automatically with type-safe access via `std::get<T>()` and `std::visit()`. Tagged unions are common in compilers (AST nodes), interpreters (dynamic typing), and protocols (variant message types).

**Q: What are anonymous unions and when are they useful?**

A: Anonymous unions are declared without a name, allowing direct access to members without qualifying with union name. Example: `struct {union {int i; float f;};};` lets you access as `obj.i` directly instead of `obj.unionName.i`. Restrictions: members must have unique names within scope, can only be used inside classes/structs in C++, cannot have constructors/destructors. Common uses: (1) Graphics - accessing color as RGBA components or single 32-bit value, (2) Math - accessing vector as x,y,z or array[3], (3) Clean syntax where union nature is obvious. Example: `struct Vector {union {struct{float x,y,z;}; float coords[3];};};` allows both named and array access to same data.

**Q: Why is enum class preferred over traditional enum in modern C++?**

A: enum class provides: (1) **No name pollution** - enumerators scoped within enum, preventing collisions: `enum class A {X}; enum class B {X};` works, traditional enums conflict. (2) **Type safety** - no implicit conversion to int, must explicit cast: `static_cast<int>(val)`, prevents accidental arithmetic or comparisons with wrong types. (3) **Explicit scope** - must use `Color::RED` not just `RED`, making code clearer. (4) **Forward declaration** - can declare before definition with specified underlying type. (5) **Underlying type control** - `enum class : uint8_t` for size optimization. Only downside: more verbose syntax, but the type safety and clarity are worth it. Traditional enums only when need implicit int conversion (rare) or C compatibility.

---