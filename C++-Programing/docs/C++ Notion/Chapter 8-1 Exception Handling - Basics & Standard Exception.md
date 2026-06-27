# 8.1. Exception Handling - Basics & Standard Exceptions

---

## Table of Contents

1. Introduction to Exception Handling
2. Exception Handling Basics
3. Exception Flow and Propagation
4. Standard Exception Hierarchy
5. Custom Exception Classes
6. Multiple Catch Blocks
7. Re-throwing Exceptions
8. Summary

---

## 1. Introduction to Exception Handling

### 1.1 What are Exceptions?

**Exceptions** are runtime anomalies or errors that occur during program execution. They represent conditions that disrupt the normal flow of a program.

**Examples of Exception Scenarios:**

- Division by zero
- Array index out of bounds
- Memory allocation failure
- File not found
- Invalid user input
- Network connection timeout

```cpp
#include <iostream>
using namespace std;

// WHY: Without exceptions - program crashes
void divide_bad(int a, int b) {
    cout << "Result: " << a / b << endl;  // Crashes if b is 0!
}

int main() {
    divide_bad(10, 0);  // Runtime error - undefined behavior
    return 0;
}
```

### 1.2 Why Exception Handling?

**Problems with Traditional Error Handling:**

```cpp
// WHY: Error codes clutter the code
int divide(int a, int b, int* result) {
    if (b == 0) {
        return -1;  // Error code
    }
    *result = a / b;
    return 0;  // Success
}

int main() {
    int result;
    // WHY: Must check error codes everywhere
    if (divide(10, 2, &result) != 0) {
        cout << "Error" << endl;
        return 1;
    }
    cout << result << endl;

    // More error checking...
    if (divide(20, 0, &result) != 0) {
        cout << "Error" << endl;
        return 1;
    }
    return 0;
}
```

**With Exception Handling:**

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

// WHY: Clean function - no error code mixing
double divide(int a, int b) {
    if (b == 0) {
        throw runtime_error("Division by zero");
    }
    return static_cast<double>(a) / b;
}

int main() {
    try {
        // WHY: Clean flow - no error checking in normal path
        cout << divide(10, 2) << endl;
        cout << divide(20, 0) << endl;  // Throws exception
        cout << "This won't execute" << endl;
    } catch (const runtime_error& e) {
        // WHY: Error handling separated from normal flow
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
5
Error: Division by zero
```

### 1.3 Advantages of Exception Handling

| Aspect | Traditional Error Codes | Exception Handling |
| --- | --- | --- |
| **Code Readability** | Cluttered with if checks | Clean separation |
| **Error Propagation** | Manual through each level | Automatic up call stack |
| **Constructor Errors** | Impossible to report | Natural mechanism |
| **Resource Cleanup** | Manual in each error path | Automatic (RAII) |
| **Can't Ignore** | Easy to ignore return codes | Forces handling or crash |
| **Performance (happy path)** | Slower (constant checks) | Faster (zero-cost) |
| **Performance (error path)** | Faster | Slower |

---

## 2. Exception Handling Basics

### 2.1 The Three Keywords

**C++ exception handling uses three keywords:**

1. **`try`** - Defines a block where exceptions may occur
2. **`catch`** - Defines how to handle exceptions
3. **`throw`** - Throws an exception

**Basic Syntax:**

```cpp
try {
    // Protected code that may throw exceptions
} catch (ExceptionType e) {
    // Handle the exception
}
```

### 2.2 try Block

The **try block** contains code that might throw exceptions:

```cpp
#include <iostream>
using namespace std;

int main() {
    try {
        // WHY: Protected code - exceptions caught
        int age = -5;
        if (age < 0) {
            throw age;  // Throw an integer
        }
        cout << "Age: " << age << endl;
    } catch (int e) {
        cout << "Caught exception: " << e << endl;
    }

    cout << "Program continues" << endl;
    return 0;
}
```

**Output:**

```
Caught exception: -5
Program continues
```

### 2.3 throw Statement

The **throw statement** raises an exception:

```cpp
#include <iostream>
using namespace std;

void checkPositive(int num) {
    // WHY: Throw exception if condition violated
    if (num <= 0) {
        throw "Number must be positive";
    }
    cout << "Valid number: " << num << endl;
}

int main() {
    try {
        checkPositive(10);   // OK
        checkPositive(-5);   // Throws
        checkPositive(20);   // Never executes
    } catch (const char* msg) {
        cout << "Error: " << msg << endl;
    }
    return 0;
}
```

**Output:**

```
Valid number: 10
Error: Number must be positive
```

**What Can Be Thrown:**

```cpp
// WHY: C++ allows throwing any type
throw 42;                          // int
throw 3.14;                        // double
throw 'X';                         // char
throw "Error message";             // const char*
throw string("Error");             // std::string
throw runtime_error("Error");      // Exception object
throw MyCustomException();         // Custom exception
```

### 2.4 catch Block

The **catch block** handles exceptions:

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Different catch blocks for different types
    try {
        int choice;
        cout << "Enter 1, 2, or 3: ";
        cin >> choice;

        if (choice == 1) throw 100;
        else if (choice == 2) throw 3.14;
        else if (choice == 3) throw string("Error");
        else throw 'X';

    } catch (int e) {
        cout << "Caught int: " << e << endl;
    } catch (double e) {
        cout << "Caught double: " << e << endl;
    } catch (string e) {
        cout << "Caught string: " << e << endl;
    } catch (...) {
        // WHY: Catch-all for any other type
        cout << "Caught unknown exception" << endl;
    }

    return 0;
}
```

### 2.5 Catching by Value vs Reference

```cpp
#include <iostream>
#include <exception>
using namespace std;

class MyException : public exception {
public:
    const char* what() const noexcept override {
        return "MyException occurred";
    }
};

int main() {
    // ❌ BAD: Catch by value (object slicing)
    try {
        throw MyException();
    } catch (exception e) {  // Slicing! Loses derived class info
        cout << e.what() << endl;
    }

    // ✅ GOOD: Catch by reference (no slicing)
    try {
        throw MyException();
    } catch (const exception& e) {  // Preserves derived class
        cout << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
std::exception
MyException occurred
```

**Best Practice:**

```cpp
// ✅ ALWAYS catch exceptions by const reference
catch (const std::exception& e) { }
catch (const MyException& e) { }
catch (const std::string& e) { }
```

---

## 3. Exception Flow and Propagation

### 3.1 Exception Propagation

When an exception is thrown, it propagates up the call stack until caught:

```cpp
#include <iostream>
using namespace std;

void level3() {
    cout << "Level 3: Throwing exception" << endl;
    throw runtime_error("Error at level 3");
}

void level2() {
    cout << "Level 2: Calling level 3" << endl;
    level3();  // Exception propagates from here
    cout << "Level 2: After level 3" << endl;  // Never executes
}

void level1() {
    cout << "Level 1: Calling level 2" << endl;
    level2();  // Exception propagates through here
    cout << "Level 1: After level 2" << endl;  // Never executes
}

int main() {
    try {
        cout << "Main: Calling level 1" << endl;
        level1();  // Exception caught here
        cout << "Main: After level 1" << endl;  // Never executes
    } catch (const runtime_error& e) {
        cout << "Main: Caught exception: " << e.what() << endl;
    }
    cout << "Main: Program continues" << endl;
    return 0;
}
```

**Output:**

```
Main: Calling level 1
Level 1: Calling level 2
Level 2: Calling level 3
Level 3: Throwing exception
Main: Caught exception: Error at level 3
Main: Program continues
```

**Flow Diagram:**

```
main() [try]
  └─> level1()
       └─> level2()
            └─> level3() [throw]
                    ↓
            [propagates up]
                    ↓
       [propagates up]
                    ↓
[caught in main]
```

### 3.2 Uncaught Exceptions

```cpp
#include <iostream>
using namespace std;

void problematic() {
    throw runtime_error("Uncaught exception");
}

int main() {
    // WHY: No try-catch block
    problematic();  // Exception not caught!
    cout << "This never executes" << endl;
    return 0;
}
```

**Result:** Program terminates with `std::terminate()` called

**Output (typical):**

```
terminate called after throwing an instance of 'std::runtime_error'
  what():  Uncaught exception
Aborted (core dumped)
```

### 3.3 Exception in Function Chain

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

double safeDivide(int a, int b) {
    // WHY: Lowest level throws exception
    if (b == 0) {
        throw invalid_argument("Division by zero");
    }
    return static_cast<double>(a) / b;
}

double calculate(int x, int y) {
    // WHY: Middle level doesn't catch - propagates up
    return safeDivide(x, y) * 2;
}

int main() {
    try {
        // WHY: Top level catches exception
        cout << "Result: " << calculate(10, 2) << endl;
        cout << "Result: " << calculate(10, 0) << endl;
    } catch (const invalid_argument& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
Result: 10
Error: Division by zero
```

---

## 4. Standard Exception Hierarchy

### 4.1 The std::exception Base Class

All standard exceptions inherit from `std::exception`:

```cpp
#include <exception>

namespace std {
    class exception {
    public:
        exception() noexcept;
        exception(const exception&) noexcept;
        exception& operator=(const exception&) noexcept;
        virtual ~exception();

        // WHY: what() returns description of the exception
        virtual const char* what() const noexcept;
    };
}
```

### 4.2 Complete Exception Hierarchy

```
std::exception (base class)
│
├── std::logic_error
│   ├── std::invalid_argument
│   ├── std::domain_error
│   ├── std::length_error
│   └── std::out_of_range
│
├── std::runtime_error
│   ├── std::range_error
│   ├── std::overflow_error
│   └── std::underflow_error
│
├── std::bad_alloc
├── std::bad_cast
├── std::bad_typeid
├── std::bad_exception
├── std::bad_function_call
└── std::bad_weak_ptr
```

### 4.3 logic_error Family

**Used for errors detectable before program runs:**

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

int main() {
    try {
        // WHY: invalid_argument - invalid function parameter
        string str = "abc";
        int value = stoi(str);  // Can't convert to int

    } catch (const invalid_argument& e) {
        cout << "invalid_argument: " << e.what() << endl;
    }

    try {
        // WHY: out_of_range - access outside valid range
        string str = "hello";
        char c = str.at(100);  // Index out of range

    } catch (const out_of_range& e) {
        cout << "out_of_range: " << e.what() << endl;
    }

    try {
        // WHY: length_error - attempt to exceed max size
        string str;
        str.resize(str.max_size() + 1);  // Too large

    } catch (const length_error& e) {
        cout << "length_error: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
invalid_argument: stoi
out_of_range: basic_string::at
length_error: basic_string::_M_create
```

### 4.4 runtime_error Family

**Used for errors detectable only at runtime:**

```cpp
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace std;

double safeDivide(double a, double b) {
    if (b == 0.0) {
        throw runtime_error("Division by zero");
    }

    double result = a / b;

    // WHY: Check for overflow
    if (result == INFINITY) {
        throw overflow_error("Result too large");
    }

    // WHY: Check for underflow
    if (result == 0.0 && a != 0.0) {
        throw underflow_error("Result too small");
    }

    return result;
}

int main() {
    try {
        cout << safeDivide(10.0, 2.0) << endl;
        cout << safeDivide(10.0, 0.0) << endl;
    } catch (const runtime_error& e) {
        cout << "runtime_error: " << e.what() << endl;
    } catch (const overflow_error& e) {
        cout << "overflow_error: " << e.what() << endl;
    } catch (const underflow_error& e) {
        cout << "underflow_error: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
5
runtime_error: Division by zero
```

### 4.5 Other Standard Exceptions

**bad_alloc - Memory Allocation Failure:**

```cpp
#include <iostream>
#include <new>
using namespace std;

int main() {
    try {
        // WHY: Try to allocate huge amount of memory
        size_t huge = 1000000000000000;
        int* arr = new int[huge];
    } catch (const bad_alloc& e) {
        cout << "bad_alloc: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
bad_alloc: std::bad_alloc
```

**bad_cast - Invalid dynamic_cast:**

```cpp
#include <iostream>
#include <typeinfo>
using namespace std;

class Base { virtual ~Base() {} };
class Derived : public Base {};
class Other : public Base {};

int main() {
    try {
        Base* b = new Other();
        // WHY: Invalid cast - Other is not Derived
        Derived& d = dynamic_cast<Derived&>(*b);
    } catch (const bad_cast& e) {
        cout << "bad_cast: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
bad_cast: std::bad_cast
```

### 4.6 Catching Base Class

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

void throwRandom(int choice) {
    if (choice == 1) throw invalid_argument("Invalid arg");
    if (choice == 2) throw out_of_range("Out of range");
    if (choice == 3) throw runtime_error("Runtime error");
    if (choice == 4) throw overflow_error("Overflow");
}

int main() {
    for (int i = 1; i <= 4; ++i) {
        try {
            throwRandom(i);
        } catch (const exception& e) {
            // WHY: Catches all exceptions derived from std::exception
            cout << "Caught: " << e.what() << endl;
        }
    }
    return 0;
}
```

**Output:**

```
Caught: Invalid arg
Caught: Out of range
Caught: Runtime error
Caught: Overflow
```

---

## 5. Custom Exception Classes

### 5.1 Why Custom Exceptions?

**Reasons to create custom exceptions:**

1. More specific error information
2. Domain-specific error handling
3. Additional data with exception
4. Clear intent in code
5. Type-safe exception handling

### 5.2 Pattern 1: Inherit from std::exception

```cpp
#include <iostream>
#include <exception>
using namespace std;

// WHY: Simplest custom exception
class FileException : public exception {
private:
    string message;

public:
    FileException(const string& msg) : message(msg) {}

    // WHY: Override what() to provide description
    const char* what() const noexcept override {
        return message.c_str();
    }
};

void openFile(const string& filename) {
    // WHY: Simulate file not found
    if (filename.empty()) {
        throw FileException("Filename cannot be empty");
    }
    if (filename == "invalid.txt") {
        throw FileException("File not found: " + filename);
    }
    cout << "File opened: " << filename << endl;
}

int main() {
    try {
        openFile("data.txt");
        openFile("invalid.txt");
    } catch (const FileException& e) {
        cout << "File error: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
File opened: data.txt
File error: File not found: invalid.txt
```

### 5.3 Pattern 2: Inherit from runtime_error or logic_error

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

// WHY: Inherit from runtime_error (easier message handling)
class DatabaseException : public runtime_error {
public:
    DatabaseException(const string& msg)
        : runtime_error("Database Error: " + msg) {}
};

class NetworkException : public runtime_error {
public:
    NetworkException(const string& msg)
        : runtime_error("Network Error: " + msg) {}
};

void connectDatabase() {
    // Simulate connection failure
    throw DatabaseException("Connection timeout");
}

void sendData() {
    // Simulate network error
    throw NetworkException("Connection lost");
}

int main() {
    try {
        connectDatabase();
    } catch (const DatabaseException& e) {
        cout << e.what() << endl;
    }

    try {
        sendData();
    } catch (const NetworkException& e) {
        cout << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Database Error: Connection timeout
Network Error: Connection lost
```

### 5.4 Pattern 3: Exception with Additional Data

```cpp
#include <iostream>
#include <stdexcept>
#include <string>
using namespace std;

// WHY: Custom exception with additional context
class ValidationException : public exception {
private:
    string field;
    string message;
    int errorCode;
    string fullMessage;

public:
    ValidationException(const string& f, const string& msg, int code = 0)
        : field(f), message(msg), errorCode(code) {
        fullMessage = "Validation Error [" + field + "]: " + message;
    }

    const char* what() const noexcept override {
        return fullMessage.c_str();
    }

    // WHY: Provide access to additional data
    string getField() const { return field; }
    string getMessage() const { return message; }
    int getErrorCode() const { return errorCode; }
};

void validateAge(int age) {
    if (age < 0) {
        throw ValidationException("age", "Cannot be negative", 101);
    }
    if (age > 150) {
        throw ValidationException("age", "Unrealistic value", 102);
    }
}

void validateEmail(const string& email) {
    if (email.find('@') == string::npos) {
        throw ValidationException("email", "Must contain @", 201);
    }
}

int main() {
    try {
        validateAge(-5);
    } catch (const ValidationException& e) {
        cout << e.what() << endl;
        cout << "Field: " << e.getField() << endl;
        cout << "Error code: " << e.getErrorCode() << endl;
    }

    try {
        validateEmail("invalid-email");
    } catch (const ValidationException& e) {
        cout << e.what() << endl;
        cout << "Field: " << e.getField() << endl;
        cout << "Error code: " << e.getErrorCode() << endl;
    }

    return 0;
}
```

**Output:**

```
Validation Error [age]: Cannot be negative
Field: age
Error code: 101
Validation Error [email]: Must contain @
Field: email
Error code: 201
```

### 5.5 Exception Hierarchy Example

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

// WHY: Base exception for application
class AppException : public exception {
protected:
    string message;
public:
    AppException(const string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// WHY: Specific exception categories
class ConfigException : public AppException {
public:
    ConfigException(const string& msg)
        : AppException("Config: " + msg) {}
};

class SecurityException : public AppException {
public:
    SecurityException(const string& msg)
        : AppException("Security: " + msg) {}
};

class DataException : public AppException {
public:
    DataException(const string& msg)
        : AppException("Data: " + msg) {}
};

int main() {
    try {
        throw ConfigException("Missing config file");
    } catch (const AppException& e) {
        // WHY: Can catch all app exceptions with base class
        cout << "Application error: " << e.what() << endl;
    }

    try {
        throw SecurityException("Unauthorized access");
    } catch (const SecurityException& e) {
        // WHY: Or catch specific exception type
        cout << "Security alert: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Application error: Config: Missing config file
Security alert: Security: Unauthorized access
```

---

## 6. Multiple Catch Blocks

### 6.1 Handling Different Exception Types

```cpp
#include <iostream>
#include <stdexcept>
#include <string>
using namespace std;

void processInput(int choice) {
    if (choice == 1) throw 42;
    if (choice == 2) throw 3.14;
    if (choice == 3) throw string("Error");
    if (choice == 4) throw runtime_error("Runtime error");
}

int main() {
    for (int i = 1; i <= 5; ++i) {
        try {
            processInput(i);
            cout << "No exception" << endl;
        }
        // WHY: Multiple catch blocks for different types
        catch (int e) {
            cout << "Caught int: " << e << endl;
        }
        catch (double e) {
            cout << "Caught double: " << e << endl;
        }
        catch (const string& e) {
            cout << "Caught string: " << e << endl;
        }
        catch (const runtime_error& e) {
            cout << "Caught runtime_error: " << e.what() << endl;
        }
        catch (...) {
            // WHY: Catch-all for any other type
            cout << "Caught unknown exception" << endl;
        }
    }
    return 0;
}
```

**Output:**

```
Caught int: 42
Caught double: 3.14
Caught string: Error
Caught runtime_error: Runtime error
No exception
```

### 6.2 Catch Block Ordering

**CRITICAL:** Catch blocks are checked in order - **specific before general**

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

int main() {
    // ✅ CORRECT: Specific before general
    try {
        throw invalid_argument("Invalid argument");
    }
    catch (const invalid_argument& e) {
        cout << "Caught invalid_argument" << endl;
    }
    catch (const logic_error& e) {
        cout << "Caught logic_error" << endl;
    }
    catch (const exception& e) {
        cout << "Caught exception" << endl;
    }

    // ❌ WRONG: General before specific
    // This will cause compilation error or always catch in first block
    /*
    try {
        throw invalid_argument("Invalid");
    }
    catch (const exception& e) {
        // This catches everything - specific blocks never reached!
        cout << "Caught exception" << endl;
    }
    catch (const logic_error& e) {  // Unreachable!
        cout << "Caught logic_error" << endl;
    }
    */

    return 0;
}
```

**Output:**

```
Caught invalid_argument
```

**Correct Order:**

```
Most Specific
    ↓
invalid_argument
    ↓
logic_error
    ↓
exception (base class)
    ↓
catch(...) - catch all
Most General
```

### 6.3 Catch-All Handler

```cpp
#include <iostream>
using namespace std;

void riskyOperation(int choice) {
    if (choice == 1) throw 42;
    if (choice == 2) throw "Error string";
    if (choice == 3) throw 3.14;
    struct CustomType {};
    if (choice == 4) throw CustomType();
}

int main() {
    for (int i = 1; i <= 4; ++i) {
        try {
            riskyOperation(i);
        }
        catch (int e) {
            cout << "Caught int: " << e << endl;
        }
        catch (...) {
            // WHY: Catch any type not caught by previous handlers
            cout << "Caught unknown exception" << endl;
        }
    }
    return 0;
}
```

**Output:**

```
Caught int: 42
Caught unknown exception
Caught unknown exception
Caught unknown exception
```

---

## 7. Re-throwing Exceptions

### 7.1 Simple Re-throw

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

void processData() {
    try {
        // Some operation that throws
        throw runtime_error("Data processing error");
    } catch (const runtime_error& e) {
        // WHY: Log or do some cleanup
        cout << "Logging error: " << e.what() << endl;

        // WHY: Re-throw to let caller handle
        throw;  // Re-throws the SAME exception object
    }
}

int main() {
    try {
        processData();
    } catch (const runtime_error& e) {
        cout << "Caught in main: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
Logging error: Data processing error
Caught in main: Data processing error
```

### 7.2 throw vs throw e

**CRITICAL DIFFERENCE:**

```cpp
#include <iostream>
#include <exception>
using namespace std;

class DerivedError : public exception {
public:
    const char* what() const noexcept override {
        return "DerivedError";
    }
};

void method1() {
    try {
        throw DerivedError();
    } catch (exception& e) {
        // ❌ BAD: throw e causes slicing!
        cout << "Before throw e: " << e.what() << endl;
        throw e;  // Throws exception (base class), loses derived info!
    }
}

void method2() {
    try {
        throw DerivedError();
    } catch (exception& e) {
        // ✅ GOOD: throw preserves exact exception
        cout << "Before throw: " << e.what() << endl;
        throw;  // Re-throws DerivedError (keeps derived type)!
    }
}

int main() {
    try {
        method1();
    } catch (const exception& e) {
        cout << "Caught from method1: " << e.what() << endl;
    }

    try {
        method2();
    } catch (const exception& e) {
        cout << "Caught from method2: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Before throw e: DerivedError
Caught from method1: std::exception
Before throw: DerivedError
Caught from method2: DerivedError
```

### 7.3 Conditional Re-throw

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

void processTransaction(bool canRecover) {
    try {
        throw runtime_error("Transaction failed");
    } catch (const runtime_error& e) {
        cout << "Error occurred: " << e.what() << endl;

        if (canRecover) {
            // WHY: Handle the error here
            cout << "Recovered from error" << endl;
            // Don't re-throw
        } else {
            // WHY: Can't handle - pass to caller
            cout << "Cannot recover - re-throwing" << endl;
            throw;  // Re-throw
        }
    }
}

int main() {
    try {
        cout << "Case 1: Can recover" << endl;
        processTransaction(true);
        cout << "Continuing after recovery\n" << endl;
    } catch (const runtime_error& e) {
        cout << "Caught in main: " << e.what() << endl;
    }

    try {
        cout << "Case 2: Cannot recover" << endl;
        processTransaction(false);
        cout << "This won't execute" << endl;
    } catch (const runtime_error& e) {
        cout << "Caught in main: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Case 1: Can recover
Error occurred: Transaction failed
Recovered from error
Continuing after recovery

Case 2: Cannot recover
Error occurred: Transaction failed
Cannot recover - re-throwing
Caught in main: Transaction failed
```

### 7.4 Wrapping Exceptions

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

class DatabaseException : public runtime_error {
public:
    DatabaseException(const string& msg)
        : runtime_error("Database: " + msg) {}
};

class ServiceException : public runtime_error {
public:
    ServiceException(const string& msg)
        : runtime_error("Service: " + msg) {}
};

void databaseOperation() {
    throw DatabaseException("Connection lost");
}

void serviceLayer() {
    try {
        databaseOperation();
    } catch (const DatabaseException& e) {
        // WHY: Wrap low-level exception in high-level exception
        cout << "Caught database error: " << e.what() << endl;
        throw ServiceException("Failed to process request due to database error");
    }
}

int main() {
    try {
        serviceLayer();
    } catch (const ServiceException& e) {
        cout << "Service layer error: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
Caught database error: Database: Connection lost
Service layer error: Service: Failed to process request due to database error
```

---

## Summary

### Key Takeaways

1. **Exceptions Handle Runtime Errors Gracefully** - Unlike return codes, exceptions separate error handling from normal flow, making code cleaner and more maintainable. Use for unexpected errors, not flow control.
2. **try-catch-throw are the Three Keywords** - `try` defines protected code, `throw` raises exceptions, `catch` handles them. Exceptions propagate up the call stack automatically until caught or program terminates.
3. **Always Catch by const Reference** - `catch (const exception& e)` prevents object slicing and avoids unnecessary copying. Never catch by value for polymorphic types.
4. **Standard Exception Hierarchy is Well-Designed** - `std::exception` is the base. `logic_error` for programming errors, `runtime_error` for runtime issues. Use appropriate standard exceptions before creating custom ones.
5. **Custom Exceptions Add Context** - Inherit from `std::exception` or `std::runtime_error`. Override `what()` method. Add domain-specific data when needed. Create exception hierarchies for large applications.
6. **Multiple Catch Blocks: Specific Before General** - Order matters! Catch derived classes before base classes. Use catch-all `catch(...)` as last resort to prevent crashes.
7. **Exception Propagation is Automatic** - No need to manually pass errors through each function. Exceptions bubble up until caught or program terminates with `std::terminate()`.
8. **Re-throw with throw; Not throw e;** - Use bare `throw;` to preserve exception type (no slicing). `throw e;` creates a copy and causes slicing if e is a base class reference to derived object.
9. **Exceptions vs Error Codes Trade-offs** - Exceptions: cleaner code, automatic propagation, works in constructors. Error codes: predictable performance, explicit in signatures. Choose based on context.
10. **Uncaught Exceptions Terminate Program** - If no catch block handles an exception, `std::terminate()` is called. Always have appropriate handlers or let program crash intentionally for critical errors.

---