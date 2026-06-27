# 10.1. Namespaces

---

## Table of Contents

1. Introduction to Namespaces
2. Namespace Basics
3. Accessing Namespace Members
4. using Declaration vs using Directive
5. Nested Namespaces
6. Inline Namespaces (C++11)
7. Anonymous Namespaces
8. Namespace Aliases
9. Global Namespace
10. ADL (Argument-Dependent Lookup)
11. Extending Namespaces
12. Classes in Namespaces
13. Best Practices
14. Common Pitfalls
15. Summary

---

## 1. Introduction to Namespaces

### What is a Namespace?

**Namespace** is a declarative region that provides a scope to identifiers (names of types, functions, variables, etc.) to prevent name collisions.

**Think of it as:** A labeled container or room that holds a group of names, preventing confusion when the same name is used in different parts of the program.

### Why Do We Need Namespaces?

**Problem:** Name collisions occur when multiple libraries or code sections use the same identifiers.

```cpp
// Without namespaces - NAME COLLISION!
void print() { cout << "From Library A"; }
void print() { cout << "From Library B"; }  // ERROR: Redefinition!
```

**Solution:** Namespaces isolate identifiers.

```cpp
namespace LibraryA {
    void print() { cout << "From Library A"; }
}

namespace LibraryB {
    void print() { cout << "From Library B"; }
}
```

### Key Benefits

1. **Avoid Name Collisions**: Different entities can have the same name in different namespaces
2. **Code Organization**: Group related functionality logically
3. **Library Development**: Create clean interfaces for libraries
4. **Maintainability**: Easier to manage large codebases

### Real-World Use Cases

- **Standard Library**: All STL components are in `std` namespace
- **Graphics Libraries**: `SDL::`, `OpenGL::`, `DirectX::`
- **Company Projects**: `CompanyName::ProjectName::`
- **Versioning**: `v1::`, `v2::` for API versions

---

## 2. Namespace Basics

### Defining a Namespace

**Syntax:**

```cpp
namespace namespace_name {
    // Declarations: variables, functions, classes
}
```

**Important:** No semicolon (`;`) after the closing brace!

### Example: Basic Namespace

```cpp
#include <iostream>
using namespace std;

// WHY: Define a namespace to group related functions
namespace Math {
    const double PI = 3.14159;

    double square(double x) {
        return x * x;
    }

    double cube(double x) {
        return x * x * x;
    }
}

int main() {
    cout << "PI = " << Math::PI << endl;
    cout << "Square of 5 = " << Math::square(5) << endl;
    cout << "Cube of 3 = " << Math::cube(3) << endl;

    return 0;
}
```

**Output:**

```
PI = 3.14159
Square of 5 = 25
Cube of 3 = 27
```

**Explanation:**

- `Math` namespace groups mathematical constants and functions
- Prevents conflict if another library has `square()` function
- Accessed using scope resolution operator `::`

### Multiple Namespaces

```cpp
#include <iostream>
using namespace std;

// WHY: Separate concerns - different rooms for different purposes
namespace Room1 {
    void greet() {
        cout << "Hello from Room 1!" << endl;
    }
}

namespace Room2 {
    void greet() {
        cout << "Hello from Room 2!" << endl;
    }
}

int main() {
    Room1::greet();  // WHY: Explicitly specify which greet() to call
    Room2::greet();

    return 0;
}
```

**Output:**

```
Hello from Room 1!
Hello from Room 2!
```

**WHY this works:** The compiler knows exactly which `greet()` function to call based on the namespace prefix.

---

## 3. Accessing Namespace Members

### Method 1: Scope Resolution Operator (`::`)

**Most Explicit and Safe Approach**

```cpp
#include <iostream>
using namespace std;

namespace MySpace {
    int value = 100;

    void display() {
        cout << "Value: " << value << endl;
    }
}

int main() {
    // WHY: Explicit access - clear which namespace we're using
    cout << "MySpace::value = " << MySpace::value << endl;
    MySpace::display();

    return 0;
}
```

**Advantages:**

- Most explicit and clear
- No ambiguity
- Recommended for professional code

### Method 2: Using Declaration

```cpp
#include <iostream>
using namespace std;

namespace MySpace {
    int x = 10;
    int y = 20;
}

int main() {
    // WHY: Import only specific identifier
    using MySpace::x;  // Only x is visible

    cout << "x = " << x << endl;          // OK: x is imported
    cout << "y = " << MySpace::y << endl; // Must use MySpace:: for y

    return 0;
}
```

**Advantages:**

- Selective import
- More control than `using namespace`
- Reduces typing while maintaining clarity

### Method 3: Using Directive

```cpp
#include <iostream>
using namespace std;

namespace MySpace {
    int value = 42;
    void print() { cout << "MySpace::print()" << endl; }
}

int main() {
    // WHY: Import entire namespace
    using namespace MySpace;

    cout << "value = " << value << endl;  // No MySpace:: needed
    print();                              // Directly accessible

    return 0;
}
```

**Warning:** Can lead to name collisions. Use with caution!

---

## 4. using Declaration vs using Directive

### using Declaration (`using Name::identifier`)

**Imports a single identifier**

```cpp
#include <iostream>
using std::cout;  // WHY: Only import cout
using std::endl;  // WHY: Only import endl

namespace Math {
    double PI = 3.14159;
    double E = 2.71828;
}

int main() {
    using Math::PI;  // WHY: Import only PI, not E

    cout << "PI = " << PI << endl;           // OK
    cout << "E = " << Math::E << endl;       // Must use Math::
    // cout << "E = " << E << endl;          // ERROR: E not imported

    return 0;
}
```

**When to use:**

- You need only specific identifiers
- Want to avoid polluting the namespace
- Professional/library code

### using Directive (`using namespace Name`)

**Imports entire namespace**

```cpp
#include <iostream>
using namespace std;  // WHY: All std members accessible

namespace Graphics {
    void draw() { cout << "Drawing..." << endl; }
    void render() { cout << "Rendering..." << endl; }
    void display() { cout << "Displaying..." << endl; }
}

int main() {
    using namespace Graphics;  // WHY: All Graphics members accessible

    draw();     // No Graphics:: needed
    render();
    display();

    return 0;
}
```

**Danger Example:**

```cpp
#include <iostream>
using namespace std;

namespace MyLib {
    int count = 10;  // Conflicts with std::count
}

int main() {
    using namespace MyLib;  // BAD: Imports all

    // Ambiguous! Which count?
    // cout << count << endl;  // ERROR: Ambiguous

    // Must be explicit:
    cout << MyLib::count << endl;

    return 0;
}
```

### Comparison Table

| Aspect | using Declaration | using Directive |
| --- | --- | --- |
| **Imports** | Single identifier | Entire namespace |
| **Syntax** | `using NS::name;` | `using namespace NS;` |
| **Safety** | ✅ Safer | ⚠️ Can cause collisions |
| **Clarity** | ✅ More explicit | ❌ Less clear |
| **Use in Headers** | ✅ Acceptable | ❌ Never use |
| **Use in .cpp** | ✅ Preferred | ⚠️ Use carefully |
| **Recommended** | ✅ Yes | ⚠️ Only in limited scope |

### Best Practice Example

```cpp
// header.h
#ifndef HEADER_H
#define HEADER_H

// WHY: Never "using namespace" in headers!
namespace MyLib {
    void function();
}

#endif

// implementation.cpp
#include "header.h"
#include <iostream>

// WHY: using namespace OK in .cpp files, limited scope
void MyLib::function() {
    using std::cout;  // WHY: Selective import
    using std::endl;

    cout << "MyLib::function()" << endl;
}
```

---

## 5. Nested Namespaces

### Basic Nested Namespaces

**Namespaces can be nested for hierarchical organization**

```cpp
#include <iostream>
using namespace std;

// WHY: Organize code hierarchically
namespace Company {
    namespace Product {
        namespace Version {
            int major = 2;
            int minor = 5;
            int patch = 1;

            void displayVersion() {
                cout << "Version " << major << "." << minor << "." << patch << endl;
            }
        }
    }
}

int main() {
    // WHY: Multi-level access with :: operator
    Company::Product::Version::displayVersion();

    cout << "Major: " << Company::Product::Version::major << endl;

    return 0;
}
```

**Output:**

```
Version 2.5.1
Major: 2
```

### Accessing Nested Namespace Members

**Method 1: Full Path**

```cpp
#include <iostream>
using namespace std;

namespace Outer {
    void outerFunc() {
        cout << "Outer namespace" << endl;
    }

    namespace Inner {
        void innerFunc() {
            cout << "Inner namespace" << endl;
        }
    }
}

int main() {
    // WHY: Full qualification - most explicit
    Outer::outerFunc();
    Outer::Inner::innerFunc();

    return 0;
}
```

### Method 2: using for Nested Namespaces

```cpp
#include <iostream>
using namespace std;

namespace Outer {
    namespace Inner {
        int value = 100;
        void display() {
            cout << "Outer::Inner::display()" << endl;
        }
    }
}

int main() {
    // WHY: Import nested namespace members
    using namespace Outer::Inner;

    cout << "value = " << value << endl;  // Directly accessible
    display();

    return 0;
}
```

**Output:**

```
value = 100
Outer::Inner::display()
```

### C++17: Nested Namespace Syntax

**Old Way (before C++17):**

```cpp
namespace A {
    namespace B {
        namespace C {
            int value = 42;
        }
    }
}
```

**New Way (C++17 onwards):**

```cpp
// WHY: Cleaner, more readable syntax
namespace A::B::C {
    int value = 42;
}
```

**Complete Example:**

```cpp
#include <iostream>
using namespace std;

// WHY: C++17 nested namespace - cleaner syntax
namespace Graphics::Rendering::Engine {
    void initialize() {
        cout << "Graphics engine initialized" << endl;
    }

    void render() {
        cout << "Rendering frame..." << endl;
    }
}

int main() {
    Graphics::Rendering::Engine::initialize();
    Graphics::Rendering::Engine::render();

    // Or use alias (covered later)
    namespace GRE = Graphics::Rendering::Engine;
    GRE::render();

    return 0;
}
```

**Output:**

```
Graphics engine initialized
Rendering frame...
Rendering frame...
```

### Practical Example: Project Structure

```cpp
#include <iostream>
using namespace std;

// WHY: Real-world project organization
namespace MyCompany {
    namespace Utils {
        void log(const string& msg) {
            cout << "[LOG] " << msg << endl;
        }
    }

    namespace Database {
        void connect() {
            cout << "[DB] Connecting..." << endl;
        }

        namespace MySQL {
            void query(const string& sql) {
                cout << "[MySQL] Query: " << sql << endl;
            }
        }
    }
}

int main() {
    MyCompany::Utils::log("Application started");
    MyCompany::Database::connect();
    MyCompany::Database::MySQL::query("SELECT * FROM users");

    return 0;
}
```

**Output:**

```
[LOG] Application started
[DB] Connecting...
[MySQL] Query: SELECT * FROM users
```

---

## 6. Inline Namespaces (C++11)

### What are Inline Namespaces?

**Inline namespaces** make nested namespace members appear as if they belong to the enclosing namespace.

**Keyword:** `inline` before `namespace`

### Basic Inline Namespace

```cpp
#include <iostream>
using namespace std;

namespace Outer {
    // WHY: inline makes Inner members accessible from Outer
    inline namespace Inner {
        int value = 10;

        void display() {
            cout << "Inner::display(), value = " << value << endl;
        }
    }
}

int main() {
    // WHY: Can access Inner members through Outer directly!
    cout << Outer::value << endl;  // No need for Outer::Inner::value
    Outer::display();              // No need for Outer::Inner::display

    // But can still use full path
    cout << Outer::Inner::value << endl;

    return 0;
}
```

**Output:**

```
10
Inner::display(), value = 10
10
```

**Key Point:** Members of inline namespace are "dragged out" to the enclosing namespace.

### Nested Inline Namespaces

**Transitive Property:** If `N` contains inline `M`, which contains inline `O`, then `O`'s members can be used as if they were in `N` or `M`.

```cpp
#include <iostream>
using namespace std;

namespace Level1 {
    inline namespace Level2 {
        inline namespace Level3 {
            int var = 42;

            void func() {
                cout << "Level3::func()" << endl;
            }
        }
    }
}

int main() {
    // WHY: All three work due to transitive inline
    cout << Level1::var << endl;                // Through Level1
    cout << Level1::Level2::var << endl;        // Through Level2
    cout << Level1::Level2::Level3::var << endl; // Full path

    Level1::func();           // Shortest path
    Level1::Level2::func();   // Medium path
    Level1::Level2::Level3::func(); // Full path

    return 0;
}
```

**Output:**

```
42
42
42
Level3::func()
Level3::func()
Level3::func()
```

### Use Case: Library Versioning

**Primary Use Case:** Support library evolution and versioning

```cpp
#include <iostream>
using namespace std;

// WHY: Version 1 of the library
namespace MyLib {
    namespace v1 {
        void process() {
            cout << "Processing with v1 algorithm" << endl;
        }
    }

    // WHY: v2 is inline - default version
    inline namespace v2 {
        void process() {
            cout << "Processing with v2 algorithm (default)" << endl;
        }
    }
}

int main() {
    // WHY: Calls v2 (inline) by default
    MyLib::process();      // Uses v2

    // WHY: Can explicitly use v1 if needed
    MyLib::v1::process();  // Uses v1

    // WHY: Can explicitly use v2
    MyLib::v2::process();  // Uses v2

    return 0;
}
```

**Output:**

```
Processing with v2 algorithm (default)
Processing with v1 algorithm
Processing with v2 algorithm (default)
```

### Practical Example: API Versioning

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Graphics library with multiple versions
namespace Graphics {
    // Old version - not inline
    namespace v1 {
        void render(const string& obj) {
            cout << "[v1] Simple render: " << obj << endl;
        }
    }

    // Current version - inline (default)
    inline namespace v2 {
        void render(const string& obj) {
            cout << "[v2] Advanced render with shadows: " << obj << endl;
        }

        void renderWithReflections(const string& obj) {
            cout << "[v2] Ultra render: " << obj << endl;
        }
    }
}

int main() {
    // WHY: Uses v2 by default (inline)
    Graphics::render("Sphere");
    Graphics::renderWithReflections("Cube");

    // WHY: Legacy code can still use v1
    Graphics::v1::render("Sphere");

    return 0;
}
```

**Output:**

```
[v2] Advanced render with shadows: Sphere
[v2] Ultra render: Cube
[v1] Simple render: Sphere
```

### Why Inline Namespaces?

**Benefits:**

1. **Avoid Verbose Code**: Don't need long namespace paths
2. **Default Version**: Inline namespace becomes the default
3. **Backward Compatibility**: Old versions still accessible
4. **Smooth Migration**: Users can gradually migrate to new API

**Real-World Example:**

```cpp
// WHY: Standard library uses this for ABI versioning
namespace std {
    inline namespace __1 {  // libc++ uses this
        // Current implementation
    }
}
```

---

## 7. Anonymous Namespaces

### What are Anonymous Namespaces?

**Anonymous (unnamed) namespaces** provide **internal linkage** - members are only visible in the current translation unit (source file).

**Think of it as:** `static` at namespace level.

### Basic Anonymous Namespace

```cpp
#include <iostream>
using namespace std;

// WHY: Anonymous namespace - internal linkage
namespace {
    int internalValue = 300;

    void internalFunction() {
        cout << "This function is file-local" << endl;
    }
}

int main() {
    // WHY: Can access directly (no namespace name needed)
    cout << "internalValue = " << internalValue << endl;
    internalFunction();

    return 0;
}
```

**Output:**

```
internalValue = 300
This function is file-local
```

**Key Properties:**

1. Members have **internal linkage** (file-scoped)
2. Not accessible from other translation units
3. No name collision across files
4. Better than `static` keyword for consistency

### Anonymous Namespace vs Static

**Old Way (C-style):**

```cpp
// file1.cpp
static int helper = 100;  // WHY: Internal linkage

static void helperFunc() {
    // File-local function
}
```

**Modern Way (C++ style):**

```cpp
// file1.cpp
namespace {
    int helper = 100;  // WHY: Internal linkage, C++ style

    void helperFunc() {
        // File-local function
    }
}
```

### Why Anonymous Namespaces Better?

```cpp
// WHY: Anonymous namespace can contain classes and namespaces
namespace {
    class InternalClass {  // OK: File-local class
    public:
        void method() {}
    };

    namespace Nested {     // OK: File-local nested namespace
        int value = 10;
    }
}

// static class InternalClass {};  // ERROR: Can't use static with class
```

### Practical Example: Implementation Details

**header.h:**

```cpp
#ifndef HEADER_H
#define HEADER_H

// WHY: Public interface
void publicFunction();

#endif
```

**implementation.cpp:**

```cpp
#include "header.h"
#include <iostream>
using namespace std;

// WHY: Hide implementation details
namespace {
    // Helper function - not exposed to other files
    int calculateInternal(int x) {
        return x * x + 10;
    }

    // Internal constant - file-scoped
    const double PI = 3.14159;

    // Internal class - file-scoped
    class InternalHelper {
    public:
        void assist() {
            cout << "Internal helper" << endl;
        }
    };
}

// WHY: Public function implementation
void publicFunction() {
    cout << "Public function using internal: ";
    cout << calculateInternal(5) << endl;

    InternalHelper helper;
    helper.assist();
}
```

**main.cpp:**

```cpp
#include "header.h"

int main() {
    publicFunction();  // OK: Public function

    // calculateInternal(5);  // ERROR: Not accessible
    // InternalHelper h;       // ERROR: Not accessible

    return 0;
}
```

### Multiple Files Example

**file1.cpp:**

```cpp
#include <iostream>
using namespace std;

namespace {
    int counter = 0;  // WHY: file1.cpp's private counter

    void increment() {
        counter++;
    }
}

void file1Function() {
    increment();
    cout << "file1 counter: " << counter << endl;
}
```

**file2.cpp:**

```cpp
#include <iostream>
using namespace std;

namespace {
    int counter = 0;  // WHY: file2.cpp's private counter (different!)

    void increment() {
        counter++;
    }
}

void file2Function() {
    increment();
    cout << "file2 counter: " << counter << endl;
}
```

**main.cpp:**

```cpp
void file1Function();
void file2Function();

int main() {
    file1Function();  // file1's counter
    file1Function();
    file2Function();  // file2's counter (separate!)
    file2Function();

    return 0;
}
```

**Output:**

```
file1 counter: 1
file1 counter: 2
file2 counter: 1
file2 counter: 2
```

**Explanation:** Each file has its own `counter` - no collision!

---

## 8. Namespace Aliases

### What are Namespace Aliases?

**Namespace aliases** create shorter names for long namespace paths.

**Syntax:**

```cpp
namespace alias_name = long::nested::namespace::path;
```

### Basic Namespace Alias

```cpp
#include <iostream>
using namespace std;

namespace VeryLongCompanyName {
    namespace Project {
        namespace Module {
            int value = 42;

            void display() {
                cout << "Very long namespace path" << endl;
            }
        }
    }
}

int main() {
    // WHY: Without alias - too verbose!
    VeryLongCompanyName::Project::Module::display();
    cout << VeryLongCompanyName::Project::Module::value << endl;

    // WHY: With alias - much cleaner!
    namespace VM = VeryLongCompanyName::Project::Module;

    VM::display();
    cout << VM::value << endl;

    return 0;
}
```

**Output:**

```
Very long namespace path
42
Very long namespace path
42
```

### Practical Example: Graphics API

```cpp
#include <iostream>
using namespace std;

namespace Graphics {
    namespace Rendering {
        namespace Engine {
            void initialize() {
                cout << "Engine initialized" << endl;
            }

            void render() {
                cout << "Rendering..." << endl;
            }
        }
    }
}

int main() {
    // WHY: Alias for convenience
    namespace GRE = Graphics::Rendering::Engine;

    GRE::initialize();
    GRE::render();
    GRE::render();

    return 0;
}
```

**Output:**

```
Engine initialized
Rendering...
Rendering...
```

### Multiple Aliases

```cpp
#include <iostream>
using namespace std;

namespace Company::Product::Version1 {
    void run() { cout << "Version 1" << endl; }
}

namespace Company::Product::Version2 {
    void run() { cout << "Version 2" << endl; }
}

int main() {
    // WHY: Create aliases for different versions
    namespace V1 = Company::Product::Version1;
    namespace V2 = Company::Product::Version2;

    V1::run();
    V2::run();

    return 0;
}
```

**Output:**

```
Version 1
Version 2
```

### Use Case: Library Migration

```cpp
#include <iostream>
#include <string>
using namespace std;

// Old library namespace
namespace OldGraphics {
    void render(const string& obj) {
        cout << "[Old] Rendering: " << obj << endl;
    }
}

// New library namespace
namespace NewGraphics {
    void render(const string& obj) {
        cout << "[New] Rendering: " << obj << endl;
    }
}

int main() {
    // WHY: Use alias to easily switch between versions
    // Change this one line to switch libraries!
    namespace Graphics = NewGraphics;  // Try: = OldGraphics

    Graphics::render("Sphere");
    Graphics::render("Cube");
    Graphics::render("Cylinder");

    return 0;
}
```

**Output:**

```
[New] Rendering: Sphere
[New] Rendering: Cube
[New] Rendering: Cylinder
```

**Benefits:**

- Change one line to switch library
- Easy A/B testing
- Gradual migration strategy

---

## 9. Global Namespace

### What is the Global Namespace?

**Global namespace** is the default namespace where all entities that are not explicitly in any namespace reside.

**Access:** Use `::` without any namespace name.

### Global vs Named Namespace

```cpp
#include <iostream>
using namespace std;

// WHY: Global namespace (no namespace declaration)
int globalValue = 100;

void globalFunction() {
    cout << "Global function" << endl;
}

// WHY: Named namespace
namespace MySpace {
    int value = 200;

    void function() {
        cout << "MySpace function" << endl;
    }
}

int main() {
    // WHY: Access global namespace explicitly
    cout << "::globalValue = " << ::globalValue << endl;
    ::globalFunction();

    // WHY: Access named namespace
    cout << "MySpace::value = " << MySpace::value << endl;
    MySpace::function();

    return 0;
}
```

**Output:**

```
::globalValue = 100
Global function
MySpace::value = 200
MySpace function
```

### Resolving Name Conflicts

```cpp
#include <iostream>
using namespace std;

int value = 10;  // WHY: Global namespace

namespace MySpace {
    int value = 20;  // WHY: MySpace namespace
}

int main() {
    int value = 30;  // WHY: Local scope

    // WHY: Access all three different 'value' variables
    cout << "Local value: " << value << endl;        // Local (30)
    cout << "Global value: " << ::value << endl;     // Global (10)
    cout << "MySpace value: " << MySpace::value << endl; // MySpace (20)

    return 0;
}
```

**Output:**

```
Local value: 30
Global value: 10
MySpace value: 20
```

### Practical Example: Overriding Library Functions

```cpp
#include <iostream>
using namespace std;

namespace Math {
    int abs(int x) {
        cout << "[Custom abs] ";
        return (x < 0) ? -x : x;
    }
}

int main() {
    int value = -42;

    // WHY: Use custom abs
    cout << "Custom: " << Math::abs(value) << endl;

    // WHY: Use global (standard) abs
    cout << "Standard: " << ::abs(value) << endl;

    return 0;
}
```

**Output:**

```
[Custom abs] Custom: 42
Standard: 42
```

---

## 10. ADL (Argument-Dependent Lookup)

### What is ADL?

**Argument-Dependent Lookup (ADL)** automatically searches for functions in the namespaces of their arguments.

**Also known as:** Koenig Lookup

**WHY:** Makes code more natural and avoids explicit namespace qualification.

### How ADL Works

```cpp
#include <iostream>

namespace MyNamespace {
    class MyClass {
        int value;
    public:
        MyClass(int v) : value(v) {}
        int getValue() const { return value; }
    };

    // WHY: Free function in same namespace as MyClass
    void display(const MyClass& obj) {
        std::cout << "Value: " << obj.getValue() << std::endl;
    }
}

int main() {
    MyNamespace::MyClass obj(42);

    // WHY: ADL finds display() in MyNamespace automatically!
    display(obj);  // No need for MyNamespace::display()

    return 0;
}
```

**Output:**

```
Value: 42
```

**Explanation:** The compiler looks in `MyNamespace` because `obj` is of type `MyNamespace::MyClass`.

### ADL Example: Operator Overloading

```cpp
#include <iostream>

namespace Math {
    class Complex {
        double real, imag;
    public:
        Complex(double r, double i) : real(r), imag(i) {}

        double getReal() const { return real; }
        double getImag() const { return imag; }
    };

    // WHY: Operator in same namespace as Complex
    std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << c.getReal() << " + " << c.getImag() << "i";
        return os;
    }

    Complex operator+(const Complex& a, const Complex& b) {
        return Complex(a.getReal() + b.getReal(),
                      a.getImag() + b.getImag());
    }
}

int main() {
    Math::Complex c1(3, 4);
    Math::Complex c2(1, 2);

    // WHY: ADL finds operators in Math namespace!
    std::cout << "c1 = " << c1 << std::endl;  // Finds operator<<
    std::cout << "c2 = " << c2 << std::endl;

    Math::Complex c3 = c1 + c2;  // Finds operator+
    std::cout << "c1 + c2 = " << c3 << std::endl;

    return 0;
}
```

**Output:**

```
c1 = 3 + 4i
c2 = 1 + 2i
c1 + c2 = 4 + 6i
```

### Without ADL (Bad)

```cpp
namespace Math {
    class Number {
        int value;
    public:
        Number(int v) : value(v) {}
        int get() const { return value; }
    };
}

// WHY: Function in global namespace - ADL won't find it!
void print(const Math::Number& n) {
    std::cout << "Value: " << n.get() << std::endl;
}

int main() {
    Math::Number num(42);

    // Must explicitly call
    print(num);  // Works, but not natural

    // Math::print(num);  // ERROR: print not in Math namespace

    return 0;
}
```

### std Namespace and ADL

```cpp
#include <iostream>
#include <string>

int main() {
    std::string str = "Hello";

    // WHY: ADL looks in std namespace for operator<<
    std::cout << str << std::endl;  // Finds std::operator<<

    // Without ADL, we'd need:
    // std::operator<<(std::cout, str);  // Ugly!

    return 0;
}
```

### When ADL Doesn't Help

```cpp
#include <iostream>

namespace MyLib {
    int value = 100;
}

int main() {
    // value;  // ERROR: No ADL without function call or operator

    // Must use explicit namespace
    std::cout << MyLib::value << std::endl;

    return 0;
}
```

**Note:** ADL only works for function calls and operators, not for variable access.

---

## 11. Extending Namespaces

### Namespace Extension

**Namespaces can be defined in multiple parts** - useful for organizing large codebases.

```cpp
#include <iostream>
using namespace std;

// WHY: First part of MyLib namespace
namespace MyLib {
    void function1() {
        cout << "Function 1" << endl;
    }
}

// WHY: Extend MyLib namespace (add more members)
namespace MyLib {
    void function2() {
        cout << "Function 2" << endl;
    }
}

// WHY: Further extend MyLib namespace
namespace MyLib {
    void function3() {
        cout << "Function 3" << endl;
    }
}

int main() {
    // WHY: All functions are in the same MyLib namespace
    MyLib::function1();
    MyLib::function2();
    MyLib::function3();

    return 0;
}
```

**Output:**

```
Function 1
Function 2
Function 3
```

### Multi-File Namespace Extension

**header1.h:**

```cpp
#ifndef HEADER1_H
#define HEADER1_H

#include <iostream>

namespace Graphics {
    void drawCircle();
}

#endif
```

**source1.cpp:**

```cpp
#include "header1.h"

namespace Graphics {
    void drawCircle() {
        std::cout << "Drawing circle" << std::endl;
    }
}
```

**header2.h:**

```cpp
#ifndef HEADER2_H
#define HEADER2_H

#include <iostream>

// WHY: Extend Graphics namespace in different file
namespace Graphics {
    void drawSquare();
}

#endif
```

**source2.cpp:**

```cpp
#include "header2.h"

namespace Graphics {
    void drawSquare() {
        std::cout << "Drawing square" << std::endl;
    }
}
```

**main.cpp:**

```cpp
#include "header1.h"
#include "header2.h"

int main() {
    // WHY: Both functions are in Graphics namespace
    Graphics::drawCircle();
    Graphics::drawSquare();

    return 0;
}
```

### Practical Use Case

```cpp
// math_basic.cpp
namespace Math {
    double add(double a, double b) {
        return a + b;
    }
}

// math_advanced.cpp
namespace Math {  // WHY: Extend Math namespace
    double power(double base, int exp) {
        double result = 1;
        for(int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// math_trigonometry.cpp
namespace Math {  // WHY: Further extend Math namespace
    const double PI = 3.14159;

    double sin(double x) {
        // Implementation
        return 0;  // Simplified
    }
}
```

---

## 12. Classes in Namespaces

### Defining Classes in Namespaces

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Group related classes in namespace
namespace Geometry {
    class Point {
        double x, y;
    public:
        Point(double x_val, double y_val) : x(x_val), y(y_val) {}

        void display() const {
            cout << "(" << x << ", " << y << ")" << endl;
        }

        double getX() const { return x; }
        double getY() const { return y; }
    };

    class Circle {
        Point center;
        double radius;
    public:
        Circle(Point c, double r) : center(c), radius(r) {}

        void display() const {
            cout << "Circle at ";
            center.display();
            cout << "with radius " << radius << endl;
        }
    };
}

int main() {
    // WHY: Use classes from namespace
    Geometry::Point p(3, 4);
    Geometry::Circle c(p, 5);

    p.display();
    c.display();

    return 0;
}
```

**Output:**

```
(3, 4)
Circle at (3, 4)
with radius 5
```

### Class Declaration and Definition Separation

**geometry.h:**

```cpp
#ifndef GEOMETRY_H
#define GEOMETRY_H

namespace Geometry {
    class Rectangle {
        double width, height;
    public:
        // WHY: Declaration in header
        Rectangle(double w, double h);
        double area() const;
        double perimeter() const;
        void display() const;
    };
}

#endif
```

**geometry.cpp:**

```cpp
#include "geometry.h"
#include <iostream>
using namespace std;

// WHY: Definition outside namespace, using namespace::Class::
Geometry::Rectangle::Rectangle(double w, double h)
    : width(w), height(h) {
}

double Geometry::Rectangle::area() const {
    return width * height;
}

double Geometry::Rectangle::perimeter() const {
    return 2 * (width + height);
}

void Geometry::Rectangle::display() const {
    cout << "Rectangle: " << width << " x " << height << endl;
    cout << "Area: " << area() << ", Perimeter: " << perimeter() << endl;
}
```

**Alternative: Define inside namespace block**

```cpp
#include "geometry.h"
#include <iostream>
using namespace std;

// WHY: Definition inside namespace block
namespace Geometry {
    Rectangle::Rectangle(double w, double h)
        : width(w), height(h) {
    }

    double Rectangle::area() const {
        return width * height;
    }

    double Rectangle::perimeter() const {
        return 2 * (width + height);
    }

    void Rectangle::display() const {
        cout << "Rectangle: " << width << " x " << height << endl;
        cout << "Area: " << area() << ", Perimeter: " << perimeter() << endl;
    }
}
```

### Nested Classes in Namespaces

```cpp
#include <iostream>
#include <string>
using namespace std;

namespace Company {
    class Employee {
    public:
        // WHY: Nested class for Address
        class Address {
            string street, city;
        public:
            Address(string st, string ct) : street(st), city(ct) {}

            void display() const {
                cout << street << ", " << city << endl;
            }
        };

    private:
        string name;
        Address address;

    public:
        Employee(string n, string street, string city)
            : name(n), address(street, city) {}

        void display() const {
            cout << "Employee: " << name << endl;
            cout << "Address: ";
            address.display();
        }
    };
}

int main() {
    // WHY: Create employee with nested Address
    Company::Employee emp("John Doe", "123 Main St", "New York");
    emp.display();

    return 0;
}
```

**Output:**

```
Employee: John Doe
Address: 123 Main St, New York
```

---

## 13. Best Practices

### ✅ DO: Use Namespaces in Headers

```cpp
// mylib.h
#ifndef MYLIB_H
#define MYLIB_H

// WHY: Always wrap library code in namespace
namespace MyLib {
    void function();

    class MyClass {
        // ...
    };
}

#endif
```

### ❌ DON'T: using namespace in Headers

```cpp
// bad_header.h
#ifndef BAD_HEADER_H
#define BAD_HEADER_H

#include <iostream>
using namespace std;  // BAD: Pollutes all including files!

namespace MyLib {
    void function();
}

#endif
```

### ✅ DO: using in Implementation Files

```cpp
// mylib.cpp
#include "mylib.h"
#include <iostream>

// WHY: OK in .cpp files (limited scope)
using namespace std;

namespace MyLib {
    void function() {
        cout << "Function" << endl;  // std:: not needed
    }
}
```

### ✅ DO: Use Selective Imports

```cpp
#include <iostream>
#include <vector>
#include <string>

// WHY: Import only what you need
using std::cout;
using std::endl;
using std::vector;
using std::string;

int main() {
    vector<string> names = {"Alice", "Bob"};

    for(const string& name : names) {
        cout << name << endl;
    }

    return 0;
}
```

### ✅ DO: Use Namespace Aliases for Long Paths

```cpp
#include <iostream>

namespace Company::Product::Version2::Graphics {
    void render() {
        std::cout << "Rendering..." << std::endl;
    }
}

int main() {
    // WHY: Alias for convenience
    namespace Gfx = Company::Product::Version2::Graphics;

    Gfx::render();
    Gfx::render();

    return 0;
}
```

### ✅ DO: Use Anonymous Namespaces for Internal Helpers

```cpp
// implementation.cpp
#include <iostream>

namespace {
    // WHY: Internal helper - file-local
    int helperFunction(int x) {
        return x * x;
    }
}

void publicFunction() {
    std::cout << helperFunction(5) << std::endl;
}
```

### ❌ DON'T: Put using namespace in Global Scope

```cpp
// BAD
using namespace std;  // Pollutes global scope

int main() {
    // ...
}
```

```cpp
// GOOD
int main() {
    using namespace std;  // Limited to main() scope

    cout << "Hello" << endl;

    return 0;
}
```

### ✅ DO: Organize by Functionality

```cpp
namespace Project {
    namespace Core {
        // Core functionality
    }

    namespace UI {
        // User interface
    }

    namespace Database {
        // Database operations
    }

    namespace Utils {
        // Utility functions
    }
}
```

---

## 14. Common Pitfalls

### Pitfall 1: using namespace in Headers

```cpp
// header.h - BAD!
#ifndef HEADER_H
#define HEADER_H

#include <iostream>
using namespace std;  // ❌ Affects all files including this header!

void function();

#endif
```

**Problem:** Every file that includes this header gets `using namespace std` forced on them.

**Solution:** Never use `using namespace` in headers!

### Pitfall 2: Name Collisions

```cpp
#include <iostream>
#include <algorithm>  // Has std::count
using namespace std;

namespace MyLib {
    int count = 0;  // Conflicts with std::count
}

int main() {
    using namespace MyLib;

    // cout << count << endl;  // ❌ ERROR: Ambiguous!

    // Must be explicit:
    cout << MyLib::count << endl;

    return 0;
}
```

### Pitfall 3: Forgetting Namespace in Implementation

```cpp
// header.h
namespace MyLib {
    class MyClass {
    public:
        void method();
    };
}

// implementation.cpp - BAD!
#include "header.h"

void MyClass::method() {  // ❌ Creates ::MyClass, not MyLib::MyClass!
    // ...
}

// CORRECT:
namespace MyLib {
    void MyClass::method() {  // ✅ Correct namespace
        // ...
    }
}
```

### Pitfall 4: ADL Unexpected Behavior

```cpp
namespace A {
    struct X {};
    void func(X) {}
}

namespace B {
    void func(A::X) {}

    void test() {
        A::X x;
        func(x);  // Calls which func? A::func or B::func?
    }
}
```

**Solution:** Be aware of ADL when designing APIs.

---

## 15. Summary

### Key Takeaways

1. **Namespaces Prevent Collisions**
    - Group related code
    - Avoid name conflicts
    - Essential for libraries
2. **Access Methods**
    - Scope resolution: `Namespace::member`
    - using declaration: `using NS::member;`
    - using directive: `using namespace NS;`
3. **Advanced Features**
    - Nested namespaces: `A::B::C`
    - Inline namespaces: For versioning
    - Anonymous namespaces: For internal linkage
    - Namespace aliases: For convenience
4. **Best Practices**
    - ✅ Always use namespaces in libraries
    - ✅ Use selective imports (`using NS::name;`)
    - ✅ Use anonymous namespaces for helpers
    - ❌ Never `using namespace` in headers
    - ❌ Avoid `using namespace std;` globally
5. **Real-World Usage**
    - Standard Library: `std::`
    - Project organization: `Company::Project::Module`
    - API versioning: inline namespaces
    - File-local code: anonymous namespaces

### Quick Reference

| Feature | Syntax | Use Case |
| --- | --- | --- |
| **Define** | `namespace Name { }` | Group code |
| **Access** | `Name::member` | Explicit access |
| **using declaration** | `using Name::member;` | Import specific |
| **using directive** | `using namespace Name;` | Import all (careful!) |
| **Nested** | `namespace A::B::C { }` | Hierarchy (C++17) |
| **Inline** | `inline namespace Name { }` | Versioning |
| **Anonymous** | `namespace { }` | File-local |
| **Alias** | `namespace Short = Long::Path;` | Convenience |
| **Global** | `::member` | Global scope |

### Keywords Covered

✅ namespace keyword (15)
✅ using declaration (2)
✅ using directive (2)
✅ Nested namespaces (3)
✅ Inline namespaces (C++11) (2)
✅ Anonymous namespaces (2)
✅ Namespace aliases (2)
✅ ADL (Argument-Dependent Lookup) (1)
✅ Global namespace (1)
✅ Scope resolution operator (2)
✅ Extending namespaces (1)

**Total: 33 keywords/concepts covered**

---