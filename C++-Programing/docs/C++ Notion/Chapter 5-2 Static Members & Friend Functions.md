# 5.2. Static Members & Friend Functions

---

## Table of Contents

1. Static Data Members
2. Static Member Functions
3. Friend Functions
4. Friend Classes
5. The mutable Keyword
6. Summary

---

## 1. Static Data Members

### 1.1 Understanding Static Data Members

**Definition**: Class member shared by ALL objects of the class (only one copy exists).

**Why Static Data Members?**

- Share data across all instances
- Class-level properties (counters, configuration)
- Constant class data
- Memory efficiency (single copy)

**Key Characteristics:**

- Only one copy for entire class
- Initialized at compile time (outside class)
- Exists even if no objects created
- Accessible via class name or object
- Cannot be initialized in constructor

```cpp
// static_data_members_basics.cpp
#include <iostream>
using namespace std;

class Counter {
private:
    static int objectCount;  // WHY: Shared by all objects
    int id;

public:
    Counter() {
        objectCount++;       // Increment for each object
        id = objectCount;
        cout << "Object " << id << " created" << endl;
    }

    ~Counter() {
        cout << "Object " << id << " destroyed" << endl;
        objectCount--;
    }

    static int getCount() {  // Static function to access static data
        return objectCount;
    }

    int getID() const {
        return id;
    }
};

// WHY: Static members MUST be defined outside class
int Counter::objectCount = 0;  // Initialization

int main() {
    cout << "Initial count: " << Counter::getCount() << endl;

    {
        Counter c1;
        Counter c2;
        Counter c3;

        cout << "\nCurrent count: " << Counter::getCount() << endl;
    }  // c3, c2, c1 destroyed here

    cout << "\nFinal count: " << Counter::getCount() << endl;

    return 0;
}
```

**Output:**

```
Initial count: 0
Object 1 created
Object 2 created
Object 3 created

Current count: 3
Object 3 destroyed
Object 2 destroyed
Object 1 destroyed

Final count: 0
```

### 1.2 Static vs Non-Static Members

**Comparison:**

| Aspect | Static Member | Non-Static Member |
| --- | --- | --- |
| **Copies** | One copy for class | One copy per object |
| **Memory** | Data/BSS segment | Object memory |
| **Access** | Via class name or object | Only via object |
| **Initialization** | Outside class | In constructor |
| **Lifetime** | Program lifetime | Object lifetime |
| **this pointer** | Not available | Available |

```cpp
// static_vs_nonstatic.cpp
#include <iostream>
using namespace std;

class BankAccount {
private:
    // WHY: Non-static - each object has own balance
    double balance;

    // WHY: Static - shared interest rate for all accounts
    static double interestRate;

public:
    BankAccount(double bal) : balance(bal) {}

    void applyInterest() {
        // WHY: Can access both static and non-static members
        balance += balance * interestRate;
    }

    void displayBalance() const {
        cout << "Balance: $" << balance
             << " (Rate: " << interestRate * 100 << "%)" << endl;
    }

    // WHY: Static function to change rate for ALL accounts
    static void setInterestRate(double rate) {
        interestRate = rate;
    }

    static double getInterestRate() {
        return interestRate;
    }
};

// WHY: Must initialize static member outside
double BankAccount::interestRate = 0.05;  // 5% default

int main() {
    BankAccount acc1(1000);
    BankAccount acc2(2000);

    cout << "Initial balances:" << endl;
    acc1.displayBalance();
    acc2.displayBalance();

    // WHY: Change rate affects ALL accounts
    BankAccount::setInterestRate(0.08);  // 8% for everyone

    cout << "\nAfter rate change:" << endl;
    acc1.applyInterest();
    acc2.applyInterest();

    acc1.displayBalance();
    acc2.displayBalance();

    return 0;
}
```

### 1.3 Static const Members

**Two Initialization Methods:**

```cpp
// static_const_members.cpp
#include <iostream>
#include <string>
using namespace std;

class Config {
public:
    // WHY: Method 1 - In-class initialization (C++11)
    // Only for static const integral/enum types
    static const int MAX_USERS = 100;
    static const int MIN_AGE = 18;

    // WHY: Method 2 - Must initialize outside for non-integral
    static const string APP_NAME;
    static const double VERSION;

    // WHY: C++17 - inline for any type
    static inline const string AUTHOR = "GeeksforGeeks";
};

// Initialize non-integral static const outside
const string Config::APP_NAME = "MyApp";
const double Config::VERSION = 1.5;

int main() {
    cout << "App: " << Config::APP_NAME << endl;
    cout << "Version: " << Config::VERSION << endl;
    cout << "Max Users: " << Config::MAX_USERS << endl;
    cout << "Author: " << Config::AUTHOR << endl;

    return 0;
}
```

### 1.4 Real-World Application

```cpp
// real_world_static.cpp
#include <iostream>
#include <string>
using namespace std;

class Database {
private:
    string name;
    static int connectionCount;
    static const int MAX_CONNECTIONS = 10;

public:
    Database(string n) : name(n) {
        if (connectionCount < MAX_CONNECTIONS) {
            connectionCount++;
            cout << "Database '" << name << "' connected. "
                 << "Total connections: " << connectionCount << endl;
        } else {
            cout << "Cannot connect - max connections reached!" << endl;
        }
    }

    ~Database() {
        connectionCount--;
        cout << "Database '" << name << "' disconnected. "
             << "Remaining: " << connectionCount << endl;
    }

    static int getConnectionCount() {
        return connectionCount;
    }

    static int getMaxConnections() {
        return MAX_CONNECTIONS;
    }
};

int Database::connectionCount = 0;

int main() {
    cout << "Max allowed: " << Database::getMaxConnections() << endl;
    cout << endl;

    Database db1("MySQL");
    Database db2("PostgreSQL");
    Database db3("MongoDB");

    cout << "\nCurrent connections: "
         << Database::getConnectionCount() << endl;

    return 0;
}
```

---

## 2. Static Member Functions

### 2.1 Understanding Static Member Functions

**Definition**: Function that belongs to the class, not to objects.

**Characteristics:**

- Can be called without object (via class name)
- No `this` pointer
- Can only access static members
- Cannot be `const`, `virtual`, or `volatile`
- Cannot access non-static members

**Why Static Member Functions?**

- Utility functions for class
- Factory methods
- Access static data
- Class-level operations

```cpp
// static_member_functions.cpp
#include <iostream>
using namespace std;

class Math {
public:
    // WHY: Static utility functions - don't need object
    static int add(int a, int b) {
        return a + b;
    }

    static int multiply(int a, int b) {
        return a * b;
    }

    static double power(double base, int exp) {
        double result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
};

int main() {
    // WHY: Call without creating object
    cout << "5 + 3 = " << Math::add(5, 3) << endl;
    cout << "5 * 3 = " << Math::multiply(5, 3) << endl;
    cout << "2^8 = " << Math::power(2, 8) << endl;

    // Can also call via object (but not recommended)
    Math m;
    cout << "Via object: " << m.add(10, 20) << endl;

    return 0;
}
```

### 2.2 Restrictions on Static Member Functions

```cpp
// static_function_restrictions.cpp
#include <iostream>
using namespace std;

class Example {
private:
    int nonStaticData;
    static int staticData;

public:
    Example(int val) : nonStaticData(val) {}

    // WHY: Static function - no this pointer
    static void staticFunction() {
        cout << "Static function called" << endl;

        // CAN access static members
        cout << "Static data: " << staticData << endl;

        // CANNOT access non-static members
        // cout << nonStaticData << endl;  // ERROR!

        // CANNOT use this pointer
        // this->nonStaticData = 10;  // ERROR!

        // CAN call other static functions
        anotherStaticFunction();

        // CANNOT call non-static functions
        // nonStaticFunction();  // ERROR!
    }

    static void anotherStaticFunction() {
        cout << "Another static function" << endl;
    }

    void nonStaticFunction() {
        cout << "Non-static function" << endl;

        // CAN access everything
        cout << "Non-static data: " << nonStaticData << endl;
        cout << "Static data: " << staticData << endl;

        // CAN call static functions
        staticFunction();
    }
};

int Example::staticData = 100;

int main() {
    // Call static function without object
    Example::staticFunction();

    cout << endl;

    // Call non-static needs object
    Example obj(42);
    obj.nonStaticFunction();

    return 0;
}
```

### 2.3 Factory Pattern with Static Functions

```cpp
// factory_pattern.cpp
#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    string level;

    // WHY: Private constructor - force use of factory methods
    Logger(string l) : level(l) {}

public:
    // WHY: Static factory methods
    static Logger createDebugLogger() {
        return Logger("DEBUG");
    }

    static Logger createInfoLogger() {
        return Logger("INFO");
    }

    static Logger createErrorLogger() {
        return Logger("ERROR");
    }

    void log(string message) {
        cout << "[" << level << "] " << message << endl;
    }
};

int main() {
    // WHY: Cannot create directly (private constructor)
    // Logger log("WARN");  // ERROR!

    // WHY: Must use factory methods
    Logger debugLog = Logger::createDebugLogger();
    Logger infoLog = Logger::createInfoLogger();
    Logger errorLog = Logger::createErrorLogger();

    debugLog.log("Debug message");
    infoLog.log("Info message");
    errorLog.log("Error message");

    return 0;
}
```

### 2.4 Singleton Pattern

```cpp
// singleton_pattern.cpp
#include <iostream>
using namespace std;

class Singleton {
private:
    static Singleton* instance;
    int data;

    // WHY: Private constructor prevents direct instantiation
    Singleton() : data(0) {
        cout << "Singleton instance created" << endl;
    }

    // WHY: Delete copy constructor and assignment
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    // WHY: Static method to get single instance
    static Singleton* getInstance() {
        if (instance == nullptr) {
            instance = new Singleton();
        }
        return instance;
    }

    void setData(int val) {
        data = val;
    }

    int getData() const {
        return data;
    }

    static void destroyInstance() {
        if (instance != nullptr) {
            delete instance;
            instance = nullptr;
            cout << "Singleton instance destroyed" << endl;
        }
    }
};

// WHY: Initialize static pointer to nullptr
Singleton* Singleton::instance = nullptr;

int main() {
    // WHY: Get singleton instance
    Singleton* s1 = Singleton::getInstance();
    s1->setData(100);

    // WHY: Same instance returned
    Singleton* s2 = Singleton::getInstance();
    cout << "s2 data: " << s2->getData() << endl;  // Still 100

    // WHY: s1 and s2 point to same object
    cout << "Same instance? " << (s1 == s2 ? "Yes" : "No") << endl;

    Singleton::destroyInstance();

    return 0;
}
```

---

## 3. Friend Functions

### 3.1 Understanding Friend Functions

**Definition**: Non-member function that can access private/protected members of a class.

**Why Friend Functions?**

- Operator overloading (especially binary operators)
- Bridge between two classes
- Utility functions needing private access
- Better syntax for some operations

**When to Use:**
✅ **Use friend when:**

- Implementing binary operators (e.g., `obj1 + obj2`)
- Function needs access to multiple classes
- More natural syntax than member function
- External function needs private access

❌ **Avoid friend when:**

- Can be implemented as public member
- Breaks encapsulation unnecessarily
- Alternative design possible

```cpp
// friend_function_basics.cpp
#include <iostream>
using namespace std;

class Box {
private:
    double width;
    double height;
    double depth;

public:
    Box(double w, double h, double d) : width(w), height(h), depth(d) {}

    // WHY: Declare function as friend
    friend double calculateVolume(const Box& box);
    friend void displayDimensions(const Box& box);
};

// WHY: Friend function - can access private members
double calculateVolume(const Box& box) {
    return box.width * box.height * box.depth;
}

void displayDimensions(const Box& box) {
    cout << "Width: " << box.width << endl;
    cout << "Height: " << box.height << endl;
    cout << "Depth: " << box.depth << endl;
}

int main() {
    Box myBox(10.5, 20.3, 15.2);

    // WHY: Call friend functions like regular functions
    cout << "Volume: " << calculateVolume(myBox) << endl;
    cout << endl;
    displayDimensions(myBox);

    return 0;
}
```

### 3.2 Friend Function for Multiple Classes

```cpp
// friend_multiple_classes.cpp
#include <iostream>
using namespace std;

// Forward declaration
class ClassB;

class ClassA {
private:
    int valueA;

public:
    ClassA(int val) : valueA(val) {}

    // WHY: Friend function accessing two classes
    friend int addValues(const ClassA& a, const ClassB& b);
};

class ClassB {
private:
    int valueB;

public:
    ClassB(int val) : valueB(val) {}

    // WHY: Same friend declaration in both classes
    friend int addValues(const ClassA& a, const ClassB& b);
};

// WHY: Friend function can access private members of both
int addValues(const ClassA& a, const ClassB& b) {
    return a.valueA + b.valueB;
}

int main() {
    ClassA objA(10);
    ClassB objB(20);

    cout << "Sum: " << addValues(objA, objB) << endl;

    return 0;
}
```

### 3.3 Friend Function vs Member Function

```cpp
// friend_vs_member.cpp
#include <iostream>
using namespace std;

class Complex {
private:
    double real, imag;

public:
    Complex(double r = 0, double i = 0) : real(r), imag(i) {}

    // WHY: Member function - only left operand can be Complex
    Complex addMember(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    // WHY: Friend function - both operands can be Complex
    friend Complex addFriend(const Complex& c1, const Complex& c2);

    void display() const {
        cout << real << " + " << imag << "i" << endl;
    }
};

Complex addFriend(const Complex& c1, const Complex& c2) {
    return Complex(c1.real + c2.real, c1.imag + c2.imag);
}

int main() {
    Complex c1(3, 4);
    Complex c2(1, 2);

    // WHY: Member function syntax
    Complex r1 = c1.addMember(c2);  // c1.add(c2)
    cout << "Member: ";
    r1.display();

    // WHY: Friend function syntax (more natural)
    Complex r2 = addFriend(c1, c2);  // add(c1, c2)
    cout << "Friend: ";
    r2.display();

    return 0;
}
```

### 3.4 Friend for Operator Overloading

```cpp
// friend_operator_overloading.cpp
#include <iostream>
using namespace std;

class Point {
private:
    int x, y;

public:
    Point(int xVal = 0, int yVal = 0) : x(xVal), y(yVal) {}

    // WHY: Friend for symmetric operators
    friend Point operator+(const Point& p1, const Point& p2);
    friend ostream& operator<<(ostream& out, const Point& p);
    friend istream& operator>>(istream& in, Point& p);
};

// WHY: Friend allows natural syntax: p1 + p2
Point operator+(const Point& p1, const Point& p2) {
    return Point(p1.x + p2.x, p1.y + p2.y);
}

// WHY: Stream operators must be non-member
ostream& operator<<(ostream& out, const Point& p) {
    out << "(" << p.x << ", " << p.y << ")";
    return out;
}

istream& operator>>(istream& in, Point& p) {
    cout << "Enter x: ";
    in >> p.x;
    cout << "Enter y: ";
    in >> p.y;
    return in;
}

int main() {
    Point p1(3, 4);
    Point p2(1, 2);

    // WHY: Natural syntax with friend
    Point p3 = p1 + p2;

    cout << "p1: " << p1 << endl;
    cout << "p2: " << p2 << endl;
    cout << "p3: " << p3 << endl;

    return 0;
}
```

---

## 4. Friend Classes

### 4.1 Understanding Friend Classes

**Definition**: Class that can access private/protected members of another class.

**Characteristics:**

- Friendship is not mutual (A friend of B ≠ B friend of A)
- Not inherited (friend of base ≠ friend of derived)
- Not transitive (A friend of B, B friend of C ≠ A friend of C)

```cpp
// friend_class_basics.cpp
#include <iostream>
#include <string>
using namespace std;

class Engine {
private:
    int horsepower;
    double displacement;

public:
    Engine(int hp, double disp) : horsepower(hp), displacement(disp) {}

    // WHY: Declare Car as friend class
    friend class Car;
};

class Car {
private:
    string brand;
    string model;
    Engine engine;

public:
    Car(string b, string m, int hp, double disp)
        : brand(b), model(m), engine(hp, disp) {}

    void displayInfo() const {
        cout << "Car: " << brand << " " << model << endl;

        // WHY: Can access private members of Engine
        cout << "Engine: " << engine.horsepower << "hp, "
             << engine.displacement << "L" << endl;
    }
};

int main() {
    Car myCar("Toyota", "Camry", 268, 3.5);
    myCar.displayInfo();

    return 0;
}
```

### 4.2 Friendship is Not Mutual

```cpp
// friendship_not_mutual.cpp
#include <iostream>
using namespace std;

class ClassB;  // Forward declaration

class ClassA {
private:
    int dataA;

public:
    ClassA(int val) : dataA(val) {}

    // WHY: B is friend of A (B can access A's private)
    friend class ClassB;

    void display() {
        cout << "ClassA data: " << dataA << endl;
    }

    void accessB(const ClassB& b) {
        // cout << b.dataB << endl;  // ERROR! A is NOT friend of B
    }
};

class ClassB {
private:
    int dataB;

public:
    ClassB(int val) : dataB(val) {}

    void accessA(const ClassA& a) {
        // WHY: B can access A's private (B is friend of A)
        cout << "Accessing ClassA::dataA from B: " << a.dataA << endl;
    }
};

int main() {
    ClassA objA(10);
    ClassB objB(20);

    objB.accessA(objA);  // OK - B is friend of A
    // objA.accessB(objB);  // Would fail - A is NOT friend of B

    return 0;
}
```

### 4.3 Mutual Friendship

```cpp
// mutual_friendship.cpp
#include <iostream>
using namespace std;

class ClassB;  // Forward declaration

class ClassA {
private:
    int dataA;

public:
    ClassA(int val) : dataA(val) {}

    // WHY: Declare B as friend
    friend class ClassB;

    void accessB(const ClassB& b);  // Declaration only

    void display() const {
        cout << "ClassA::dataA = " << dataA << endl;
    }
};

class ClassB {
private:
    int dataB;

public:
    ClassB(int val) : dataB(val) {}

    // WHY: Declare A as friend (mutual friendship)
    friend class ClassA;

    void accessA(const ClassA& a) {
        cout << "B accessing A::dataA = " << a.dataA << endl;
    }

    void display() const {
        cout << "ClassB::dataB = " << dataB << endl;
    }
};

// WHY: Define after both classes declared
void ClassA::accessB(const ClassB& b) {
    cout << "A accessing B::dataB = " << b.dataB << endl;
}

int main() {
    ClassA objA(100);
    ClassB objB(200);

    objA.accessB(objB);  // OK - A is friend of B
    objB.accessA(objA);  // OK - B is friend of A

    return 0;
}
```

### 4.4 Member Function as Friend

```cpp
// member_function_friend.cpp
#include <iostream>
using namespace std;

class ClassB;  // Forward declaration

class ClassA {
private:
    int dataA;

public:
    ClassA(int val) : dataA(val) {}

    void display() const {
        cout << "ClassA::dataA = " << dataA << endl;
    }

    void accessB(const ClassB& b);  // Declaration
};

class ClassB {
private:
    int dataB;

public:
    ClassB(int val) : dataB(val) {}

    // WHY: Only specific member function of A is friend
    friend void ClassA::accessB(const ClassB& b);

    void display() const {
        cout << "ClassB::dataB = " << dataB << endl;
    }
};

// WHY: Define after ClassB is complete
void ClassA::accessB(const ClassB& b) {
    cout << "A::accessB accessing B::dataB = " << b.dataB << endl;
}

int main() {
    ClassA objA(10);
    ClassB objB(20);

    objA.accessB(objB);  // OK - specific function is friend

    return 0;
}
```

---

## 5. The mutable Keyword

### 5.1 Understanding mutable

**Definition**: Allows modification of member variable in const member function.

**Why mutable?**

- Caching computed values
- Lazy initialization
- Internal state that doesn't affect external state
- Mutex locks in const functions

**When to Use:**

- Counters (access count, cache hits)
- Cached/memoized values
- Debug/logging data
- Thread synchronization primitives

```cpp
// mutable_basics.cpp
#include <iostream>
using namespace std;

class Counter {
private:
    int value;
    mutable int accessCount;  // WHY: Can modify in const functions

public:
    Counter(int val) : value(val), accessCount(0) {}

    // WHY: const function can modify mutable member
    int getValue() const {
        accessCount++;  // OK - mutable
        // value++;     // ERROR! - non-mutable
        return value;
    }

    int getAccessCount() const {
        return accessCount;
    }
};

int main() {
    const Counter c(42);  // const object

    // WHY: Can call const function on const object
    cout << "Value: " << c.getValue() << endl;
    cout << "Value: " << c.getValue() << endl;
    cout << "Value: " << c.getValue() << endl;

    // WHY: Access count modified even though object is const
    cout << "Access count: " << c.getAccessCount() << endl;

    return 0;
}
```

### 5.2 Caching with mutable

```cpp
// mutable_caching.cpp
#include <iostream>
#include <cmath>
using namespace std;

class Circle {
private:
    double radius;

    // WHY: Cached values - mutable for lazy computation
    mutable double cachedArea;
    mutable bool areaComputed;

public:
    Circle(double r) : radius(r), areaComputed(false) {}

    void setRadius(double r) {
        radius = r;
        areaComputed = false;  // Invalidate cache
    }

    // WHY: const function but can cache result
    double getArea() const {
        if (!areaComputed) {
            cout << "Computing area..." << endl;
            cachedArea = 3.14159 * radius * radius;
            areaComputed = true;  // OK - mutable
        } else {
            cout << "Using cached area..." << endl;
        }
        return cachedArea;
    }
};

int main() {
    const Circle c(5.0);

    // WHY: First call computes
    cout << "Area: " << c.getArea() << endl;

    // WHY: Subsequent calls use cache
    cout << "Area: " << c.getArea() << endl;
    cout << "Area: " << c.getArea() << endl;

    return 0;
}
```

### 5.3 Real-World Example: Database Query Cache

```cpp
// mutable_real_world.cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Database {
private:
    string connectionString;

    // WHY: Cache that doesn't affect logical const-ness
    mutable vector<string> queryCache;
    mutable int cacheHits;
    mutable int cacheMisses;

public:
    Database(string conn)
        : connectionString(conn), cacheHits(0), cacheMisses(0) {}

    // WHY: Logically const (doesn't change DB state)
    // But modifies cache internally
    vector<string> executeQuery(string query) const {
        // Check cache
        for (const auto& cached : queryCache) {
            if (cached == query) {
                cacheHits++;  // OK - mutable
                cout << "[CACHE HIT] " << query << endl;
                return {"cached", "result"};
            }
        }

        // Not in cache
        cacheMisses++;  // OK - mutable
        cout << "[CACHE MISS] Executing: " << query << endl;

        // Simulate query execution
        vector<string> result = {"data1", "data2"};

        // Cache result
        queryCache.push_back(query);  // OK - mutable

        return result;
    }

    void printStats() const {
        cout << "\n=== Cache Statistics ===" << endl;
        cout << "Hits: " << cacheHits << endl;
        cout << "Misses: " << cacheMisses << endl;
        cout << "Hit rate: "
             << (cacheHits * 100.0 / (cacheHits + cacheMisses))
             << "%" << endl;
    }
};

int main() {
    const Database db("server:1234");

    db.executeQuery("SELECT * FROM users");
    db.executeQuery("SELECT * FROM orders");
    db.executeQuery("SELECT * FROM users");     // Cache hit
    db.executeQuery("SELECT * FROM products");
    db.executeQuery("SELECT * FROM orders");    // Cache hit

    db.printStats();

    return 0;
}
```

---

## Summary

### Key Takeaways

1. **Static Data Members** - Shared by all objects, only one copy exists in memory. Must initialize outside class definition. Useful for: object counting, class-level configuration, shared resources. Access via `ClassName::member` or object. Lifetime is entire program.
2. **Static Member Functions** - Belong to class, not objects. Can call without creating instance. No `this` pointer - can only access static members. Cannot be virtual, const, or volatile. Use cases: utility functions, factory methods, singleton pattern, class-level operations.
3. **Static vs Non-Static** - Static: one copy, data/BSS segment, class scope, program lifetime. Non-static: per-object copy, object memory, object scope, object lifetime. Static functions cannot access non-static members, but non-static can access static.
4. **Friend Functions** - Non-member functions with private/protected access. Use for: operator overloading (especially binary like +, <<), bridging classes, natural syntax. Not member functions - no `this` pointer. Declared with `friend` keyword in class, defined normally outside.
5. **Friend vs Member** - Friend function: symmetric operators (`a+b` = `b+a`), access multiple classes, more natural syntax for some operations. Member function: has `this`, one less parameter, preferred when operation belongs to class. Choose friend for operators where left operand isn't always class type.
6. **Friend Classes** - Entire class gets private/protected access. Friendship is: NOT mutual (A friend of B ≠ B friend of A), NOT inherited (derived doesn't inherit friendship), NOT transitive (A→B, B→C ≠ A→C). Use cautiously - breaks encapsulation. Good for tightly coupled classes.
7. **Member Function as Friend** - Can make specific member function (not whole class) a friend. Requires forward declaration and careful ordering. More restrictive than friend class - better encapsulation. Example: `friend void ClassA::specificFunction();`
8. **mutable Keyword** - Allows modification in const member functions. Use for: caching (lazy evaluation), counters (access tracking), internal bookkeeping, mutex locks. Doesn't affect logical const-ness. Example: `mutable int accessCount;` can be modified in const functions.
9. **When to Use Static** - Use static data for: class-level constants, counters across instances, shared configuration. Use static functions for: utility functions (no object needed), factory methods, singleton getInstance(), operations not dependent on object state.
10. **When to Use Friend** - Use friend function for: operator overloading (especially << and >>), functions needing access to multiple classes, symmetric operations. Use friend class for: tightly coupled classes (iterator/container), privileged access for specific collaborators. Avoid overuse - prefer public interface.

### Interview Essential Questions

**Q1: What is the difference between static and non-static data members? When would you use each?**

A: Static data members are shared by all objects of the class - only one copy exists in data/BSS segment for entire program lifetime. Non-static members are per-object - each instance has its own copy in object memory with object lifetime.

Use static when: (1) Data should be shared across all instances (e.g., object counter, connection limit), (2) Class-level constants or configuration, (3) Need data to persist even if no objects exist, (4) Memory efficiency for shared data.

Use non-static when: (1) Each object needs independent data (e.g., balance in BankAccount), (2) Data specific to object's state, (3) Normal object-oriented design.

Key differences: Static initialized outside class definition, accessed via `ClassName::member` or object. Non-static initialized in constructor, accessed only via object. Static functions can only access static members because they have no `this` pointer.

**Q2: Explain static member functions. What are their limitations and use cases?**

A: Static member functions belong to class, not objects. Can be called without creating instance using `ClassName::function()`. Don't have `this` pointer, so can only access static members (data and functions). Cannot be declared virtual, const, or volatile.

Limitations: (1) Cannot access non-static members directly, (2) No `this` pointer available, (3) Cannot be overridden (not virtual), (4) Cannot modify object state (no object context).

Use cases: (1) Utility functions not requiring object (e.g., Math::sqrt()), (2) Factory methods returning instances, (3) Singleton pattern's getInstance(), (4) Access static data members, (5) Class-level operations.

Example: `class Config { static Config* getInstance(); }` - Singleton pattern uses static function because no instance exists yet when creating first instance.

**Q3: What are friend functions and classes? When should you use them? What are the disadvantages?**

A: Friend functions are non-member functions that can access private/protected members. Friend classes give entire class access to another class's private/protected members. Declared with `friend` keyword inside granting class.

Use when: (1) Operator overloading where left operand isn't class type (e.g., `cout << obj` needs non-member `<<`), (2) Function needs private access to multiple classes, (3) More natural syntax than member function, (4) Tight coupling justified (iterator/container).

Key properties: Friendship is NOT mutual - if A is friend of B, B is NOT automatically friend of A. NOT inherited - derived classes don't inherit friendship. NOT transitive - if A→B and B→C, then A cannot access C's private.

Disadvantages: (1) Breaks encapsulation - violates data hiding, (2) Increases coupling between classes, (3) Harder to maintain - changes affect multiple classes, (4) Reduces OOP principles. Use sparingly, prefer public interfaces when possible.

**Q4: Explain the mutable keyword. When and why would you use it?**

A: `mutable` allows member variable to be modified in const member functions and by const objects. Without mutable, const functions cannot modify ANY members - with mutable, can modify specifically marked members.

Why it exists: Some members don't affect object's logical const-ness but need modification. Examples: cache values (computed lazily), access counters (tracking), mutex locks (thread safety), internal bookkeeping.

Use cases: (1) Caching - store computed values: `mutable double cachedArea; mutable bool computed;` in const getArea(), (2) Counters - track accesses: `mutable int accessCount;` in const getValue(), (3) Thread safety - lock in const: `mutable std::mutex mtx;` in const read(), (4) Lazy initialization.

Example: `class Circle { double radius; mutable double cachedArea; mutable bool computed; public: double getArea() const { if (!computed) { cachedArea = PI*r*r; computed=true; } return cachedArea; } };`

Without mutable, couldn't cache in const function - would need to recompute every call or make function non-const (losing ability to call on const objects).

**Q5: Compare static member function with friend function. When would you choose one over the other?**

A: Both can be called without object, but fundamentally different:

Static member function: (1) Belongs to class (member), (2) Declared with `static` inside class, (3) Can access only static members, (4) Called via `ClassName::function()`, (5) Has class scope, (6) Part of class interface.

Friend function: (1) NOT class member (external), (2) Declared with `friend` inside class, defined outside, (3) Can access all members (static + non-static) via parameter object, (4) Called like normal function, (5) Not part of class scope, (6) Privileged external function.

Choose static when: (1) Function is utility for class, (2) Only needs static data, (3) Natural part of class interface, (4) Factory methods, singleton.

Choose friend when: (1) Operator overloading (especially binary like `+`, `<<`), (2) Function operates on object's private data but conceptually external, (3) Need symmetric syntax (both operands same), (4) Bridging multiple classes.

Example: `static Config* getInstance()` vs `friend ostream& operator<<(ostream&, const Config&)` - getInstance is class operation, `<<` is external operation needing private access.

---