# 1.4. Operators, Input/Output, Control Flow & Loops

---

## Table of Contents

1. Operators
2. Input/Output (I/O)
3. Control Flow Statements
4. Loops

---

## 1. Operators

**Definition:** Symbols that perform operations on operands (variables/values).

### 1.1 Arithmetic Operators

**Purpose:** Perform mathematical calculations.

| Operator | Name | Example | Result |
| --- | --- | --- | --- |
| `+` | Addition | `5 + 3` | `8` |
| `-` | Subtraction | `5 - 3` | `2` |
| `*` | Multiplication | `5 * 3` | `15` |
| `/` | Division | `5 / 2` | `2` (integer division) |
| `%` | Modulus (remainder) | `5 % 2` | `1` |

```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 10, b = 3;

    cout << "Addition: " << (a + b) << endl;        // 13
    cout << "Subtraction: " << (a - b) << endl;     // 7
    cout << "Multiplication: " << (a * b) << endl;  // 30
    cout << "Division: " << (a / b) << endl;        // 3 (integer division truncates)
    cout << "Modulus: " << (a % b) << endl;         // 1 (remainder of 10/3)

    // Floating-point division
    cout << "Float Division: " << (10.0 / 3) << endl;  // 3.33333

    return 0;
}
```

**Key Point:** Division of integers truncates decimal part. Use `double` for precise division.

### 1.2 Relational (Comparison) Operators

**Purpose:** Compare two values, return `true` (1) or `false` (0).

| Operator | Meaning | Example | Result |
| --- | --- | --- | --- |
| `==` | Equal to | `5 == 5` | `true` |
| `!=` | Not equal | `5 != 3` | `true` |
| `>` | Greater than | `5 > 3` | `true` |
| `<` | Less than | `5 < 3` | `false` |
| `>=` | Greater or equal | `5 >= 5` | `true` |
| `<=` | Less or equal | `5 <= 3` | `false` |

```cpp
int x = 10, y = 20;

// Common mistake: = vs ==
if (x = y) {  // âŒ Assignment (always true if y != 0)
    cout << "Wrong!";
}

if (x == y) {  // âœ… Comparison
    cout << "Equal";
}
```

### 1.3 Logical Operators

**Purpose:** Combine boolean expressions.

| Operator | Name | Meaning | Example | Result |
| --- | --- | --- | --- | --- |
| `&&` | AND | Both true | `true && false` | `false` |
| `||` | OR | At least one true | `true || false` | `true` |
| `!` | NOT | Negation | `!true` | `false` |

```cpp
int age = 25;
bool hasLicense = true;

// Short-circuit evaluation (efficient!)
if (age >= 18 && hasLicense) {  // Both must be true
    cout << "Can drive";
}

if (age < 18 || !hasLicense) {  // At least one true
    cout << "Cannot drive";
}
```

**Short-circuit evaluation:**

- `&&`: If left is `false`, right is NOT evaluated
- `||`: If left is `true`, right is NOT evaluated

### 1.4 Bitwise Operators

**Purpose:** Operate on individual bits (low-level operations).

| Operator | Name | Example (Binary) | Result |
| --- | --- | --- | --- |
| `&` | AND | `6 & 3` â†’ `110 & 011` | `010` (2) |
| `|` | OR | `6 | 3` â†’ `110 | 011` | `111` (7) |
| `^` | XOR | `6 ^ 3` â†’ `110 ^ 011` | `101` (5) |
| `~` | NOT | `~6` â†’ `~0110` | `1001` (-7 in 2's complement) |
| `<<` | Left shift | `3 << 2` â†’ `11 << 2` | `1100` (12) |
| `>>` | Right shift | `12 >> 2` â†’ `1100 >> 2` | `11` (3) |

```cpp
int a = 6;  // Binary: 0110
int b = 3;  // Binary: 0011

cout << (a & b) << endl;   // 2  (0010) - bits set in both
cout << (a | b) << endl;   // 7  (0111) - bits set in either
cout << (a ^ b) << endl;   // 5  (0101) - bits different
cout << (~a) << endl;      // -7 (flip all bits)
cout << (a << 1) << endl;  // 12 (multiply by 2)
cout << (a >> 1) << endl;  // 3  (divide by 2)
```

**Use cases:**

- Flags and permissions
- Efficient multiplication/division by powers of 2
- Low-level hardware programming

### 1.5 Assignment Operators

| Operator | Example | Equivalent |
| --- | --- | --- |
| `=` | `x = 5` | Assign 5 to x |
| `+=` | `x += 3` | `x = x + 3` |
| `-=` | `x -= 3` | `x = x - 3` |
| `*=` | `x *= 3` | `x = x * 3` |
| `/=` | `x /= 3` | `x = x / 3` |
| `%=` | `x %= 3` | `x = x % 3` |
| `&=` | `x &= 3` | `x = x & 3` |
| `|=` | `x |= 3` | `x = x | 3` |
| `^=` | `x ^= 3` | `x = x ^ 3` |
| `<<=` | `x <<= 2` | `x = x << 2` |
| `>>=` | `x >>= 2` | `x = x >> 2` |

### 1.6 Increment/Decrement Operators

```cpp
int x = 5;

// Pre-increment: Increment THEN use value
int a = ++x;  // x becomes 6, a = 6

// Post-increment: Use value THEN increment
int b = x++;  // b = 6, x becomes 7

// Pre-decrement: Decrement THEN use value
int c = --x;  // x becomes 6, c = 6

// Post-decrement: Use value THEN decrement
int d = x--;  // d = 6, x becomes 5
```

**Use case:** Prefer `++i` in loops (slightly faster, no temporary).

### 1.7 Ternary Operator

**Purpose:** Compact if-else for simple conditions.

**Syntax:**

```cpp
condition ? value_if_true : value_if_false;
```

```cpp
int age = 20;
string status = (age >= 18) ? "Adult" : "Minor";  // "Adult"

// Equivalent to:
string status;
if (age >= 18) {
    status = "Adult";
} else {
    status = "Minor";
}
```

### 1.8 sizeof Operator

```cpp
cout << sizeof(int) << endl;        // 4 (bytes)
cout << sizeof(double) << endl;     // 8
int arr[10];
cout << sizeof(arr) << endl;        // 40 (10 * 4)
cout << sizeof(arr)/sizeof(arr[0]); // 10 (array size)
```

### 1.9 Comma Operator

```cpp
int a, b;
a = (b = 3, b + 2);  // b = 3, then a = 5 (last expression)

// Common use: Multiple expressions in for loop
for (int i = 0, j = 10; i < j; i++, j--) {
    cout << i << " " << j << endl;
}
```

### 1.10 Operator Precedence & Associativity

**Precedence** (high to low):

| Level | Operators | Associativity |
| --- | --- | --- |
| 1 | `::` | Left-to-right |
| 2 | `++ --` (postfix), `()` (function call), `[]` | Left-to-right |
| 3 | `++ --` (prefix), `!`, `~`, `+` `-` (unary), `sizeof` | Right-to-left |
| 4 | `*` `/` `%` | Left-to-right |
| 5 | `+` `-` | Left-to-right |
| 6 | `<<` `>>` | Left-to-right |
| 7 | `<` `<=` `>` `>=` | Left-to-right |
| 8 | `==` `!=` | Left-to-right |
| 9 | `&` | Left-to-right |
| 10 | `^` | Left-to-right |
| 11 | `|` | Left-to-right |
| 12 | `&&` | Left-to-right |
| 13 | `||` | Left-to-right |
| 14 | `?:` | Right-to-left |
| 15 | `=` `+=` `-=` etc. | Right-to-left |

**Use parentheses for clarity!**

---

## 2. Input/Output (I/O)

### 2.1 Standard Streams

| Stream | Purpose | Buffered |
| --- | --- | --- |
| `cin` | Standard input (keyboard) | Yes |
| `cout` | Standard output (screen) | Yes |
| `cerr` | Error output (immediate) | No (unbuffered) |
| `clog` | Log output | Yes |

```cpp
#include <iostream>
using namespace std;

int main() {
    int age;

    cout << "Enter age: ";  // Output prompt
    cin >> age;             // Read input

    if (age < 0) {
        cerr << "Error: Invalid age!" << endl;  // Immediate error display
    }

    clog << "Log: User entered " << age << endl;  // Log message

    return 0;
}
```

### 2.2 cin Methods

```cpp
#include <iostream>
#include <limits>
using namespace std;

int main() {
    char ch;
    char buffer[100];
    string name;

    // cin.get() - Read single character (including whitespace)
    cin.get(ch);

    // cin.getline() - Read entire line
    cin.getline(buffer, 100);  // C-style string

    // getline() - Read line into std::string
    getline(cin, name);

    // cin.ignore() - Skip characters (clear buffer)
    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Clear entire line

    return 0;
}
```

### 2.3 Stream Manipulators

**Include:** `<iomanip>`

```cpp
#include <iostream>
#include <iomanip>
using namespace std;

int main() {
    double pi = 3.14159265359;
    int num = 42;

    // Output formatting
    cout << setw(10) << num << endl;           // Width 10
    cout << setfill('*') << setw(10) << num << endl;  // Fill with *
    cout << setprecision(3) << pi << endl;     // 3 significant digits
    cout << fixed << setprecision(2) << pi << endl;   // 2 decimal places
    cout << scientific << pi << endl;          // Scientific notation

    // Number base
    cout << hex << num << endl;  // Hexadecimal
    cout << oct << num << endl;  // Octal
    cout << dec << num << endl;  // Decimal (default)

    // Alignment
    cout << left << setw(10) << num << endl;   // Left-aligned
    cout << right << setw(10) << num << endl;  // Right-aligned

    // Boolean display
    cout << boolalpha << true << endl;  // "true" instead of 1

    return 0;
}
```

---

## 3. Control Flow Statements

### 3.1 if Statement

```cpp
int score = 85;

if (score >= 90) {
    cout << "Grade: A" << endl;
}
```

### 3.2 if-else Statement

```cpp
if (score >= 60) {
    cout << "Pass" << endl;
} else {
    cout << "Fail" << endl;
}
```

### 3.3 if-else-if Ladder

```cpp
if (score >= 90) {
    cout << "A";
} else if (score >= 80) {
    cout << "B";
} else if (score >= 70) {
    cout << "C";
} else if (score >= 60) {
    cout << "D";
} else {
    cout << "F";
}
```

### 3.4 Nested if

```cpp
int age = 25;
bool hasID = true;

if (age >= 21) {
    if (hasID) {
        cout << "Entry allowed" << endl;
    } else {
        cout << "ID required" << endl;
    }
} else {
    cout << "Too young" << endl;
}
```

### 3.5 switch Statement

**Purpose:** Multi-way branch based on integer/char value.

```cpp
#include <iostream>
using namespace std;

int main() {
    int day = 3;

    switch (day) {
        case 1:
            cout << "Monday" << endl;
            break;  // IMPORTANT: Prevents fall-through
        case 2:
            cout << "Tuesday" << endl;
            break;
        case 3:
            cout << "Wednesday" << endl;
            break;
        case 4:
            cout << "Thursday" << endl;
            break;
        case 5:
            cout << "Friday" << endl;
            break;
        case 6:
        case 7:  // Fall-through for weekend
            cout << "Weekend" << endl;
            break;
        default:  // Optional: When no case matches
            cout << "Invalid day" << endl;
    }

    return 0;
}
```

**Fall-through behavior (intentional):**

```cpp
char grade = 'B';

switch (grade) {
    case 'A':
    case 'B':  // Fall-through
    case 'C':  // Fall-through
        cout << "Pass" << endl;
        break;
    case 'D':
    case 'F':
        cout << "Fail" << endl;
        break;
}
```

**Limitations:**

- Only works with integral types (`int`, `char`, `enum`)
- Cannot use ranges or variables in cases

---

## 4. Loops

### 4.1 for Loop

**Syntax:**

```cpp
for (initialization; condition; update) {
    // Body
}
```

```cpp
// Print 1 to 5
for (int i = 1; i <= 5; i++) {
    cout << i << " ";  // 1 2 3 4 5
}
```

**Nested for loop:**

```cpp
// Multiplication table
for (int i = 1; i <= 3; i++) {
    for (int j = 1; j <= 3; j++) {
        cout << i * j << " ";
    }
    cout << endl;
}
```

### 4.2 while Loop

**Entry-controlled:** Condition checked BEFORE execution.

```cpp
int i = 1;

while (i <= 5) {
    cout << i << " ";
    i++;  // Must update loop variable!
}

```

### 4.3 do-while Loop

**Exit-controlled:** Executes at LEAST ONCE.

```cpp
int i = 1;

do {
    cout << i << " ";
    i++;
} while (i <= 5);  // Semicolon required!
```

**Key difference:**

```cpp
int x = 10;

while (x < 5) {
    cout << "Never prints";  // Condition false, not executed
}

do {
    cout << "Prints once!";  // Executes before checking
} while (x < 5);
```

### 4.4 Range-Based for Loop (C++11)

**Purpose:** Iterate over containers/arrays.

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    int arr[] = {10, 20, 30, 40};

    // By value (read-only)
    for (int x : arr) {
        cout << x << " ";  // Cannot modify array
    }

    // By reference (can modify)
    for (int& x : arr) {
        x *= 2;  // Doubles each element
    }

    // auto keyword (type deduction)
    for (auto x : arr) {
        cout << x << " ";
    }

    // Works with STL containers
    vector<string> names = {"Alice", "Bob", "Charlie"};
    for (const auto& name : names) {  // const reference for efficiency
        cout << name << endl;
    }

    return 0;
}
```

### 4.5 Loop Control: break

**Purpose:** Exit loop immediately.

```cpp
for (int i = 1; i <= 10; i++) {
    if (i == 5) {
        break;  // Loop ends when i is 5
    }
    cout << i << " ";  // Prints: 1 2 3 4
}
```

### 4.6 Loop Control: continue

**Purpose:** Skip current iteration, proceed to next.

```cpp
for (int i = 1; i <= 5; i++) {
    if (i == 3) {
        continue;  // Skip when i is 3
    }
    cout << i << " ";  // Prints: 1 2 4 5
}
```

### 4.7 Loop Control: goto (Avoid!)

```cpp
int x = 0;

start:  // Label
    cout << x << " ";
    x++;
    if (x < 5) {
        goto start;  // Jump to label
    }
```

**Why avoid:**

- Makes code hard to read
- Can create "spaghetti code"
- Modern alternatives: `break`, `continue`, functions

### 4.8 Infinite Loops

```cpp
// Intentional infinite loop (use with break)
while (true) {
    int choice;
    cout << "Enter 0 to quit: ";
    cin >> choice;

    if (choice == 0) {
        break;  // Exit loop
    }
}

// Alternative
for (;;) {  // Empty for loop = infinite
    // Code here
}
```

### 4.9 Loop Comparison

| Loop Type | Entry/Exit Control | Min Executions | Use When |
| --- | --- | --- | --- |
| `for` | Entry-controlled | 0 | Iterations known |
| `while` | Entry-controlled | 0 | Iterations unknown, may skip |
| `do-while` | Exit-controlled | 1 | Must execute at least once |
| Range-for | Entry-controlled | 0 | Iterating over collections |

---

## Summary

### Key Takeaways

1. **Operators** - Foundation of computation
    - Arithmetic: `+ - * / %`
    - Relational: `== != < > <= >=`
    - Logical: `&& || !` (short-circuit evaluation)
    - Bitwise: `& | ^ ~ << >>` (low-level operations)
    - Precedence matters: Use `()` for clarity
2. **I/O Streams** - Communication with user
    - `cin` (input), `cout` (output), `cerr` (errors)
    - Manipulators: `setw`, `setprecision`, `fixed`
    - `getline()` for strings with spaces
3. **Control Flow** - Decision making
    - `if-else`: Binary decisions
    - `switch`: Multi-way branch (integers/chars only)
    - Fall-through: Intentional or bug?
4. **Loops** - Repetition
    - `for`: Known iterations
    - `while`: Entry-controlled, may skip
    - `do-while`: Guarantees one execution
    - Range-for (C++11): Clean iteration over containers
    - `break` (exit), `continue` (skip), `goto` (avoid)

### Essential Interview Points

**Operator Precedence:**

> "Multiplication/division before addition/subtraction. Logical AND before OR. Always use parentheses to clarify complex expressions and prevent bugs."
> 

**Short-Circuit Evaluation:**

> "Logical AND (&&) stops if left operand is false. OR (||) stops if left is true. This optimizes performance and prevents errors like null pointer dereference."
> 

**switch vs if-else:**

> "Switch is faster for many integer comparisons but limited to integral types. If-else handles any condition type but slower for many branches."
> 

**for vs while:**

> "Use for loop when iterations are known (counting). Use while when iterations depend on condition (user input, file reading)."
> 

**do-while Use Case:**

> "Perfect for menu-driven programs where you must display menu at least once before checking user input."
> 

**Range-based for:**

> "Use auto& to modify elements, const auto& for read-only (avoids copying large objects). C++11 feature for cleaner iteration."
>