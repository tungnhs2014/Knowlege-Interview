# 2.1. Function Basics

---

## Table of Contents

1. What are Functions?
2. Function Structure & Components
3. Function Declaration vs Definition
4. Function Signature vs Function Prototype
5. Return Types & Return Statement
6. void Functions
7. Function Calling Mechanism
8. Call Stack & Stack Frames
9. Function Scope & Lifetime

---

## 1. What are Functions?

### 1.1 Definition

**A function is a self-contained block of code designed to perform a specific, well-defined task.** Functions are the fundamental building blocks of modular programming in C++.

**Core Concept:**

```cpp
// WHY: Break complex problems into smaller, manageable pieces
int calculateTax(double income) {
    // Single responsibility: calculate tax only
    return income * 0.2;
}

int main() {
    double salary = 50000;
    double tax = calculateTax(salary);  // Reusable, testable, maintainable
    return 0;
}
```

### 1.2 Purpose & Benefits

**Why Use Functions?**

1. **Code Reusability**
    - Write once, use multiple times
    - Reduces code duplication
2. **Modularity**
    - Break large programs into logical units
    - Each function has single responsibility
3. **Maintainability**
    - Changes in one place affect entire program
    - Easier to debug and test
4. **Abstraction**
    - Hide implementation details
    - Focus on what function does, not how
5. **Team Collaboration**
    - Different developers work on different functions
    - Clear interfaces between modules

**Real-World Example:**

```cpp
#include <iostream>
#include <cmath>
using namespace std;

// WHY: Banking system needs to calculate loan payments repeatedly
double calculateMonthlyPayment(double principal, double annualRate, int years) {
    // Formula: M = P * [r(1+r)^n] / [(1+r)^n - 1]
    double monthlyRate = annualRate / 12 / 100;
    int months = years * 12;

    return principal * (monthlyRate * pow(1 + monthlyRate, months))
           / (pow(1 + monthlyRate, months) - 1);
}

int main() {
    // Reuse for multiple customers
    cout << "Loan 1: $" << calculateMonthlyPayment(200000, 5.5, 30) << endl;
    cout << "Loan 2: $" << calculateMonthlyPayment(150000, 4.8, 20) << endl;
    cout << "Loan 3: $" << calculateMonthlyPayment(300000, 6.2, 15) << endl;

    return 0;
}
```

**Output:**

```
Loan 1: $1135.58
Loan 2: $973.60
Loan 3: $2571.92
```

### 1.3 When to Use Functions

**✅ Create a Function When:**

- Task is performed more than once
- Code block has clear, single purpose
- Logic is complex and needs separation
- Need to test a specific operation independently
- Want to hide implementation details

**❌ Don't Create Function When:**

- Code is used only once and very simple (1-2 lines)
- Function would be called from only one place with no future reuse
- Creating function adds unnecessary complexity

---

## 2. Function Structure & Components

### 2.1 Anatomy of a Function

```cpp
// COMPLETE FUNCTION STRUCTURE
return_type function_name(parameter_list) {
    // Function body
    // Local variables
    // Statements
    return value;  // if return_type is not void
}
```

**Components Breakdown:**

```cpp
#include <iostream>
using namespace std;

// 1. Return Type: double - function returns decimal number
// 2. Function Name: calculateArea - descriptive, verb-based
// 3. Parameter List: (double length, double width) - inputs
double calculateArea(double length, double width) {
    // 4. Function Body: contains implementation
    double area = length * width;

    // 5. Return Statement: sends result back to caller
    return area;
}

int main() {
    // 6. Function Call: invoking the function
    double result = calculateArea(5.5, 3.2);
    cout << "Area: " << result << endl;

    return 0;
}
```

**Output:**

```
Area: 17.6
```

### 2.2 Component Details

### **1. Return Type**

Specifies the data type of value function returns.

```cpp
int getAge();           // Returns integer
double getPrice();      // Returns floating-point
char getGrade();        // Returns character
bool isValid();         // Returns boolean
void display();         // Returns nothing
string getName();       // Returns string object
int* getArray();        // Returns pointer to integer
```

### **2. Function Name**

**Naming Conventions:**

```cpp
// ✅ GOOD - Descriptive, verb-based names
calculateTotal()
validateInput()
getUserData()
printReport()

// ❌ BAD - Vague, unclear names
doStuff()
func1()
x()
process()
```

**Rules:**

- Must start with letter or underscore
- Can contain letters, digits, underscores
- Cannot be C++ keyword
- Case-sensitive

### **3. Parameter List**

Variables that receive values when function is called.

```cpp
// No parameters
void greet() { }

// Single parameter
void display(int value) { }

// Multiple parameters
int add(int a, int b) { }

// Different types
void process(int id, string name, double salary) { }
```

### **4. Function Body**

Contains the actual code to be executed.

```cpp
int multiply(int a, int b) {
    // Local variables
    int result = a * b;

    // Statements
    cout << "Multiplying " << a << " and " << b << endl;

    // Return
    return result;
}
```

### **5. Return Statement**

Sends value back to caller and terminates function.

```cpp
int max(int a, int b) {
    if (a > b) {
        return a;  // Early return
    }
    return b;      // Default return
}
```

---

## 3. Function Declaration vs Definition

### 3.1 Understanding the Difference

**Function Declaration (Prototype):**

- Tells compiler function exists
- Specifies name, return type, parameters
- Does NOT contain function body
- Ends with semicolon

**Function Definition:**

- Provides actual implementation
- Contains function body
- Can be placed anywhere in program

### 3.2 Why Both Are Needed

**Problem Without Declaration:**

```cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ ERROR: compiler doesn't know about multiply yet
    cout << multiply(5, 3) << endl;
    return 0;
}

int multiply(int a, int b) {
    return a * b;
}
```

**Solution 1: Declaration First**

```cpp
#include <iostream>
using namespace std;

// WHY: Declaration tells compiler function exists
int multiply(int a, int b);  // Forward declaration

int main() {
    // ✅ OK: compiler knows about multiply
    cout << multiply(5, 3) << endl;
    return 0;
}

// Definition can be after main()
int multiply(int a, int b) {
    return a * b;
}
```

**Solution 2: Define Before Use**

```cpp
#include <iostream>
using namespace std;

// WHY: Definition before use, no declaration needed
int multiply(int a, int b) {
    return a * b;
}

int main() {
    cout << multiply(5, 3) << endl;
    return 0;
}
```

### 3.3 Declaration Syntax Variations

```cpp
// Full declaration (most readable)
int add(int a, int b);

// Without parameter names (compiler only needs types)
int add(int, int);

// Multiple declarations
int subtract(int, int);
double subtract(double, double);  // Overloaded

// Complex return types
int* getArray(int size);
string& getName();
const int& getMax(const int& a, const int& b);
```

### 3.4 Real-World Usage

**Header File (.h) - Declarations:**

```cpp
// math_operations.h
#ifndef MATH_OPERATIONS_H
#define MATH_OPERATIONS_H

// WHY: Declarations in header for other files to use
int add(int a, int b);
int subtract(int a, int b);
double divide(double a, double b);

#endif
```

**Source File (.cpp) - Definitions:**

```cpp
// math_operations.cpp
#include "math_operations.h"

// WHY: Implementations in source file
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

double divide(double a, double b) {
    return a / b;
}
```

**Usage File:**

```cpp
// main.cpp
#include <iostream>
#include "math_operations.h"  // Gets declarations

int main() {
    // WHY: Can use functions without knowing implementation
    std::cout << add(10, 5) << std::endl;
    std::cout << subtract(10, 5) << std::endl;
    return 0;
}
```

---

## 4. Function Signature vs Function Prototype

### 4.1 Definitions

**Function Signature:**

- Function name + parameter types (in order)
- Does NOT include return type
- Used by compiler for function matching

**Function Prototype:**

- Complete function declaration
- Includes return type + name + parameters
- Ends with semicolon

### 4.2 Detailed Comparison

```cpp
// PROTOTYPE: Complete declaration
int add(int a, int b);

// SIGNATURE: Just name + parameter types
// add(int, int)

// ANOTHER EXAMPLE
double calculateArea(double length, double width);

// Signature: calculateArea(double, double)
// Prototype: double calculateArea(double length, double width);
```

**Comparison Table:**

| Aspect | Function Signature | Function Prototype |
| --- | --- | --- |
| **Components** | Name + parameter types | Return type + name + parameters |
| **Return Type** | NOT included | Included |
| **Parameter Names** | Optional (only types matter) | Optional but recommended |
| **Purpose** | Function matching/overloading | Declaration for compiler |
| **Example** | `foo(int, double)` | `int foo(int, double);` |

### 4.3 Why Signature Matters

**Function Overloading Example:**

```cpp
#include <iostream>
using namespace std;

// WHY: Signature determines which function to call

// Signature: print(int)
void print(int value) {
    cout << "Integer: " << value << endl;
}

// Signature: print(double)
void print(double value) {
    cout << "Double: " << value << endl;
}

// Signature: print(int, int)
void print(int a, int b) {
    cout << "Two integers: " << a << ", " << b << endl;
}

int main() {
    print(10);        // Calls print(int)
    print(10.5);      // Calls print(double)
    print(10, 20);    // Calls print(int, int)

    return 0;
}
```

**Output:**

```
Integer: 10
Double: 10.5
Two integers: 10, 20
```

### 4.4 Common Mistakes

**❌ Wrong: Same signature**

```cpp
int calculate(int a);
double calculate(int b);  // ERROR: Same signature calculate(int)
                          // Return type doesn't matter!
```

**✅ Correct: Different signatures**

```cpp
int calculate(int a);      // Signature: calculate(int)
int calculate(double a);   // Signature: calculate(double) - OK!
int calculate(int a, int b); // Signature: calculate(int, int) - OK!
```

---

## 5. Return Types & Return Statement

### 5.1 Understanding Return Types

**Purpose:** Specifies what data type the function will send back to caller.

```cpp
// WHY: Return type matches the kind of result function produces

int getAge() {
    return 25;  // Must return int
}

double getPrice() {
    return 99.99;  // Must return double
}

bool isEven(int n) {
    return n % 2 == 0;  // Must return bool
}

string getName() {
    return "John";  // Must return string
}
```

### 5.2 Return Statement Behavior

```cpp
#include <iostream>
using namespace std;

int findMax(int a, int b) {
    if (a > b) {
        return a;  // WHY: Early exit when condition met
        // Code after return is NOT executed
    }
    return b;  // Default return
}

int main() {
    cout << findMax(10, 5) << endl;   // Returns 10
    cout << findMax(3, 20) << endl;   // Returns 20
    return 0;
}
```

**Output:**

```
10
20
```

### 5.3 Multiple Return Statements

**Use Cases:**

1. **Error Handling:**

```cpp
int divide(int a, int b) {
    if (b == 0) {
        cout << "Error: Division by zero!" << endl;
        return 0;  // Early return on error
    }
    return a / b;  // Normal return
}
```

1. **Search Operations:**

```cpp
int findElement(int arr[], int size, int target) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == target) {
            return i;  // WHY: Return immediately when found
        }
    }
    return -1;  // Not found
}
```

1. **Validation Logic:**

```cpp
bool isValidAge(int age) {
    if (age < 0) return false;      // Invalid negative
    if (age > 150) return false;    // Invalid too large
    return true;                    // Valid range
}
```

### 5.4 Advanced Return Types

**Returning References:**

```cpp
#include <iostream>
using namespace std;

int globalValue = 100;

// WHY: Return reference to modify original variable
int& getGlobal() {
    return globalValue;  // Returns reference, not copy
}

int main() {
    getGlobal() = 200;  // Can modify through reference
    cout << globalValue << endl;  // 200

    return 0;
}
```

**Returning Pointers:**

```cpp
// WHY: Return pointer for dynamic memory or arrays
int* createArray(int size) {
    int* arr = new int[size];  // Dynamic allocation
    for (int i = 0; i < size; i++) {
        arr[i] = i * 10;
    }
    return arr;  // Return pointer to array
}

int main() {
    int* numbers = createArray(5);

    for (int i = 0; i < 5; i++) {
        cout << numbers[i] << " ";
    }

    delete[] numbers;  // Don't forget to free memory!
    return 0;
}
```

**Output:**

```
0 10 20 30 40
```

---

## 6. void Functions

### 6.1 What is void?

**Definition:** `void` means "nothing" - function performs action but doesn't return value.

**Purpose:**

- Functions that perform operations without producing result
- Display information, modify global state, print output
- Side-effects oriented functions

### 6.2 Basic void Functions

```cpp
#include <iostream>
using namespace std;

// WHY: Displays greeting, no value to return
void greet() {
    cout << "Hello, Welcome!" << endl;
}

// WHY: Prints formatted data, returns nothing
void displayInfo(string name, int age) {
    cout << "Name: " << name << endl;
    cout << "Age: " << age << endl;
}

// WHY: Modifies global state through reference
void increment(int& value) {
    value++;  // Modifies original variable
    // No return needed, effect is through reference
}

int main() {
    greet();
    displayInfo("Alice", 25);

    int counter = 10;
    increment(counter);
    cout << "Counter: " << counter << endl;  // 11

    return 0;
}
```

**Output:**

```
Hello, Welcome!
Name: Alice
Age: 25
Counter: 11
```

### 6.3 Return in void Functions

**You can use `return;` (without value) to exit early:**

```cpp
void processData(int value) {
    if (value < 0) {
        cout << "Error: Negative value" << endl;
        return;  // WHY: Early exit on error condition
    }

    // Process valid data
    cout << "Processing: " << value << endl;
}

int main() {
    processData(10);   // Processes normally
    processData(-5);   // Exits early
    return 0;
}
```

**Output:**

```
Processing: 10
Error: Negative value
```

### 6.4 void vs Return Value Functions

**When to use void:**

```cpp
// WHY: Action-oriented, no meaningful return value
void printHeader() {
    cout << "=== APPLICATION ===" << endl;
}

void saveToFile(const string& data) {
    // Save operation, success indicated by exceptions
}

void drawLine(int length) {
    for (int i = 0; i < length; i++) {
        cout << "-";
    }
    cout << endl;
}
```

**When to use return value:**

```cpp
// WHY: Computation produces result
int calculateSum(int a, int b) {
    return a + b;
}

// WHY: Query returns information
bool fileExists(const string& path) {
    // Check file
    return true;  // or false
}

// WHY: Transformation produces new value
string toUpperCase(const string& text) {
    string result = text;
    // Transform
    return result;
}
```

**Comparison Table:**

| Aspect | void Functions | Return Value Functions |
| --- | --- | --- |
| **Purpose** | Perform actions, side effects | Compute and return results |
| **Return** | No value returned | Specific type returned |
| **Usage** | Display, modify state, I/O | Calculations, queries, transformations |
| **Example** | `printData()` | `calculateTotal()` |
| **Early Exit** | `return;` (no value) | `return value;` |

---

## 7. Function Calling Mechanism

### 7.1 How Function Calls Work

**Call Sequence:**

1. **Caller pauses** execution
2. **Arguments evaluated** and passed
3. **Control transfers** to called function
4. **Function executes** its code
5. **Return value** (if any) sent back
6. **Control returns** to caller
7. **Caller continues** from next statement

**Visual Example:**

```cpp
#include <iostream>
using namespace std;

int multiply(int a, int b) {
    cout << "  Inside multiply" << endl;  // Step 3
    int result = a * b;
    cout << "  Returning from multiply" << endl;  // Step 4
    return result;  // Step 5
}

int main() {
    cout << "Before function call" << endl;  // Step 1

    int result = multiply(5, 3);  // Step 2: Call function

    cout << "After function call" << endl;  // Step 6
    cout << "Result: " << result << endl;

    return 0;
}
```

**Output:**

```
Before function call
  Inside multiply
  Returning from multiply
After function call
Result: 15
```

### 7.2 Parameter Passing

**By Value (Copy):**

```cpp
void modifyValue(int x) {
    x = 100;  // Changes local copy only
}

int main() {
    int num = 50;
    modifyValue(num);
    cout << num << endl;  // Still 50
    return 0;
}
```

**By Reference (Direct):**

```cpp
void modifyReference(int& x) {
    x = 100;  // Changes original variable
}

int main() {
    int num = 50;
    modifyReference(num);
    cout << num << endl;  // Now 100
    return 0;
}
```

### 7.3 Nested Function Calls

```cpp
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int calculate(int x, int y) {
    // WHY: Function calls can be nested
    return multiply(add(x, y), 2);  // (x + y) * 2
}

int main() {
    cout << calculate(3, 4) << endl;  // (3+4)*2 = 14
    return 0;
}
```

**Output:**

```
14
```

---

## 8. Call Stack & Stack Frames

### 8.1 What is the Call Stack?

**Definition:** The call stack is a special region of memory that stores information about active function calls in a program.

**Purpose:**

- Track function call hierarchy
- Store local variables and parameters
- Manage return addresses
- Enable function returns to correct location

**Structure:** LIFO (Last In, First Out) - like a stack of plates

### 8.2 Stack Frame (Activation Record)

**Each function call creates a stack frame containing:**

1. **Function parameters**
2. **Local variables**
3. **Return address** (where to go back)
4. **Previous frame pointer**
5. **Return value** space

**Visual Representation:**

```
HIGH MEMORY ADDRESS
┌─────────────────────┐
│   main() frame      │  ← Base of stack
│  - local vars       │
│  - parameters       │
│  - return addr      │
├─────────────────────┤
│   funcA() frame     │  ← Called from main
│  - local vars       │
│  - parameters       │
│  - return addr      │
├─────────────────────┤
│   funcB() frame     │  ← Called from funcA
│  - local vars       │  ← Stack Pointer (SP)
│  - parameters       │
│  - return addr      │
└─────────────────────┘
LOW MEMORY ADDRESS
```

### 8.3 Stack Frame Example

```cpp
#include <iostream>
using namespace std;

// Step 3: funcB creates its frame
int funcB(int x) {
    int local_b = x * 2;  // Local variable in funcB's frame
    cout << "In funcB: " << local_b << endl;
    return local_b;  // Step 4: Return to funcA
}

// Step 2: funcA creates its frame
int funcA(int a, int b) {
    int local_a = a + b;  // Local variable in funcA's frame
    cout << "In funcA: " << local_a << endl;

    int result = funcB(local_a);  // Step 3: Call funcB
    return result;  // Step 5: Return to main
}

// Step 1: main creates first frame
int main() {
    int x = 10;  // Local variable in main's frame
    cout << "In main: " << x << endl;

    int answer = funcA(x, 5);  // Step 2: Call funcA
    cout << "Answer: " << answer << endl;  // Step 6: Continue main

    return 0;  // Step 7: Program ends
}
```

**Output:**

```
In main: 10
In funcA: 15
In funcB: 30
Answer: 30
```

**Stack State During Execution:**

```
When funcB is executing:
┌─────────────────────┐
│   main() frame      │
│  x = 10             │
│  answer = ?         │
├─────────────────────┤
│   funcA() frame     │
│  a = 10, b = 5      │
│  local_a = 15       │
│  result = ?         │
├─────────────────────┤
│   funcB() frame     │  ← Current
│  x = 15             │
│  local_b = 30       │
└─────────────────────┘

After funcB returns:
┌─────────────────────┐
│   main() frame      │
│  x = 10             │
│  answer = ?         │
├─────────────────────┤
│   funcA() frame     │  ← Current
│  a = 10, b = 5      │
│  local_a = 15       │
│  result = 30        │  ← Received return value
└─────────────────────┘

After funcA returns:
┌─────────────────────┐
│   main() frame      │  ← Current
│  x = 10             │
│  answer = 30        │  ← Received return value
└─────────────────────┘
```

### 8.4 Stack Overflow

**Definition:** Stack overflow occurs when the call stack exceeds its maximum size.

**Common Causes:**

1. **Infinite Recursion:**

```cpp
// ❌ BAD: No base case
void infiniteRecursion(int n) {
    cout << n << endl;
    infiniteRecursion(n + 1);  // Never stops!
}

int main() {
    infiniteRecursion(1);  // STACK OVERFLOW!
    return 0;
}
```

1. **Too Many Nested Calls:**

```cpp
// ❌ BAD: Deep recursion without tail optimization
int factorial(int n) {
    if (n == 1) return 1;
    return n * factorial(n - 1);  // Creates n frames
}

int main() {
    cout << factorial(1000000) << endl;  // STACK OVERFLOW!
    return 0;
}
```

1. **Large Local Arrays:**

```cpp
// ❌ BAD: Huge local array on stack
void processData() {
    int huge[10000000];  // Too large for stack!
    // Process array...
}
```

**✅ Solutions:**

```cpp
// 1. Use iteration instead of recursion
int factorialIterative(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// 2. Use dynamic allocation for large data
void processData() {
    int* huge = new int[10000000];  // On heap, not stack
    // Process array...
    delete[] huge;
}

// 3. Proper base case in recursion
int fibonacci(int n) {
    if (n <= 1) return n;  // Base case prevents infinite recursion
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

### 8.5 Stack vs Heap

**Comparison:**

| Aspect | Stack | Heap |
| --- | --- | --- |
| **Allocation** | Automatic (function call) | Manual (`new`/`malloc`) |
| **Deallocation** | Automatic (function return) | Manual (`delete`/`free`) |
| **Size** | Limited (typically 1-8MB) | Large (limited by RAM) |
| **Speed** | Very fast | Slower |
| **Lifetime** | Function scope | Until manually freed |
| **Usage** | Local variables, parameters | Dynamic data structures |
| **Fragmentation** | None | Can occur |

---

## 9. Function Scope & Lifetime

### 9.1 Variable Scope

**Local Variables:**

```cpp
#include <iostream>
using namespace std;

void function1() {
    int x = 10;  // WHY: Local to function1 only
    cout << "function1: " << x << endl;
}

void function2() {
    int x = 20;  // WHY: Different x, local to function2
    cout << "function2: " << x << endl;
}

int main() {
    int x = 30;  // WHY: Different x, local to main
    cout << "main: " << x << endl;

    function1();  // Uses its own x
    function2();  // Uses its own x

    cout << "main: " << x << endl;  // Still 30

    return 0;
}
```

**Output:**

```
main: 30
function1: 10
function2: 20
main: 30
```

**Global Variables:**

```cpp
#include <iostream>
using namespace std;

// WHY: Global variable accessible everywhere
int globalCounter = 0;

void increment() {
    globalCounter++;  // Can access global
}

void display() {
    cout << "Global: " << globalCounter << endl;
}

int main() {
    display();      // 0
    increment();
    increment();
    display();      // 2

    return 0;
}
```

**Output:**

```
Global: 0
Global: 2
```

### 9.2 Variable Lifetime

**Stack Variables (Automatic Storage):**

```cpp
void demonstrate() {
    int local = 100;  // Created when function called
    // Use local...
}  // Destroyed when function returns

int main() {
    demonstrate();
    // 'local' no longer exists here
    return 0;
}
```

**Static Variables:**

```cpp
#include <iostream>
using namespace std;

void counter() {
    static int count = 0;  // WHY: Initialized only once, persists between calls
    count++;
    cout << "Count: " << count << endl;
}

int main() {
    counter();  // Count: 1
    counter();  // Count: 2
    counter();  // Count: 3

    return 0;
}
```

**Output:**

```
Count: 1
Count: 2
Count: 3
```

### 9.3 Best Practices

**Scope Rules:**

- Keep variables in smallest possible scope
- Prefer local over global
- Use parameters instead of globals
- Use `static` judiciously

**Example:**

```cpp
// ❌ BAD: Unnecessary global
int temp;  // Global - bad practice

void process(int value) {
    temp = value * 2;
    // Use temp...
}

// ✅ GOOD: Local variable
void process(int value) {
    int temp = value * 2;  // Local - better
    // Use temp...
}

// ✅ GOOD: Return result instead
int process(int value) {
    return value * 2;  // Even better - functional style
}
```

---

## Summary

### Key Takeaways

1. **Functions are fundamental building blocks** - Enable modular, reusable, maintainable code
2. **Function components** - Return type, name, parameters, body, return statement
3. **Declaration vs Definition** - Declaration tells compiler function exists; definition provides implementation
4. **Signature vs Prototype** - Signature is name + parameter types; prototype includes return type
5. **Return types** - Specify what function produces; `void` means no return value
6. **Call stack mechanism** - LIFO structure manages function calls; each call creates stack frame
7. **Stack frames** - Contain local variables, parameters, return address; destroyed on return
8. **Stack overflow** - Occurs from infinite recursion, deep nesting, or large local arrays
9. **Variable scope** - Local variables exist in function; global variables accessible everywhere
10. **Variable lifetime** - Stack variables destroyed on return; static variables persist

### Interview Essential Points

**Q: What is a function in C++?**
A: A function is a self-contained block of code designed to perform a specific task. It enables code reusability, modularity, and abstraction. Functions have a return type, name, parameter list, and body. They can be called multiple times from different parts of the program.

**Q: Difference between function declaration and definition?**
A: Declaration (prototype) tells the compiler about function's existence - includes return type, name, and parameters but no body. Definition provides the actual implementation with function body. Declarations typically go in header files, definitions in source files.

**Q: What is function signature vs prototype?**
A: Function signature consists of function name + parameter types (without return type). Prototype is the complete declaration including return type. Example: `int add(int, int)` - prototype is full declaration, signature is `add(int, int)`.

**Q: Explain the call stack and stack frames.**
A: Call stack is LIFO memory structure storing active function calls. Each function call creates a stack frame (activation record) containing local variables, parameters, return address, and saved registers. Frames are pushed on call, popped on return.

**Q: What causes stack overflow?**
A: Stack overflow occurs when call stack exceeds maximum size, typically from: (1) Infinite recursion without base case, (2) Excessive recursion depth, (3) Large local arrays. Solutions: use iteration, limit recursion depth, allocate large data on heap.

**Q: When to use void functions?**
A: Use `void` when function performs action without producing meaningful return value - display operations, I/O, state modifications. Functions that compute values should return result instead of using global variables or output parameters.

**Q: Scope and lifetime of variables?**
A: Scope is where variable is accessible (local vs global). Lifetime is duration variable exists in memory. Local variables have function scope and automatic lifetime (destroyed on return). Global variables have program scope and static lifetime (exist entire program). Static local variables have function scope but static lifetime.

**Q: Best practices for functions?**
A: (1) Single responsibility principle, (2) Descriptive names, (3) Limit parameters (3-4 max), (4) Prefer return values over output parameters, (5) Minimize global variable use, (6) Keep functions short (<50 lines), (7) Document complex logic.

---