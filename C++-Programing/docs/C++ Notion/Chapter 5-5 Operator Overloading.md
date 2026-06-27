# 5.5. Operator Overloading

---

## Table of Contents

1. Understanding Operator Overloading
2. Overloadable vs Non-Overloadable Operators
3. Unary Operator Overloading
4. Binary Operator Overloading
5. Special Operators
6. Member vs Friend Function
7. Best Practices and Common Pitfalls
8. Summary

---

## 1. Understanding Operator Overloading

### 1.1 What is Operator Overloading?

**Definition**: Giving new meaning to existing operators when used with user-defined types (classes/structs).

**Core Concept**: Make custom types behave like built-in types.

**Why Operator Overloading?**

- Natural syntax: `c1 + c2` instead of `c1.add(c2)`
- Intuitive code: `cout << obj` instead of `obj.print()`
- Mathematical types: Complex numbers, Vectors, Matrices
- Compile-time polymorphism
- Code readability

**Syntax**: `returnType operator@(parameters) { }`

Where `@` is the operator symbol (+, -, *, /, ==, etc.)

```cpp
// operator_overloading_intro.cpp
#include <iostream>
using namespace std;

class Complex {
private:
    double real, imag;

public:
    Complex(double r = 0, double i = 0) : real(r), imag(i) {}

    // WHY: Overload + operator for natural syntax
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    void display() const {
        cout << real << " + " << imag << "i" << endl;
    }
};

int main() {
    Complex c1(3, 4);
    Complex c2(1, 2);

    // WHY: Natural syntax like built-in types
    Complex c3 = c1 + c2;  // Calls c1.operator+(c2)

    cout << "c1: "; c1.display();
    cout << "c2: "; c2.display();
    cout << "c3: "; c3.display();

    // WHY: Without overloading, would need:
    // Complex c3 = c1.add(c2);  // Less natural

    return 0;
}
```

**Output:**

```
c1: 3 + 4i
c2: 1 + 2i
c3: 4 + 6i
```

### 1.2 Types of Operator Overloading

**Two Main Categories:**

1. **Unary Operators** - Operate on single operand
    - Examples: ++, --, -, +, !, ~
    - Syntax: `operator@()` (no parameters for member function)
2. **Binary Operators** - Operate on two operands
    - Examples: +, -, *, /, ==, !=, <, >
    - Syntax: `operator@(param)` (one parameter for member function)

**Special Categories:**

- Assignment operators: =, +=, -=, *=, /=
- Stream operators: <<, >>
- Subscript operator: []
- Function call operator: ()
- Member access: ->

---

## 2. Overloadable vs Non-Overloadable Operators

### 2.1 Overloadable Operators

**Arithmetic Operators:**

```
+   -   *   /   %   (binary)
+   -   (unary)
```

**Comparison/Relational:**

```
==  !=  <   >   <=  >=
```

**Logical:**

```
&&  ||  !
```

**Bitwise:**

```
&   |   ^   ~   <<  >>
```

**Assignment:**

```
=   +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
```

**Increment/Decrement:**

```
++  --  (prefix and postfix)
```

**Special:**

```
[]  ()  ->  ->*  ,  new  delete  new[]  delete[]
```

### 2.2 Non-Overloadable Operators

**CANNOT be overloaded:**

| Operator | Name | Reason |
| --- | --- | --- |
| `::` | Scope resolution | Operates on names, not values |
| `.` | Member access | Fundamental language feature |
| `.*` | Pointer-to-member | Fundamental language feature |
| `?:` | Ternary conditional | Special evaluation rules |
| `sizeof` | Size operator | Compile-time evaluation |
| `typeid` | Type information | RTTI, must remain unchanged |
| `alignof` | Alignment (C++11) | Compile-time evaluation |

**Why Cannot Overload:**

- Operate on names rather than values
- Fundamental to language structure
- Overloading would break language semantics
- Evaluated at compile time

```cpp
// non_overloadable_demo.cpp
#include <iostream>
using namespace std;

class MyClass {
public:
    int value;

    MyClass(int v) : value(v) {}

    // WHY: Cannot overload these operators
    // void operator::() { }     // ERROR!
    // void operator.() { }      // ERROR!
    // void operator.*() { }     // ERROR!
    // void operator?:() { }     // ERROR!
    // void operator sizeof() { } // ERROR!
    // void operator typeid() { } // ERROR!
};

int main() {
    MyClass obj(42);

    // WHY: These operators work without overloading
    cout << sizeof(obj) << endl;      // Built-in sizeof
    cout << typeid(obj).name() << endl; // Built-in typeid

    int x = 10;
    int y = (x > 5) ? 20 : 30;  // Built-in ternary

    return 0;
}
```

---

## 3. Unary Operator Overloading

### 3.1 Prefix Increment/Decrement (++obj, --obj)

**Syntax**: `Type& operator++()` (no parameter)

**Returns**: Reference to allow chaining

```cpp
// prefix_operators.cpp
#include <iostream>
using namespace std;

class Counter {
private:
    int count;

public:
    Counter(int c = 0) : count(c) {}

    // WHY: Prefix ++, increments then returns
    Counter& operator++() {
        ++count;  // Increment first
        return *this;  // Return reference to self
    }

    // WHY: Prefix --, decrements then returns
    Counter& operator--() {
        --count;  // Decrement first
        return *this;
    }

    int getValue() const { return count; }
};

int main() {
    Counter c(10);

    cout << "Original: " << c.getValue() << endl;

    ++c;  // Calls c.operator++()
    cout << "After ++c: " << c.getValue() << endl;

    --c;  // Calls c.operator--()
    cout << "After --c: " << c.getValue() << endl;

    // WHY: Can chain prefix operators
    ++++c;  // Calls operator++() twice
    cout << "After ++++c: " << c.getValue() << endl;

    return 0;
}
```

### 3.2 Postfix Increment/Decrement (obj++, obj--)

**Syntax**: `Type operator++(int)` (dummy int parameter)

**Returns**: Copy (old value before increment)

**Dummy int**: Distinguishes postfix from prefix (not used)

```cpp
// postfix_operators.cpp
#include <iostream>
using namespace std;

class Counter {
private:
    int count;

public:
    Counter(int c = 0) : count(c) {}

    // WHY: Prefix ++
    Counter& operator++() {
        ++count;
        return *this;
    }

    // WHY: Postfix ++, int is dummy parameter
    Counter operator++(int) {
        Counter temp = *this;  // Save old value
        count++;               // Increment
        return temp;           // Return old value
    }

    int getValue() const { return count; }
};

int main() {
    Counter c(10);

    cout << "Original: " << c.getValue() << endl;

    // WHY: Postfix returns old value
    Counter old = c++;
    cout << "After c++, c: " << c.getValue() << endl;
    cout << "After c++, old: " << old.getValue() << endl;

    // WHY: Prefix returns new value
    Counter& ref = ++c;
    cout << "After ++c, c: " << c.getValue() << endl;
    cout << "After ++c, ref: " << ref.getValue() << endl;

    return 0;
}
```

**Output:**

```
Original: 10
After c++, c: 11
After c++, old: 10
After ++c, c: 12
After ++c, ref: 12
```

**Prefix vs Postfix:**

| Aspect | Prefix (++obj) | Postfix (obj++) |
| --- | --- | --- |
| **Syntax** | `operator++()` | `operator++(int)` |
| **Returns** | Reference | Copy |
| **Value returned** | New value | Old value |
| **Performance** | Faster (no copy) | Slower (copy created) |
| **Chaining** | Can chain | Cannot chain |
| **Recommendation** | Prefer prefix | Use only if needed |

### 3.3 Unary Minus and Plus

```cpp
// unary_plus_minus.cpp
#include <iostream>
using namespace std;

class Vector3D {
private:
    double x, y, z;

public:
    Vector3D(double a = 0, double b = 0, double c = 0) : x(a), y(b), z(c) {}

    // WHY: Unary minus - negate all components
    Vector3D operator-() const {
        return Vector3D(-x, -y, -z);
    }

    // WHY: Unary plus - return copy
    Vector3D operator+() const {
        return *this;
    }

    void display() const {
        cout << "(" << x << ", " << y << ", " << z << ")" << endl;
    }
};

int main() {
    Vector3D v1(1, 2, 3);

    cout << "v1: "; v1.display();

    Vector3D v2 = -v1;  // Unary minus
    cout << "v2 = -v1: "; v2.display();

    Vector3D v3 = +v1;  // Unary plus
    cout << "v3 = +v1: "; v3.display();

    return 0;
}
```

### 3.4 Logical NOT (!)

```cpp
// logical_not.cpp
#include <iostream>
using namespace std;

class SmartPointer {
private:
    int* ptr;

public:
    SmartPointer(int* p = nullptr) : ptr(p) {}

    // WHY: ! operator checks if pointer is null
    bool operator!() const {
        return ptr == nullptr;
    }

    ~SmartPointer() {
        delete ptr;
    }
};

int main() {
    SmartPointer sp1(new int(42));
    SmartPointer sp2(nullptr);

    if (!sp1) {
        cout << "sp1 is null" << endl;
    } else {
        cout << "sp1 is valid" << endl;
    }

    if (!sp2) {
        cout << "sp2 is null" << endl;
    } else {
        cout << "sp2 is valid" << endl;
    }

    return 0;
}
```

---

## 4. Binary Operator Overloading

### 4.1 Arithmetic Operators (+, -, *, /, %)

```cpp
// arithmetic_operators.cpp
#include <iostream>
using namespace std;

class Fraction {
private:
    int numerator, denominator;

    // WHY: Helper to simplify fraction
    int gcd(int a, int b) const {
        return b == 0 ? a : gcd(b, a % b);
    }

    void simplify() {
        int g = gcd(numerator, denominator);
        numerator /= g;
        denominator /= g;
    }

public:
    Fraction(int n = 0, int d = 1) : numerator(n), denominator(d) {
        if (d == 0) {
            cout << "Error: Division by zero" << endl;
            denominator = 1;
        }
        simplify();
    }

    // WHY: Addition a/b + c/d = (ad + bc) / bd
    Fraction operator+(const Fraction& other) const {
        int n = numerator * other.denominator + other.numerator * denominator;
        int d = denominator * other.denominator;
        return Fraction(n, d);
    }

    // WHY: Subtraction
    Fraction operator-(const Fraction& other) const {
        int n = numerator * other.denominator - other.numerator * denominator;
        int d = denominator * other.denominator;
        return Fraction(n, d);
    }

    // WHY: Multiplication
    Fraction operator*(const Fraction& other) const {
        return Fraction(numerator * other.numerator,
                       denominator * other.denominator);
    }

    // WHY: Division
    Fraction operator/(const Fraction& other) const {
        return Fraction(numerator * other.denominator,
                       denominator * other.numerator);
    }

    void display() const {
        cout << numerator << "/" << denominator;
    }
};

int main() {
    Fraction f1(1, 2);   // 1/2
    Fraction f2(1, 3);   // 1/3

    cout << "f1 = "; f1.display(); cout << endl;
    cout << "f2 = "; f2.display(); cout << endl;

    Fraction sum = f1 + f2;
    cout << "f1 + f2 = "; sum.display(); cout << endl;

    Fraction diff = f1 - f2;
    cout << "f1 - f2 = "; diff.display(); cout << endl;

    Fraction prod = f1 * f2;
    cout << "f1 * f2 = "; prod.display(); cout << endl;

    Fraction quot = f1 / f2;
    cout << "f1 / f2 = "; quot.display(); cout << endl;

    return 0;
}
```

### 4.2 Comparison Operators (==, !=, <, >, <=, >=)

```cpp
// comparison_operators.cpp
#include <iostream>
#include <cmath>
using namespace std;

class Point {
private:
    double x, y;

public:
    Point(double a = 0, double b = 0) : x(a), y(b) {}

    // WHY: Equality - same coordinates
    bool operator==(const Point& other) const {
        return (fabs(x - other.x) < 1e-9) && (fabs(y - other.y) < 1e-9);
    }

    // WHY: Inequality - not equal
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }

    // WHY: Less than - distance from origin
    bool operator<(const Point& other) const {
        double dist1 = x*x + y*y;
        double dist2 = other.x*other.x + other.y*other.y;
        return dist1 < dist2;
    }

    bool operator>(const Point& other) const {
        return other < *this;
    }

    bool operator<=(const Point& other) const {
        return !(*this > other);
    }

    bool operator>=(const Point& other) const {
        return !(*this < other);
    }

    void display() const {
        cout << "(" << x << ", " << y << ")";
    }
};

int main() {
    Point p1(3, 4);   // Distance from origin: 5
    Point p2(1, 2);   // Distance from origin: sqrt(5) ≈ 2.236
    Point p3(3, 4);

    cout << "p1: "; p1.display(); cout << endl;
    cout << "p2: "; p2.display(); cout << endl;
    cout << "p3: "; p3.display(); cout << endl;

    cout << "\np1 == p3? " << (p1 == p3 ? "Yes" : "No") << endl;
    cout << "p1 != p2? " << (p1 != p2 ? "Yes" : "No") << endl;
    cout << "p2 < p1? " << (p2 < p1 ? "Yes" : "No") << endl;
    cout << "p1 > p2? " << (p1 > p2 ? "Yes" : "No") << endl;

    return 0;
}
```

### 4.3 Compound Assignment Operators (+=, -=, *=, /=)

```cpp
// compound_assignment.cpp
#include <iostream>
using namespace std;

class Number {
private:
    int value;

public:
    Number(int v = 0) : value(v) {}

    // WHY: += modifies left operand, returns reference
    Number& operator+=(const Number& other) {
        value += other.value;
        return *this;  // Return reference for chaining
    }

    Number& operator-=(const Number& other) {
        value -= other.value;
        return *this;
    }

    Number& operator*=(const Number& other) {
        value *= other.value;
        return *this;
    }

    Number& operator/=(const Number& other) {
        if (other.value != 0) {
            value /= other.value;
        }
        return *this;
    }

    int getValue() const { return value; }
};

int main() {
    Number n1(10);
    Number n2(5);

    cout << "n1 = " << n1.getValue() << endl;
    cout << "n2 = " << n2.getValue() << endl;

    n1 += n2;
    cout << "After n1 += n2: " << n1.getValue() << endl;

    n1 *= n2;
    cout << "After n1 *= n2: " << n1.getValue() << endl;

    // WHY: Can chain compound assignments
    n1 += n2 += Number(5);
    cout << "After n1 += n2 += 5: " << n1.getValue() << endl;

    return 0;
}
```

---

## 5. Special Operators

### 5.1 Stream Insertion/Extraction (<<, >>)

**Critical Rule**: MUST be friend or global function (not member)

**Why**: Left operand is stream (cout/cin), not our object

```cpp
// stream_operators.cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int rollNo;
    double gpa;

public:
    Student(string n = "", int r = 0, double g = 0.0)
        : name(n), rollNo(r), gpa(g) {}

    // WHY: Friend to access private members
    friend ostream& operator<<(ostream& out, const Student& s);
    friend istream& operator>>(istream& in, Student& s);
};

// WHY: << for output, returns ostream& for chaining
ostream& operator<<(ostream& out, const Student& s) {
    out << "Name: " << s.name
        << ", Roll: " << s.rollNo
        << ", GPA: " << s.gpa;
    return out;  // Enable chaining: cout << s1 << s2
}

// WHY: >> for input, returns istream& for chaining
istream& operator>>(istream& in, Student& s) {
    cout << "Enter name: ";
    in >> s.name;
    cout << "Enter roll number: ";
    in >> s.rollNo;
    cout << "Enter GPA: ";
    in >> s.gpa;
    return in;  // Enable chaining: cin >> s1 >> s2
}

int main() {
    Student s1;

    cout << "=== Input Student Data ===" << endl;
    cin >> s1;

    cout << "\n=== Display Student ===" << endl;
    cout << s1 << endl;

    // WHY: Chaining works
    Student s2("Bob", 102, 3.7);
    cout << "\nMultiple students: " << endl;
    cout << s1 << endl << s2 << endl;

    return 0;
}

```

### 5.2 Subscript Operator []

**Rule**: MUST be member function

**Returns**: Reference for read/write access

```cpp
// subscript_operator.cpp
#include <iostream>
using namespace std;

class SafeArray {
private:
    int* arr;
    int size;

public:
    SafeArray(int s) : size(s) {
        arr = new int[size];
        for (int i = 0; i < size; i++) {
            arr[i] = 0;
        }
    }

    // WHY: [] operator with bounds checking
    int& operator[](int index) {
        if (index < 0 || index >= size) {
            cout << "Error: Index out of bounds!" << endl;
            static int dummy = 0;
            return dummy;
        }
        return arr[index];  // Return reference for read/write
    }

    // WHY: Const version for const objects
    const int& operator[](int index) const {
        if (index < 0 || index >= size) {
            cout << "Error: Index out of bounds!" << endl;
            static int dummy = 0;
            return dummy;
        }
        return arr[index];
    }

    int getSize() const { return size; }

    ~SafeArray() {
        delete[] arr;
    }
};

int main() {
    SafeArray arr(5);

    // WHY: Use [] like regular array
    for (int i = 0; i < arr.getSize(); i++) {
        arr[i] = i * 10;  // Calls non-const operator[]
    }

    cout << "Array elements: ";
    for (int i = 0; i < arr.getSize(); i++) {
        cout << arr[i] << " ";  // Calls non-const operator[]
    }
    cout << endl;

    // WHY: Bounds checking
    arr[10] = 100;  // Out of bounds - error message

    return 0;
}
```

### 5.3 Function Call Operator ()

**Use Case**: Make objects callable like functions (function objects/functors)

```cpp
// function_call_operator.cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

class Adder {
private:
    int increment;

public:
    Adder(int inc) : increment(inc) {}

    // WHY: () operator - makes object callable
    int operator()(int x) const {
        return x + increment;
    }
};

class Multiplier {
private:
    int factor;

public:
    Multiplier(int f) : factor(f) {}

    int operator()(int x) const {
        return x * factor;
    }
};

// WHY: Generic function using function object
template<typename Func>
void applyToAll(vector<int>& vec, Func func) {
    for (int& val : vec) {
        val = func(val);
    }
}

int main() {
    Adder add5(5);
    Multiplier mult3(3);

    // WHY: Call object like function
    cout << "add5(10) = " << add5(10) << endl;
    cout << "mult3(10) = " << mult3(10) << endl;

    vector<int> numbers = {1, 2, 3, 4, 5};

    cout << "\nOriginal: ";
    for (int n : numbers) cout << n << " ";

    applyToAll(numbers, add5);
    cout << "\nAfter add5: ";
    for (int n : numbers) cout << n << " ";

    applyToAll(numbers, mult3);
    cout << "\nAfter mult3: ";
    for (int n : numbers) cout << n << " ";
    cout << endl;

    return 0;
}
```

### 5.4 Assignment Operator (=)

**Important**: Compiler provides default, often need custom version

**Deep Copy**: For classes with dynamic memory

```cpp
// assignment_operator.cpp
#include <iostream>
#include <cstring>
using namespace std;

class String {
private:
    char* data;
    int length;

public:
    String(const char* str = "") {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
        cout << "Constructor: " << data << endl;
    }

    // WHY: Copy constructor
    String(const String& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
        cout << "Copy constructor: " << data << endl;
    }

    // WHY: Assignment operator for deep copy
    String& operator=(const String& other) {
        cout << "Assignment operator called" << endl;

        // WHY: Check self-assignment
        if (this == &other) {
            return *this;
        }

        // WHY: Delete old data
        delete[] data;

        // WHY: Allocate new data
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);

        return *this;  // Return reference for chaining
    }

    void display() const {
        cout << data;
    }

    ~String() {
        cout << "Destructor: " << data << endl;
        delete[] data;
    }
};

int main() {
    String s1("Hello");
    String s2("World");

    cout << "\nBefore assignment:" << endl;
    cout << "s1: "; s1.display(); cout << endl;
    cout << "s2: "; s2.display(); cout << endl;

    s2 = s1;  // Calls operator=

    cout << "\nAfter s2 = s1:" << endl;
    cout << "s1: "; s1.display(); cout << endl;
    cout << "s2: "; s2.display(); cout << endl;

    // WHY: Chaining
    String s3("Test");
    s3 = s2 = s1;
    cout << "\nAfter s3 = s2 = s1:" << endl;
    cout << "s3: "; s3.display(); cout << endl;

    return 0;
}
```

---

## 6. Member vs Friend Function

### 6.1 Comparison

| Aspect | Member Function | Friend Function |
| --- | --- | --- |
| **Access** | Has `this` pointer | No `this`, needs parameters |
| **Left operand** | Must be class object | Can be any type |
| **Parameters** | One less (this implicit) | All explicit |
| **Syntax** | `obj.operator@(arg)` | `operator@(obj, arg)` |
| **Required for** | =, [], (), -> | Stream <<, >> |
| **Preferred for** | Unary, compound assignment | Binary, commutative ops |

### 6.2 Member Function Overloading

```cpp
// member_function_overload.cpp
#include <iostream>
using namespace std;

class Vector {
private:
    double x, y;

public:
    Vector(double a = 0, double b = 0) : x(a), y(b) {}

    // WHY: Member function - left operand is *this
    Vector operator+(const Vector& other) const {
        return Vector(x + other.x, y + other.y);
    }

    // WHY: Member function for +=
    Vector& operator+=(const Vector& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    void display() const {
        cout << "(" << x << ", " << y << ")";
    }
};

int main() {
    Vector v1(1, 2);
    Vector v2(3, 4);

    // WHY: v1.operator+(v2)
    Vector v3 = v1 + v2;

    cout << "v1: "; v1.display(); cout << endl;
    cout << "v2: "; v2.display(); cout << endl;
    cout << "v3 = v1 + v2: "; v3.display(); cout << endl;

    return 0;
}
```

### 6.3 Friend Function Overloading

```cpp
// friend_function_overload.cpp
#include <iostream>
using namespace std;

class Complex {
private:
    double real, imag;

public:
    Complex(double r = 0, double i = 0) : real(r), imag(i) {}

    // WHY: Friend for symmetric operations
    friend Complex operator+(const Complex& c1, const Complex& c2);
    friend Complex operator+(double d, const Complex& c);
    friend Complex operator+(const Complex& c, double d);

    friend ostream& operator<<(ostream& out, const Complex& c);
};

// WHY: Both operands explicit parameters
Complex operator+(const Complex& c1, const Complex& c2) {
    return Complex(c1.real + c2.real, c1.imag + c2.imag);
}

// WHY: Enable double + Complex
Complex operator+(double d, const Complex& c) {
    return Complex(d + c.real, c.imag);
}

// WHY: Enable Complex + double
Complex operator+(const Complex& c, double d) {
    return Complex(c.real + d, c.imag);
}

ostream& operator<<(ostream& out, const Complex& c) {
    out << c.real << " + " << c.imag << "i";
    return out;
}

int main() {
    Complex c1(3, 4);
    Complex c2(1, 2);

    // WHY: All combinations work
    Complex c3 = c1 + c2;      // Complex + Complex
    Complex c4 = c1 + 5.0;     // Complex + double
    Complex c5 = 5.0 + c1;     // double + Complex

    cout << "c1: " << c1 << endl;
    cout << "c2: " << c2 << endl;
    cout << "c3 = c1 + c2: " << c3 << endl;
    cout << "c4 = c1 + 5: " << c4 << endl;
    cout << "c5 = 5 + c1: " << c5 << endl;

    return 0;
}
```

### 6.4 When to Use Which?

**Use Member Function When:**

- Modifying left operand (+=, -=, *=, etc.)
- Unary operators (++, --, -, !)
- Required by language (=, [], (), ->)
- Left operand always class type

**Use Friend Function When:**

- Stream operators (<<, >>)
- Commutative operations (need double + Complex and Complex + double)
- Left operand can be non-class type
- Symmetric binary operations

---

## 7. Best Practices and Common Pitfalls

### 7.1 Best Practices

**1. Return Types**

```cpp
// WHY: Proper return types
Vector operator+(const Vector& other) const;  // Return by value
Vector& operator+=(const Vector& other);      // Return by reference
bool operator==(const Vector& other) const;   // Return bool

```

**2. Const Correctness**

```cpp
// WHY: Mark as const if doesn't modify object
bool operator==(const Complex& other) const;
Complex operator+(const Complex& other) const;

```

**3. Self-Assignment Check**

```cpp
MyClass& operator=(const MyClass& other) {
    if (this == &other) return *this;  // Check self-assignment
    // ... rest of code
}
```

**4. Symmetry for Commutative Operators**

```cpp
// WHY: Support both orders
friend Complex operator+(const Complex& c, double d);
friend Complex operator+(double d, const Complex& c);
```

**5. Consistency**

```cpp
// WHY: If overload ==, also overload !=
bool operator==(const T& other) const;
bool operator!=(const T& other) const { return !(*this == other); }
```

### 7.2 Common Pitfalls

**Pitfall 1: Wrong Return Type**

```cpp
// ❌ BAD: void return
void operator++() {  // Cannot chain
    ++value;
}

// ✅ GOOD: Reference return
Counter& operator++() {
    ++value;
    return *this;
}
```

**Pitfall 2: Forgetting Self-Assignment**

```cpp
// ❌ BAD: No self-assignment check
String& operator=(const String& other) {
    delete[] data;        // Deletes own data!
    data = new char[...];
    strcpy(data, other.data);  // Undefined behavior
}

// ✅ GOOD: Check first
String& operator=(const String& other) {
    if (this == &other) return *this;
    // ... safe copy
}
```

**Pitfall 3: Non-const When Should Be**

```cpp
// ❌ BAD: Modifies object
bool operator==(const T& other) {  // Not const
    return value == other.value;
}

// ✅ GOOD: Marked const
bool operator==(const T& other) const {
    return value == other.value;
}
```

**Pitfall 4: Stream Operators as Member**

```cpp
// ❌ BAD: Cannot make << member function
ostream& operator<<(ostream& out) {  // obj << cout ???
    out << value;
    return out;
}

// ✅ GOOD: Friend or global function
friend ostream& operator<<(ostream& out, const T& obj);
```

**Pitfall 5: Ignoring Operator Precedence**

```cpp
// Cannot change precedence!
// * always higher precedence than +
// a + b * c always means a + (b * c)
```

### 7.3 Complete Example

```cpp
// complete_example.cpp
#include <iostream>
using namespace std;

class Rational {
private:
    int numerator, denominator;

    int gcd(int a, int b) const {
        return b == 0 ? a : gcd(b, a % b);
    }

    void simplify() {
        int g = gcd(abs(numerator), abs(denominator));
        numerator /= g;
        denominator /= g;
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
    }

public:
    Rational(int n = 0, int d = 1) : numerator(n), denominator(d) {
        if (d == 0) denominator = 1;
        simplify();
    }

    // Arithmetic operators
    Rational operator+(const Rational& r) const {
        return Rational(numerator * r.denominator + r.numerator * denominator,
                       denominator * r.denominator);
    }

    Rational operator-(const Rational& r) const {
        return Rational(numerator * r.denominator - r.numerator * denominator,
                       denominator * r.denominator);
    }

    Rational operator*(const Rational& r) const {
        return Rational(numerator * r.numerator, denominator * r.denominator);
    }

    Rational operator/(const Rational& r) const {
        return Rational(numerator * r.denominator, denominator * r.numerator);
    }

    // Compound assignment
    Rational& operator+=(const Rational& r) {
        *this = *this + r;
        return *this;
    }

    // Unary operators
    Rational operator-() const {
        return Rational(-numerator, denominator);
    }

    // Comparison operators
    bool operator==(const Rational& r) const {
        return numerator == r.numerator && denominator == r.denominator;
    }

    bool operator!=(const Rational& r) const {
        return !(*this == r);
    }

    bool operator<(const Rational& r) const {
        return numerator * r.denominator < r.numerator * denominator;
    }

    // Stream operators
    friend ostream& operator<<(ostream& out, const Rational& r) {
        if (r.denominator == 1) {
            out << r.numerator;
        } else {
            out << r.numerator << "/" << r.denominator;
        }
        return out;
    }

    friend istream& operator>>(istream& in, Rational& r) {
        char slash;
        in >> r.numerator >> slash >> r.denominator;
        r.simplify();
        return in;
    }
};

int main() {
    Rational r1(1, 2);
    Rational r2(1, 3);

    cout << "r1 = " << r1 << endl;
    cout << "r2 = " << r2 << endl;

    cout << "r1 + r2 = " << (r1 + r2) << endl;
    cout << "r1 - r2 = " << (r1 - r2) << endl;
    cout << "r1 * r2 = " << (r1 * r2) << endl;
    cout << "r1 / r2 = " << (r1 / r2) << endl;

    cout << "r1 == r2? " << (r1 == r2 ? "Yes" : "No") << endl;
    cout << "r1 < r2? " << (r1 < r2 ? "Yes" : "No") << endl;

    return 0;
}
```

---

## Summary

### Key Takeaways

1. **Operator Overloading Concept** - Giving new meaning to operators for user-defined types. Syntax: `returnType operator@(params)`. Benefits: natural syntax (c1+c2 vs c1.add(c2)), intuitive code, mathematical types. Compile-time polymorphism. Makes custom types behave like built-in types.
2. **Overloadable Operators** - Can overload: arithmetic (+,-,*,/,%), comparison (==,!=,<,>,<=,>=), logical (&&,||,!), bitwise (&,|,^,~,<<,>>), assignment (=,+=,-=), increment/decrement (++,--), special ([],(),->). Cannot overload: :: (scope), . (member access), .* (pointer-to-member), ?: (ternary), sizeof, typeid, alignof. Reason: operate on names not values, fundamental language features.
3. **Unary Operators** - Operate on single operand. Prefix (++obj): `Type& operator++()`, no parameter, returns reference, increments then returns. Postfix (obj++): `Type operator++(int)`, dummy int parameter, returns copy (old value), increments after returning. Prefix faster (no copy), prefer when possible. Also: unary minus (-), plus (+), logical NOT (!).
4. **Binary Operators** - Operate on two operands. Arithmetic: +,-,*,/,% for mathematical operations. Comparison: ==,!=,<,>,<=,>= for ordering. Compound assignment: +=,-=,*=,/= modify left operand, return reference for chaining. All take one explicit parameter in member function (left operand is *this).
5. **Stream Operators** - << (output) and >> (input). MUST be friend or global function (not member) because left operand is stream (cout/cin) not our object. Return stream reference for chaining: `cout << obj1 << obj2`. Syntax: `ostream& operator<<(ostream&, const T&)` and `istream& operator>>(istream&, T&)`.
6. **Subscript Operator []** - MUST be member function. Returns reference for read/write access: `int& operator[](int)`. Implement const and non-const versions. Use for bounds checking in array classes. Example: SafeArray checks bounds before returning element reference.
7. **Function Call Operator ()** - Makes objects callable like functions (functors). Syntax: `RetType operator()(params)`. Use for: function objects, callbacks, STL algorithms. Example: `Adder add5(5); cout << add5(10);` outputs 15. Powerful for creating customizable function-like objects.
8. **Assignment Operator =** - MUST be member function. Compiler provides default (shallow copy). Need custom for: dynamic memory (deep copy), resource management. Check self-assignment: `if(this==&other) return *this;`. Delete old data, allocate new, copy. Return reference: `T& operator=(const T&)` for chaining.
9. **Member vs Friend** - Member: has this pointer, left operand is object, one less parameter, use for: unary, +=, required (=,[],(),->). Friend: no this, both operands explicit, use for: <<,>>, commutative operations (double+Complex), symmetric binary. Friend accesses private members but not member of class.
10. **Best Practices** - Return by value for binary arithmetic (+), by reference for compound (+=), bool for comparison. Mark const if doesn't modify. Check self-assignment in operator=. Implement symmetric operations for commutative ops. If overload ==, also overload !=. Prefer member for unary/compound, friend for stream/symmetric. Never change operator precedence/associativity.

### Interview Essential Questions

**Q1: What is operator overloading? Why use it? Give real-world example with code.**

A: Operator overloading is giving new meaning to existing operators when used with user-defined types (classes). Allows custom types to use operators like built-in types.

Why use: (1) Natural syntax: `c1 + c2` more intuitive than `c1.add(c2)`, (2) Mathematical types: Complex numbers, Vectors, Matrices behave like numbers, (3) Code readability: `cout << obj` clearer than `obj.print()`, (4) Compile-time polymorphism.

Real example - Complex number addition:

```cpp
class Complex {
    double real, imag;
public:
    Complex(double r, double i) : real(r), imag(i) {}
    Complex operator+(const Complex& c) const {
        return Complex(real + c.real, imag + c.imag);
    }
};
// Usage: Complex c3 = c1 + c2; // Natural!
```

Without overloading: `Complex c3 = c1.add(c2);` less intuitive. Operator overloading makes Complex behave like int or double, improving code clarity and usability.

**Q2: Which operators cannot be overloaded? Why? List at least 5.**

A: Cannot overload: (1) :: (scope resolution), (2) . (member access), (3) .* (pointer-to-member), (4) ?: (ternary conditional), (5) sizeof, (6) typeid, (7) alignof.

Reasons: (1) Scope resolution (::) operates on names not values, no syntax to capture as expression. (2) Member access (.) fundamental to language, overloading would break basic object access. (3) Pointer-to-member (.*) same reason as dot. (4) Ternary (?:) has special evaluation - only one branch evaluated based on condition, cannot guarantee with overloading. (5) sizeof evaluated at compile time, determines memory layout, overloading would break language fundamentals. (6) typeid for RTTI must uniquely identify types, overloading would cause serious issues. (7) alignof compile-time, fundamental for memory layout.

All relate to fundamental language structure that must remain consistent for language to work properly.

**Q3: Explain prefix vs postfix increment operator overloading. Which is better and why?**

A: Prefix (++obj): Syntax `Type& operator++()`, no parameter, increments value first then returns reference to incremented object. Returns new value. Can chain: `++++obj`.

Postfix (obj++): Syntax `Type operator++(int)`, dummy int parameter distinguishes from prefix, creates copy of old value, increments object, returns old copy. Returns old value. Cannot chain efficiently.

Example:

```cpp
class Counter {
    int val;
public:
    Counter& operator++() { ++val; return *this; } // Prefix
    Counter operator++(int) { // Postfix
        Counter temp = *this; // Copy old
        ++val;               // Increment
        return temp;         // Return old
    }
};
```

Prefix better: (1) More efficient - no temporary copy created, (2) Can chain operations, (3) Recommended by C++ Core Guidelines. Use postfix only when specifically need old value: `arr[i++]`.

Performance: prefix ~1 operation, postfix ~3 operations (copy, increment, return). For objects with expensive copy (strings, containers), difference significant. Always prefer prefix unless specifically need postfix behavior.

**Q4: Why must stream operators (<<, >>) be friend or global functions? Why not member functions?**

A: Stream operators must be friend/global because left operand is stream (cout/cin), not our object.

If member function: Syntax would be `obj.operator<<(cout)`, called as `obj << cout` - backwards! User expects `cout << obj` not `obj << cout`.

Correct approach - friend function:

```cpp
class Student {
    string name;
    int age;
public:
    friend ostream& operator<<(ostream& out, const Student& s) {
        out << s.name << ", " << s.age;
        return out; // Return stream for chaining
    }
};
// Usage: cout << student; // Correct order
```

Why friend: needs access to private members (name, age). Alternative: public getters, then can be global function.

Return stream reference: enables chaining `cout << s1 << s2 << endl;`. Each operator<< call returns cout, allowing next call.

Both parameters: ostream& (stream) and const T& (object). Cannot be member because cannot modify ostream class (it's in std library). This pattern standard for all custom types.

**Q5: Compare member function vs friend function operator overloading. When use each? Give specific examples.**

A: Member function: Has implicit this pointer for left operand, one parameter (right operand), syntax `obj.operator@(arg)`. Friend function: No this pointer, both operands explicit parameters, syntax `operator@(obj1, obj2)`, declared in class with friend keyword.

When use member: (1) Modifying operators: +=, -=, *=, /= (must modify left), (2) Unary operators: ++, --, -, ! (only one operand is this), (3) Required by language: =, [], (), -> must be members, (4) Left operand always your class type.

When use friend: (1) Stream operators: <<, >> (left operand is stream), (2) Symmetric operations: need both `Complex+double` and `double+Complex`, (3) Commutative binary ops for natural syntax, (4) Left operand might not be your class.

Example - symmetric addition:

```cpp
class Complex {
    friend Complex operator+(const Complex& c1, const Complex& c2);
    friend Complex operator+(double d, const Complex& c); // double+Complex
    friend Complex operator+(const Complex& c, double d); // Complex+double
};
// Now works: Complex c = 5.0 + c1; and c = c1 + 5.0;
```

Member version only supports: `c1 + 5.0` not `5.0 + c1` because 5.0 is not Complex object, cannot call its methods. Friend solves this asymmetry.

---

**Summary:**

- Part 5.1: Classes, Objects, Constructors, Destructors, this pointer
- Part 5.2: Static members, Friend functions
- Part 5.3: Abstraction, Pure virtual, Abstract classes, Interfaces
- Part 5.4: Inheritance types, Diamond problem, Polymorphism, vtable/vptr
- Part 5.5: Operator Overloading ← YOU ARE HERE