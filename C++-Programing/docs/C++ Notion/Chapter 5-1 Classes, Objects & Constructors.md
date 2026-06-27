# 5.1. Classes, Objects & Constructors

---

## Table of Contents

1. Introduction to Object-Oriented Programming
2. Classes and Objects
3. Access Specifiers and Encapsulation
4. Constructors
5. Destructors
6. The this Pointer
7. Summary

---

## 1. Introduction to Object-Oriented Programming

### 1.1 What is OOP?

**Definition**: Programming paradigm based on "objects" containing data (attributes) and code (methods).

**Why OOP?**

- Models real-world entities naturally
- Better code organization and maintainability
- Code reusability through inheritance
- Data protection through encapsulation
- Flexibility through polymorphism

**Four Pillars of OOP:**

1. **Encapsulation** - Bundling data and methods, hiding implementation
2. **Abstraction** - Showing only essential features
3. **Inheritance** - Deriving new classes from existing ones
4. **Polymorphism** - Same interface, different implementations

**C++ OOP Note**: C++ is **multi-paradigm**, not pure OOP (unlike Java/Smalltalk). You can write procedural, OOP, generic, or functional code.

```cpp
// oop_vs_procedural.cpp
#include <iostream>
using namespace std;

// WHY: Procedural approach - functions operate on data
void proceduralExample() {
    int balance = 1000;

    auto deposit = [](int& bal, int amount) {
        bal += amount;
    };

    deposit(balance, 500);
    cout << "Procedural balance: " << balance << endl;
}

// WHY: OOP approach - data and functions bundled together
class BankAccount {
private:
    int balance;  // Data hidden

public:
    BankAccount(int bal) : balance(bal) {}

    void deposit(int amount) {
        balance += amount;
    }

    int getBalance() const {
        return balance;
    }
};

void oopExample() {
    BankAccount account(1000);
    account.deposit(500);
    cout << "OOP balance: " << account.getBalance() << endl;
}

int main() {
    proceduralExample();
    oopExample();
    return 0;
}
```

---

## 2. Classes and Objects

### 2.1 Understanding Classes and Objects

**Class**: Blueprint/template for creating objects

**Object**: Instance of a class (actual entity in memory)

**Analogy**:

- Class = Car blueprint (design specifications)
- Object = Actual car manufactured (physical entity)

**Why This Matters**: Classes define structure once, create multiple objects efficiently.

```cpp
// class_object_basics.cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Class defines the structure (blueprint)
class Car {
public:
    // Data members (attributes/properties)
    string brand;
    string model;
    int year;
    double price;

    // Member functions (methods/behaviors)
    void displayInfo() {
        cout << "=== Car Information ===" << endl;
        cout << "Brand: " << brand << endl;
        cout << "Model: " << model << endl;
        cout << "Year: " << year << endl;
        cout << "Price: $" << price << endl;
    }

    void startEngine() {
        cout << brand << " " << model << " engine started!" << endl;
    }
};

int main() {
    // WHY: Objects are instances with actual data
    Car car1;  // Object 1
    car1.brand = "Toyota";
    car1.model = "Camry";
    car1.year = 2023;
    car1.price = 25000;

    Car car2;  // Object 2 (independent from car1)
    car2.brand = "Honda";
    car2.model = "Civic";
    car2.year = 2024;
    car2.price = 23000;

    cout << "Car 1:" << endl;
    car1.displayInfo();
    car1.startEngine();

    cout << "\nCar 2:" << endl;
    car2.displayInfo();
    car2.startEngine();

    return 0;
}
```

### 2.2 Class vs Struct in C++

**Key Difference**: Default access specifier

| Feature | Class | Struct |
| --- | --- | --- |
| **Default access** | private | public |
| **Typical use** | Complex types with behavior | Simple data aggregation |
| **Inheritance default** | private | public |
| **Convention** | Use for OOP | Use for POD types |

```cpp
// class_vs_struct.cpp
#include <iostream>
using namespace std;

// WHY: Struct - members public by default
struct PointStruct {
    int x;  // Public
    int y;  // Public

    void display() {  // Public
        cout << "Point(" << x << ", " << y << ")" << endl;
    }
};

// WHY: Class - members private by default
class PointClass {
    int x;  // Private
    int y;  // Private

public:
    void setX(int val) { x = val; }
    void setY(int val) { y = val; }

    void display() {
        cout << "Point(" << x << ", " << y << ")" << endl;
    }
};

int main() {
    // Struct: Direct access
    PointStruct s;
    s.x = 10;  // OK - public by default
    s.y = 20;
    s.display();

    // Class: Need public interface
    PointClass c;
    // c.x = 10;  // ERROR - private by default
    c.setX(10);  // OK - through public method
    c.setY(20);
    c.display();

    return 0;
}
```

**When to Use Which:**

✅ **Use `struct` when:**

- Simple data container (POD - Plain Old Data)
- All members should be public
- No complex behavior
- C compatibility needed

✅ **Use `class` when:**

- Need encapsulation (data hiding)
- Complex behavior/methods
- Implementing OOP principles

### 2.3 Defining Classes

**Two Ways to Define Member Functions:**

```cpp
// class_definition_styles.cpp
#include <iostream>
using namespace std;

// Style 1: Define inside class (implicit inline)
class Rectangle1 {
private:
    int width, height;

public:
    void setDimensions(int w, int h) {
        width = w;
        height = h;
    }

    int area() {
        return width * height;  // Defined inside
    }
};

// Style 2: Declare inside, define outside (more common for large classes)
class Rectangle2 {
private:
    int width, height;

public:
    void setDimensions(int w, int h);  // Declaration only
    int area();  // Declaration only
};

// WHY: Scope resolution operator (::) links definition to class
void Rectangle2::setDimensions(int w, int h) {
    width = w;
    height = h;
}

int Rectangle2::area() {
    return width * height;
}

int main() {
    Rectangle1 r1;
    r1.setDimensions(10, 5);
    cout << "R1 Area: " << r1.area() << endl;

    Rectangle2 r2;
    r2.setDimensions(8, 6);
    cout << "R2 Area: " << r2.area() << endl;

    return 0;
}
```

### 2.4 Object Creation

**Two Ways to Create Objects:**

```cpp
// object_creation.cpp
#include <iostream>
#include <string>
using namespace std;

class Person {
public:
    string name;
    int age;

    void introduce() {
        cout << "Hi, I'm " << name << ", " << age << " years old." << endl;
    }
};

int main() {
    // WHY: Stack allocation - automatic lifetime management
    Person person1;
    person1.name = "Alice";
    person1.age = 25;
    person1.introduce();

    // WHY: Heap allocation - manual lifetime control
    Person* person2 = new Person();
    person2->name = "Bob";  // Use -> for pointers
    person2->age = 30;
    person2->introduce();

    // WHY: Must manually delete heap objects
    delete person2;

    // person1 automatically destroyed when goes out of scope

    return 0;
}
```

---

## 3. Access Specifiers and Encapsulation

### 3.1 Access Specifiers

**Purpose**: Control who can access class members.

**Three Access Levels:**

| Specifier | Accessible From | Purpose |
| --- | --- | --- |
| **public** | Anywhere | Interface to outside world |
| **private** | Same class only | Hide implementation details |
| **protected** | Same class + derived classes | For inheritance |

```cpp
// access_specifiers.cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    // WHY: Private - cannot be accessed directly from outside
    double balance;
    string accountNumber;

protected:
    // WHY: Protected - accessible in derived classes
    string accountType;

public:
    // WHY: Public - interface to interact with account
    BankAccount(string accNum, double initialBalance) {
        accountNumber = accNum;
        balance = initialBalance;
        accountType = "Savings";
    }

    void deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            cout << "Deposited: $" << amount << endl;
        }
    }

    bool withdraw(double amount) {
        if (amount > 0 && amount <= balance) {
            balance -= amount;
            cout << "Withdrew: $" << amount << endl;
            return true;
        }
        cout << "Insufficient balance!" << endl;
        return false;
    }

    double getBalance() const {
        return balance;
    }
};

int main() {
    BankAccount account("ACC123", 1000.0);

    // Can access public methods
    account.deposit(500);
    account.withdraw(200);
    cout << "Balance: $" << account.getBalance() << endl;

    // Cannot access private members
    // account.balance = 9999;  // ERROR!
    // account.accountNumber = "HACK";  // ERROR!

    return 0;
}
```

### 3.2 Encapsulation

**Definition**: Bundling data and methods together, hiding internal details.

**Why Encapsulation?**

- **Data Protection**: Prevent invalid states
- **Flexibility**: Change implementation without affecting users
- **Maintainability**: Clear interface, hidden complexity
- **Validation**: Control how data is modified

```cpp
// encapsulation_example.cpp
#include <iostream>
#include <string>
using namespace std;

class Employee {
private:
    string name;
    double salary;
    int age;

public:
    // Constructor
    Employee(string n, double s, int a) : name(n), salary(s), age(a) {}

    // WHY: Getters - controlled read access
    string getName() const {
        return name;
    }

    double getSalary() const {
        return salary;
    }

    int getAge() const {
        return age;
    }

    // WHY: Setters - controlled write access with validation
    void setSalary(double newSalary) {
        if (newSalary >= 0) {
            salary = newSalary;
            cout << "Salary updated to: $" << salary << endl;
        } else {
            cout << "Invalid salary amount!" << endl;
        }
    }

    void setAge(int newAge) {
        if (newAge >= 18 && newAge <= 100) {
            age = newAge;
        } else {
            cout << "Invalid age!" << endl;
        }
    }

    void displayInfo() const {
        cout << "Name: " << name << endl;
        cout << "Age: " << age << endl;
        cout << "Salary: $" << salary << endl;
    }
};

int main() {
    Employee emp("John Doe", 50000, 30);

    // WHY: Can only access through public interface
    emp.displayInfo();

    // WHY: Validation happens in setter
    emp.setSalary(55000);   // OK
    emp.setSalary(-1000);   // Rejected

    emp.setAge(35);         // OK
    emp.setAge(150);        // Rejected

    emp.displayInfo();

    return 0;
}
```

### 3.3 const Member Functions

**Purpose**: Promise not to modify object state.

**When to Use**: All getter methods and read-only operations.

```cpp
// const_member_functions.cpp
#include <iostream>
using namespace std;

class Point {
private:
    int x, y;

public:
    Point(int xVal, int yVal) : x(xVal), y(yVal) {}

    // WHY: const function - doesn't modify data members
    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    // WHY: const function can be called on const objects
    void display() const {
        cout << "Point(" << x << ", " << y << ")" << endl;
        // x = 10;  // ERROR! Cannot modify in const function
    }

    // WHY: Non-const function - can modify members
    void setX(int val) {
        x = val;
    }

    void setY(int val) {
        y = val;
    }
};

int main() {
    Point p1(10, 20);
    p1.display();  // OK
    p1.setX(15);   // OK

    // WHY: const object can only call const member functions
    const Point p2(30, 40);
    p2.display();  // OK - const function
    cout << p2.getX() << endl;  // OK - const function
    // p2.setX(35);  // ERROR! Non-const function on const object

    return 0;
}
```

---

## 4. Constructors

### 4.1 What is a Constructor?

**Definition**: Special member function that initializes object when created.

**Characteristics:**

- Same name as class
- No return type (not even `void`)
- Called automatically when object created
- Can be overloaded (multiple constructors)

**Why Constructors?**

- Guarantee proper initialization
- Prevent uninitialized objects
- Simplify object creation
- Allocate resources

```cpp
// constructor_basics.cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int id;
    double gpa;

public:
    // WHY: Constructor ensures object is properly initialized
    Student(string n, int i, double g) {
        name = n;
        id = i;
        gpa = g;
        cout << "Student created: " << name << endl;
    }

    void displayInfo() const {
        cout << "Name: " << name << ", ID: " << id << ", GPA: " << gpa << endl;
    }
};

int main() {
    // WHY: Constructor automatically called during creation
    Student s1("Alice", 101, 3.8);  // Constructor called
    s1.displayInfo();

    Student s2("Bob", 102, 3.5);    // Constructor called
    s2.displayInfo();

    return 0;
}
```

### 4.2 Types of Constructors

**1. Default Constructor**

```cpp
// default_constructor.cpp
#include <iostream>
using namespace std;

class Box {
private:
    int width, height, depth;

public:
    // WHY: Default constructor - no parameters
    Box() {
        width = 1;
        height = 1;
        depth = 1;
        cout << "Default constructor called" << endl;
    }

    int volume() const {
        return width * height * depth;
    }
};

int main() {
    Box b1;  // Calls default constructor
    cout << "Volume: " << b1.volume() << endl;

    // If no constructor defined, compiler generates default constructor

    return 0;
}
```

**2. Parameterized Constructor**

```cpp
// parameterized_constructor.cpp
#include <iostream>
using namespace std;

class Rectangle {
private:
    int width, height;

public:
    // WHY: Parameterized constructor - accepts arguments
    Rectangle(int w, int h) {
        width = w;
        height = h;
        cout << "Parameterized constructor: " << w << "x" << h << endl;
    }

    int area() const {
        return width * height;
    }
};

int main() {
    Rectangle r1(10, 5);   // Calls parameterized constructor
    Rectangle r2(8, 12);

    cout << "R1 Area: " << r1.area() << endl;
    cout << "R2 Area: " << r2.area() << endl;

    return 0;
}
```

**3. Copy Constructor**

```cpp
// copy_constructor.cpp
#include <iostream>
#include <cstring>
using namespace std;

class String {
private:
    char* data;
    int length;

public:
    // WHY: Regular constructor
    String(const char* str) {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
        cout << "Constructor: " << data << endl;
    }

    // WHY: Copy constructor - creates copy of object
    // Takes const reference to avoid infinite recursion
    String(const String& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
        cout << "Copy constructor: " << data << endl;
    }

    ~String() {
        delete[] data;
        cout << "Destructor: " << data << endl;
    }

    void display() const {
        cout << "String: " << data << endl;
    }
};

int main() {
    String s1("Hello");      // Regular constructor
    String s2 = s1;          // Copy constructor
    String s3(s1);           // Copy constructor (explicit)

    s1.display();
    s2.display();
    s3.display();

    return 0;
}
```

**Why Copy Constructor is Needed:**

```cpp
// why_copy_constructor.cpp
#include <iostream>
using namespace std;

class Array {
private:
    int* data;
    int size;

public:
    Array(int s) : size(s) {
        data = new int[size];
        cout << "Array created" << endl;
    }

    // WHY: Without proper copy constructor, shallow copy occurs
    // Compiler-generated copy constructor only copies pointer value!

    // Proper copy constructor (deep copy)
    Array(const Array& other) : size(other.size) {
        data = new int[size];  // Allocate new memory
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];  // Copy values
        }
        cout << "Deep copy created" << endl;
    }

    ~Array() {
        delete[] data;
        cout << "Array destroyed" << endl;
    }

    void setValue(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }

    int getValue(int index) const {
        return (index >= 0 && index < size) ? data[index] : -1;
    }
};

int main() {
    Array arr1(5);
    arr1.setValue(0, 100);

    Array arr2 = arr1;  // Copy constructor called
    arr2.setValue(0, 200);

    // WHY: Deep copy - independent arrays
    cout << "arr1[0]: " << arr1.getValue(0) << endl;  // Still 100
    cout << "arr2[0]: " << arr2.getValue(0) << endl;  // Now 200

    return 0;
}
```

### 4.3 Constructor Overloading

```cpp
// constructor_overloading.cpp
#include <iostream>
#include <string>
using namespace std;

class Date {
private:
    int day, month, year;

public:
    // WHY: Default constructor
    Date() {
        day = 1;
        month = 1;
        year = 2000;
        cout << "Default constructor" << endl;
    }

    // WHY: Constructor with all parameters
    Date(int d, int m, int y) {
        day = d;
        month = m;
        year = y;
        cout << "Full parameterized constructor" << endl;
    }

    // WHY: Constructor with day only (month, year default to current)
    Date(int d) {
        day = d;
        month = 1;
        year = 2024;
        cout << "Single parameter constructor" << endl;
    }

    // WHY: Constructor with day and month
    Date(int d, int m) {
        day = d;
        month = m;
        year = 2024;
        cout << "Two parameter constructor" << endl;
    }

    void display() const {
        cout << day << "/" << month << "/" << year << endl;
    }
};

int main() {
    Date d1;              // Default
    Date d2(25);          // Single param
    Date d3(15, 8);       // Two params
    Date d4(10, 12, 2023);  // All params

    d1.display();
    d2.display();
    d3.display();
    d4.display();

    return 0;
}
```

### 4.4 Member Initializer List

**Purpose**: Initialize members before constructor body executes.

**When Required:**

- const members
- Reference members
- Base class with no default constructor
- Member objects with no default constructor

**Benefits:**

- More efficient (direct initialization vs assignment)
- Only way for const/reference members

```cpp
// initializer_list.cpp
#include <iostream>
#include <string>
using namespace std;

class Person {
private:
    const int id;        // WHY: const member - must use initializer list
    string& name;        // WHY: reference - must use initializer list
    int age;

public:
    // WHY: Member initializer list (after colon)
    Person(int i, string& n, int a) : id(i), name(n), age(a) {
        cout << "Person created: " << name << endl;
        // id = i;     // ERROR! const cannot be assigned
        // name = n;   // ERROR! reference already bound
    }

    void display() const {
        cout << "ID: " << id << ", Name: " << name << ", Age: " << age << endl;
    }
};

// WHY: Demonstrates efficiency difference
class Point {
private:
    int x, y;

public:
    // Inefficient: assignment in body
    Point(int xVal, int yVal) {
        x = xVal;  // Default initialization then assignment
        y = yVal;
    }

    // Efficient: direct initialization
    Point(int xVal, int yVal) : x(xVal), y(yVal) {
        // Members already initialized
    }
};

int main() {
    string name = "Alice";
    Person p(101, name, 25);
    p.display();

    return 0;
}

```

**Initialization Order**: Members initialized in **declaration order**, not initializer list order!

```cpp
// initialization_order.cpp
#include <iostream>
using namespace std;

class Demo {
private:
    int a;
    int b;

public:
    // WHY: Order in initializer list doesn't matter
    // Members initialized in declaration order: a then b
    Demo(int x) : b(x), a(b + 1) {  // WRONG! b used before initialized
        cout << "a: " << a << ", b: " << b << endl;
    }

    // Correct version
    Demo(int x, int) : a(x), b(a + 1) {  // OK - a initialized first
        cout << "a: " << a << ", b: " << b << endl;
    }
};

int main() {
    Demo d1(10);      // Undefined behavior
    Demo d2(10, 0);   // Correct

    return 0;
}
```

### 4.5 Delegating Constructors (C++11)

**Purpose**: Constructor calling another constructor to reduce code duplication.

```cpp
// delegating_constructors.cpp
#include <iostream>
#include <string>
using namespace std;

class Rectangle {
private:
    int width, height;
    string color;

public:
    // WHY: Main constructor with all initialization logic
    Rectangle(int w, int h, string c) : width(w), height(h), color(c) {
        cout << "Main constructor: " << w << "x" << h << ", " << c << endl;
    }

    // WHY: Delegates to main constructor (C++11)
    Rectangle(int w, int h) : Rectangle(w, h, "White") {
        cout << "Delegated from 2-param constructor" << endl;
    }

    // WHY: Delegates to 2-param constructor
    Rectangle(int side) : Rectangle(side, side) {
        cout << "Delegated from 1-param constructor" << endl;
    }

    void display() const {
        cout << "Rectangle: " << width << "x" << height
             << ", Color: " << color << endl;
    }
};

int main() {
    Rectangle r1(10, 5, "Red");  // Main constructor
    Rectangle r2(8, 6);           // Delegates to main
    Rectangle r3(7);              // Delegates twice

    r1.display();
    r2.display();
    r3.display();

    return 0;
}
```

### 4.6 explicit Keyword

**Purpose**: Prevent implicit conversions.

**Why Needed**: Avoid accidental type conversions that may hide bugs.

```cpp
// explicit_constructor.cpp
#include <iostream>
using namespace std;

class Distance {
private:
    int meters;

public:
    // WHY: Without explicit, allows implicit conversion
    Distance(int m) : meters(m) {}

    void display() const {
        cout << "Distance: " << meters << "m" << endl;
    }
};

class SafeDistance {
private:
    int meters;

public:
    // WHY: explicit prevents implicit conversion
    explicit SafeDistance(int m) : meters(m) {}

    void display() const {
        cout << "Safe Distance: " << meters << "m" << endl;
    }
};

void processDistance(Distance d) {
    d.display();
}

void processSafeDistance(SafeDistance d) {
    d.display();
}

int main() {
    // Without explicit
    Distance d1(100);      // OK - normal construction
    Distance d2 = 200;     // OK - implicit conversion (int to Distance)
    processDistance(300);  // OK - implicit conversion

    // With explicit
    SafeDistance sd1(100);           // OK
    // SafeDistance sd2 = 200;       // ERROR! No implicit conversion
    // processSafeDistance(300);     // ERROR! No implicit conversion

    processSafeDistance(SafeDistance(300));  // OK - explicit conversion

    return 0;
}
```

---

## 5. Destructors

### 5.1 What is a Destructor?

**Definition**: Special member function that cleans up when object destroyed.

**Characteristics:**

- Same name as class with `~` prefix
- No parameters, no return type
- Called automatically
- Only one per class (no overloading)

**When Called:**

- Object goes out of scope
- `delete` called on dynamic object
- Program ends

**Why Destructors?**

- Release resources (memory, files, locks)
- Cleanup operations
- RAII pattern implementation

```cpp
// destructor_basics.cpp
#include <iostream>
using namespace std;

class Resource {
private:
    int* data;
    int size;

public:
    // WHY: Constructor acquires resource
    Resource(int s) : size(s) {
        data = new int[size];
        cout << "Resource allocated (size: " << size << ")" << endl;
    }

    // WHY: Destructor releases resource
    ~Resource() {
        delete[] data;
        cout << "Resource deallocated" << endl;
    }

    void setValue(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }
};

int main() {
    {
        Resource r(10);
        r.setValue(0, 100);
        cout << "Using resource..." << endl;
    }  // WHY: Destructor called here (r goes out of scope)

    cout << "After scope" << endl;

    return 0;
}
```

### 5.2 Constructor and Destructor Order

**Rule**: Objects destroyed in **reverse order of construction** (LIFO - Last In, First Out).

**Why**: Ensures proper cleanup of dependencies.

```cpp
// ctor_dtor_order.cpp
#include <iostream>
using namespace std;

class Demo {
private:
    string name;

public:
    Demo(string n) : name(n) {
        cout << "Constructor: " << name << endl;
    }

    ~Demo() {
        cout << "Destructor: " << name << endl;
    }
};

int main() {
    cout << "=== Program start ===" << endl;

    Demo obj1("Object1");
    Demo obj2("Object2");

    {
        Demo obj3("Object3");
        Demo obj4("Object4");
        cout << "Inner scope" << endl;
    }  // obj4, obj3 destroyed here

    cout << "=== End of main ===" << endl;
    return 0;
    // obj2, obj1 destroyed here
}
```

**Output:**

```
=== Program start ===
Constructor: Object1
Constructor: Object2
Constructor: Object3
Constructor: Object4
Inner scope
Destructor: Object4
Destructor: Object3
=== End of main ===
Destructor: Object2
Destructor: Object1
```

### 5.3 RAII Pattern with Destructors

**RAII**: Resource Acquisition Is Initialization

**Principle**: Tie resource lifetime to object lifetime.

```cpp
// raii_pattern.cpp
#include <iostream>
#include <fstream>
using namespace std;

class FileHandler {
private:
    ofstream file;
    string filename;

public:
    // WHY: Constructor opens file (acquires resource)
    FileHandler(const string& fname) : filename(fname) {
        file.open(filename);
        if (file.is_open()) {
            cout << "File opened: " << filename << endl;
        } else {
            cout << "Failed to open file" << endl;
        }
    }

    // WHY: Destructor closes file (releases resource)
    ~FileHandler() {
        if (file.is_open()) {
            file.close();
            cout << "File closed: " << filename << endl;
        }
    }

    void write(const string& data) {
        if (file.is_open()) {
            file << data << endl;
        }
    }
};

int main() {
    {
        FileHandler fh("test.txt");
        fh.write("Hello, World!");
        fh.write("RAII pattern in action");

        // WHY: No need to manually close file
        // Destructor automatically called at scope end
    }  // File closed here

    cout << "File operations completed safely" << endl;

    return 0;
}
```

---

## 6. The this Pointer

### 6.1 Understanding this

**Definition**: Implicit pointer available in all non-static member functions pointing to the calling object.

**Type**: For class `X`, `this` has type `X*` (or `const X*` in const functions).

**Why this Exists:**

- Distinguish between parameters and members with same name
- Return current object for method chaining
- Pass current object to other functions
- Explicit access to members

```cpp
// this_pointer_basics.cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;

public:
    // WHY: this resolves name conflict between parameter and member
    Student(string name, int age) {
        this->name = name;  // this->name is member, name is parameter
        this->age = age;
    }

    // WHY: this is implicit - these are equivalent
    void display1() {
        cout << "Name: " << name << endl;  // Implicit this->name
        cout << "Age: " << age << endl;
    }

    void display2() {
        cout << "Name: " << this->name << endl;  // Explicit this->
        cout << "Age: " << this->age << endl;
    }

    // WHY: Return *this for method chaining
    Student& setName(string name) {
        this->name = name;
        return *this;  // Return reference to current object
    }

    Student& setAge(int age) {
        this->age = age;
        return *this;
    }
};

int main() {
    Student s("Alice", 20);
    s.display1();

    // WHY: Method chaining using return *this
    s.setName("Bob").setAge(25);  // Chained calls
    s.display2();

    return 0;
}
```

### 6.2 this in const Member Functions

```cpp
// this_const.cpp
#include <iostream>
using namespace std;

class Example {
private:
    int value;

public:
    Example(int v) : value(v) {}

    // WHY: Non-const function - this has type: Example*
    void modify() {
        this->value = 100;  // OK - can modify
    }

    // WHY: const function - this has type: const Example*
    void display() const {
        cout << "Value: " << this->value << endl;  // OK - can read
        // this->value = 200;  // ERROR! Cannot modify through const this
    }

    // WHY: Returning const reference for const function
    const Example& getThis() const {
        return *this;
    }
};

int main() {
    Example e(42);
    e.modify();
    e.display();

    const Example ce(99);
    // ce.modify();  // ERROR! Cannot call non-const function on const object
    ce.display();    // OK - const function

    return 0;
}
```

### 6.3 Method Chaining with this

```cpp
// method_chaining.cpp
#include <iostream>
#include <string>
using namespace std;

class StringBuilder {
private:
    string text;

public:
    // WHY: Return *this to enable chaining
    StringBuilder& append(const string& str) {
        text += str;
        return *this;
    }

    StringBuilder& appendLine(const string& str) {
        text += str + "\n";
        return *this;
    }

    StringBuilder& clear() {
        text.clear();
        return *this;
    }

    string build() const {
        return text;
    }
};

int main() {
    StringBuilder sb;

    // WHY: Fluent interface - method chaining
    string result = sb.append("Hello")
                      .append(" ")
                      .append("World")
                      .appendLine("!")
                      .append("This is ")
                      .append("chaining.")
                      .build();

    cout << result << endl;

    return 0;
}
```

---

## Summary

### Key Takeaways

1. **Classes vs Objects** - Class is blueprint/template defining structure and behavior. Object is instance with actual data. One class creates many independent objects. Use `struct` for simple data, `class` for OOP with encapsulation.
2. **Access Specifiers** - `public` (accessible everywhere - interface), `private` (class only - implementation hiding), `protected` (class + derived classes - inheritance). Default is private for class, public for struct. Encapsulation achieved through private data, public methods.
3. **Encapsulation** - Bundling data and methods, hiding implementation details. Benefits: data protection with validation, flexibility to change implementation, maintainability through clear interfaces, controlled access via getters/setters. Core principle of OOP.
4. **const Member Functions** - Promise not to modify object state (type of `this` becomes `const X*`). Required for const objects. Use for all getters and read-only operations. Compiler enforces immutability - cannot modify members or call non-const functions.
5. **Constructors** - Special function initializing objects: same name as class, no return type, called automatically. Types: default (no params), parameterized (with params), copy (creates copy). Always use initializer list for const/reference members and efficiency.
6. **Constructor Overloading** - Multiple constructors with different parameters. Compiler selects based on arguments. Use delegating constructors (C++11) to reduce duplication. `explicit` keyword prevents implicit conversions, catching bugs.
7. **Copy Constructor** - Creates new object as copy: signature `ClassName(const ClassName&)`. Compiler generates default (shallow copy). Define custom for deep copy when class has pointers/resources. Called on: initialization (`Type obj2 = obj1`), pass-by-value, return-by-value.
8. **Destructors** - Cleanup function: `~ClassName()`, no params/return, called automatically. One per class, no overloading. Called when: scope ends, delete called, program terminates. Destruction order: reverse of construction (LIFO). Essential for RAII pattern.
9. **RAII Pattern** - Resource Acquisition Is Initialization: acquire resources in constructor, release in destructor. Automatic cleanup, exception-safe, prevents leaks. Foundation of modern C++. Examples: smart pointers, file streams, mutex locks.
10. **this Pointer** - Implicit pointer to current object in non-static member functions. Type: `X*` for class X (or `const X*` in const functions). Uses: resolve name conflicts, method chaining (return `this`), pass current object. Cannot use in static functions.

### Interview Essential Questions

**Q1: What is the difference between class and struct in C++?**

A: Only difference is default access: class members are private by default, struct members are public. Both support all OOP features (inheritance, polymorphism, constructors). Convention: use struct for simple POD (Plain Old Data) types, class for complex types with behavior and encapsulation.

For inheritance, class uses private inheritance by default, struct uses public. Historically, struct existed in C for data grouping, class added for OOP in C++. Best practice: use class when you need data hiding (private members), use struct for transparent data containers where all members should be public.

**Q2: Explain encapsulation and why it's important. Provide examples of when you would use it.**

A: Encapsulation is bundling data and methods together while hiding implementation details through access specifiers. Data is private, accessed only through public methods (getters/setters).

Importance: (1) Data protection - prevents invalid states through validation in setters, (2) Flexibility - can change internal implementation without breaking external code, (3) Maintainability - clear interface separates usage from implementation, (4) Security - sensitive data hidden from direct access.

Example: BankAccount class with private balance, public deposit/withdraw methods with validation. Cannot set negative balance directly, must use methods that enforce business rules. Can change internal storage (array to database) without affecting users.

**Q3: What is a copy constructor? When is it called? Why might you need to write your own?**

A: Copy constructor creates new object as copy of existing object. Signature: `ClassName(const ClassName& other)`. Parameter must be const reference to avoid infinite recursion.

Called in three situations: (1) Initialization: `Type obj2 = obj1;`, (2) Pass-by-value to function, (3) Return-by-value from function.

Need custom copy constructor when class manages resources (pointers, file handles). Compiler-generated does shallow copy (copies pointer values), causing problems: both objects point to same memory, double deletion when destroyed. Custom copy constructor does deep copy: allocates new memory, copies actual data. Rule of Three: if you define destructor, copy constructor, or copy assignment, define all three.

**Q4: Explain the this pointer. When and why would you use it explicitly?**

A: `this` is implicit pointer available in all non-static member functions, pointing to the object that called the function. Type is `X*` for class X (or `const X*` in const functions).

Use explicitly for: (1) Resolve name conflicts when parameter names match member names: `this->name = name`, (2) Method chaining - return `*this` to chain calls: `obj.setX(10).setY(20)`, (3) Pass current object to other functions, (4) Explicit member access for clarity.

In const member functions, this becomes `const X*`, preventing modification. Cannot use this in static functions (no object context). Common pattern: returning reference to `*this` enables fluent interfaces (builder pattern, stream operations).

**Q5: What is RAII? How do constructors and destructors enable it?**

A: RAII (Resource Acquisition Is Initialization) ties resource lifetime to object lifetime. Resources acquired in constructor, released in destructor automatically.

How it works: Constructor allocates resources (memory, files, locks) and initializes object. When object goes out of scope or is deleted, destructor automatically called, releasing resources. No manual cleanup needed.

Benefits: (1) Automatic cleanup - impossible to forget, (2) Exception-safe - destructor called during stack unwinding even if exception thrown, (3) No resource leaks, (4) Clear ownership semantics.

Examples: `std::unique_ptr` (memory), `std::ifstream` (files), `std::lock_guard` (mutexes), custom resource wrappers. RAII is foundation of modern C++ resource management, preferred over manual new/delete or malloc/free.

---