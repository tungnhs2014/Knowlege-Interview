# 5.3. Abstraction & Abstract Classes

---

## Table of Contents

1. Understanding Abstraction
2. Pure Virtual Functions
3. Abstract Classes
4. Interfaces in C++
5. Virtual Destructors
6. Summary

---

## 1. Understanding Abstraction

### 1.1 What is Abstraction?

**Definition**: Hiding complex implementation details and showing only essential features to the user.

**Core Principle**: "Show WHAT to do, hide HOW it's done"

**Why Abstraction Exists:**

- Reduces complexity for users
- Enhances security (hides sensitive implementation)
- Improves maintainability (change implementation without breaking interface)
- Supports modularity (clear boundaries between components)
- Enables polymorphism

**Two Types of Abstraction:**

1. **Data Abstraction** - Hiding data implementation through encapsulation
    - Private members hide internal state
    - Public methods provide controlled access
    - Example: BankAccount hides balance, provides deposit/withdraw methods
2. **Control Abstraction** - Hiding implementation logic through abstract classes
    - Pure virtual functions define interface
    - Derived classes provide implementation
    - Example: Shape defines draw(), Circle/Rectangle implement it differently

**Real-World Analogies:**

```
ATM Machine:
WHAT user sees (Interface):
- Card slot
- PIN pad
- Withdraw/Deposit buttons
- Screen showing balance

HOW it works (Hidden Implementation):
- Network communication protocols
- Encryption algorithms
- Database queries
- Cash dispenser mechanics
- Security validation logic

Car Driving:
WHAT driver uses (Interface):
- Steering wheel
- Pedals (gas, brake)
- Gear shift
- Dashboard indicators

HOW it works (Hidden Implementation):
- Engine combustion process
- Transmission mechanics
- Brake hydraulics
- Fuel injection system
- Electronic control units
```

```cpp
// abstraction_concept.cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Abstraction through encapsulation (Data Abstraction)
class BankAccount {
private:
    // WHY: Hide HOW money is stored (implementation details)
    double balance;
    string accountNumber;
    int transactionCount;

    // WHY: Hide internal validation logic
    bool isValidAmount(double amount) const {
        return amount > 0 && amount <= 1000000;  // Max transaction limit
    }

    bool hasSufficientFunds(double amount) const {
        return amount <= balance;
    }

    void logTransaction(string type, double amount) {
        transactionCount++;
        cout << "[LOG] Transaction #" << transactionCount
             << ": " << type << " $" << amount << endl;
    }

public:
    // WHY: Show WHAT user can do (public interface)
    BankAccount(string accNum, double initialBalance)
        : accountNumber(accNum), balance(initialBalance), transactionCount(0) {
        cout << "Account created: " << accountNumber << endl;
    }

    // WHY: User doesn't need to know HOW withdrawal works internally
    bool withdraw(double amount) {
        if (!isValidAmount(amount)) {
            cout << "Invalid amount!" << endl;
            return false;
        }

        if (!hasSufficientFunds(amount)) {
            cout << "Insufficient funds!" << endl;
            return false;
        }

        balance -= amount;
        logTransaction("WITHDRAW", amount);
        return true;
    }

    bool deposit(double amount) {
        if (!isValidAmount(amount)) {
            cout << "Invalid amount!" << endl;
            return false;
        }

        balance += amount;
        logTransaction("DEPOSIT", amount);
        return true;
    }

    double getBalance() const {
        return balance;
    }

    void displayStatement() const {
        cout << "\n=== Account Statement ===" << endl;
        cout << "Account: " << accountNumber << endl;
        cout << "Balance: $" << balance << endl;
        cout << "Transactions: " << transactionCount << endl;
    }
};

int main() {
    BankAccount account("ACC-12345", 1000.0);

    // WHY: User sees simple, clear interface
    account.deposit(500);
    account.withdraw(200);
    account.withdraw(2000);  // Will fail - insufficient funds

    account.displayStatement();

    // WHY: Cannot access implementation details (security)
    // account.balance = 9999999;  // ERROR! Private member
    // account.logTransaction("HACK", 1000);  // ERROR! Private method

    return 0;
}
```

**Output:**

```
Account created: ACC-12345
[LOG] Transaction #1: DEPOSIT $500
[LOG] Transaction #2: WITHDRAW $200
Insufficient funds!

=== Account Statement ===
Account: ACC-12345
Balance: $1300
Transactions: 2
```

### 1.2 Abstraction Through Header Files

**Concept**: Separate interface (declaration) from implementation (definition).

**Why This Matters:**

- User sees WHAT functions do (header)
- User doesn't see HOW functions work (implementation file)
- Can change implementation without recompiling user code
- Standard library uses this extensively (e.g., `<iostream>`, `<vector>`)

```cpp
// math_operations.h
#ifndef MATH_OPERATIONS_H
#define MATH_OPERATIONS_H

// WHY: User sees WHAT functions do (interface)
double calculatePower(double base, int exponent);
double calculateSquareRoot(double number);
double calculateFactorial(int n);

#endif
```

```cpp
// math_operations.cpp
#include "math_operations.h"
#include <cmath>

// WHY: Implementation hidden from user (HOW it works)
double calculatePower(double base, int exponent) {
    // User doesn't see this complex algorithm
    return pow(base, exponent);
}

double calculateSquareRoot(double number) {
    // User doesn't know Newton-Raphson method is used
    return sqrt(number);
}

double calculateFactorial(int n) {
    // User doesn't see recursive implementation
    if (n <= 1) return 1;
    return n * calculateFactorial(n - 1);
}
```

```cpp
// main.cpp
#include <iostream>
#include "math_operations.h"
using namespace std;

int main() {
    // WHY: User just calls functions, doesn't need to understand algorithms
    cout << "2^10 = " << calculatePower(2, 10) << endl;
    cout << "sqrt(144) = " << calculateSquareRoot(144) << endl;
    cout << "5! = " << calculateFactorial(5) << endl;

    // Implementation completely hidden - user doesn't know:
    // - How pow() algorithm works
    // - How sqrt() uses Newton-Raphson
    // - That factorial uses recursion

    return 0;
}
```

---

## 2. Pure Virtual Functions

### 2.1 Understanding Pure Virtual Functions

**Definition**: Virtual function with no implementation in base class, declared with `= 0`.

**Syntax**: `virtual returnType functionName() = 0;`

**Why Pure Virtual Functions Exist:**

- Base class cannot provide meaningful implementation
- Each derived class needs completely different implementation
- Want to force derived classes to provide implementation
- Define interface/contract that derived classes must follow
- Make class abstract (cannot instantiate)

**When to Use:**

```
Use pure virtual when:
✅ Implementation is unknown/impossible in base class
✅ Each derived class has fundamentally different implementation
✅ Want to enforce implementation in all derived classes
✅ Creating interface or abstract base class

Don't use pure virtual when:
❌ Base class can provide default/common implementation
❌ Only some derived classes need custom implementation
❌ Want concrete base class that can be instantiated
```

```cpp
// pure_virtual_complete.cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// WHY: Shape is abstract concept - cannot know how to draw it
// Each shape draws completely differently
class Shape {
protected:
    string color;
    double x, y;  // Position

public:
    Shape(string c, double xPos, double yPos)
        : color(c), x(xPos), y(yPos) {}

    // WHY: Pure virtual - implementation impossible in Shape
    // How to draw? Circle? Rectangle? Triangle? Pentagon?
    virtual void draw() const = 0;  // = 0 makes it pure virtual

    // WHY: Pure virtual - area calculation depends on shape
    virtual double calculateArea() const = 0;

    // WHY: Regular virtual - has default but can override
    virtual void displayInfo() const {
        cout << "Position: (" << x << ", " << y << ")" << endl;
        cout << "Color: " << color << endl;
    }

    // WHY: Regular function - same for all shapes
    void setColor(string c) {
        color = c;
    }

    // WHY: Virtual destructor for polymorphism
    virtual ~Shape() {
        cout << "Shape destructor" << endl;
    }
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(string c, double xPos, double yPos, double r)
        : Shape(c, xPos, yPos), radius(r) {}

    // WHY: MUST override pure virtual functions
    void draw() const override {
        cout << "Drawing Circle:" << endl;
        cout << "  Center: (" << x << ", " << y << ")" << endl;
        cout << "  Radius: " << radius << endl;
        cout << "  Color: " << color << endl;
    }

    double calculateArea() const override {
        return 3.14159 * radius * radius;
    }

    ~Circle() {
        cout << "Circle destructor" << endl;
    }
};

class Rectangle : public Shape {
private:
    double width, height;

public:
    Rectangle(string c, double xPos, double yPos, double w, double h)
        : Shape(c, xPos, yPos), width(w), height(h) {}

    // WHY: MUST override pure virtual functions
    void draw() const override {
        cout << "Drawing Rectangle:" << endl;
        cout << "  Top-left: (" << x << ", " << y << ")" << endl;
        cout << "  Dimensions: " << width << "x" << height << endl;
        cout << "  Color: " << color << endl;
    }

    double calculateArea() const override {
        return width * height;
    }

    ~Rectangle() {
        cout << "Rectangle destructor" << endl;
    }
};

class Triangle : public Shape {
private:
    double base, height;

public:
    Triangle(string c, double xPos, double yPos, double b, double h)
        : Shape(c, xPos, yPos), base(b), height(h) {}

    void draw() const override {
        cout << "Drawing Triangle:" << endl;
        cout << "  Base point: (" << x << ", " << y << ")" << endl;
        cout << "  Base: " << base << ", Height: " << height << endl;
        cout << "  Color: " << color << endl;
    }

    double calculateArea() const override {
        return 0.5 * base * height;
    }

    ~Triangle() {
        cout << "Triangle destructor" << endl;
    }
};

int main() {
    // WHY: Cannot create object of abstract class
    // Shape s("Red", 0, 0);  // ERROR! Shape is abstract

    // WHY: Can create pointers/references to abstract class
    Shape* shapes[3];

    shapes[0] = new Circle("Red", 10, 20, 5);
    shapes[1] = new Rectangle("Blue", 30, 40, 15, 10);
    shapes[2] = new Triangle("Green", 50, 60, 12, 8);

    cout << "=== Drawing All Shapes ===" << endl;
    for (int i = 0; i < 3; i++) {
        shapes[i]->draw();
        cout << "Area: " << shapes[i]->calculateArea() << endl;
        cout << endl;
    }

    // WHY: Polymorphism - correct destructors called
    cout << "=== Cleanup ===" << endl;
    for (int i = 0; i < 3; i++) {
        delete shapes[i];
    }

    return 0;
}
```

### 2.2 Virtual vs Pure Virtual Functions

**Detailed Comparison:**

| Aspect | Virtual Function | Pure Virtual Function |
| --- | --- | --- |
| **Syntax** | `virtual void func()` | `virtual void func() = 0` |
| **Implementation** | Has body in base class | No body (declared with = 0) |
| **Override** | Optional in derived | Mandatory in derived |
| **Base instantiation** | Class can be instantiated | Class becomes abstract |
| **Purpose** | Provide default + allow override | Force override in derived |
| **Use case** | Common behavior | No common implementation |
| **Derived still abstract** | No (unless other pure virtuals) | Yes (if not overridden) |

```cpp
// virtual_vs_pure_virtual.cpp
#include <iostream>
using namespace std;

class Animal {
public:
    // WHY: Regular virtual - has default, can override
    virtual void eat() {
        cout << "Animal is eating something" << endl;
    }

    // WHY: Regular virtual - common breathing mechanism
    virtual void breathe() {
        cout << "Animal is breathing oxygen" << endl;
    }

    // WHY: Pure virtual - no default sound
    virtual void makeSound() = 0;

    // WHY: Pure virtual - movement varies too much
    virtual void move() = 0;

    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    // WHY: Can override virtual (but not required)
    void eat() override {
        cout << "Dog is eating meat and bones" << endl;
    }

    // WHY: Using inherited breathe() - not overriding

    // WHY: MUST override pure virtual functions
    void makeSound() override {
        cout << "Dog: Woof! Woof!" << endl;
    }

    void move() override {
        cout << "Dog runs on four legs" << endl;
    }
};

class Fish : public Animal {
public:
    // WHY: Using inherited eat() - not overriding

    // WHY: Must override - fish breathe differently
    void breathe() override {
        cout << "Fish is breathing through gills" << endl;
    }

    // WHY: MUST override pure virtual functions
    void makeSound() override {
        cout << "Fish: *bubble sounds*" << endl;
    }

    void move() override {
        cout << "Fish swims using fins" << endl;
    }
};

int main() {
    // Animal a;  // ERROR! Cannot instantiate abstract class

    Animal* animals[2];
    animals[0] = new Dog();
    animals[1] = new Fish();

    for (int i = 0; i < 2; i++) {
        cout << "\n=== Animal " << (i + 1) << " ===" << endl;
        animals[i]->eat();      // May use default or override
        animals[i]->breathe();  // May use default or override
        animals[i]->makeSound(); // Always overridden
        animals[i]->move();     // Always overridden
        delete animals[i];
    }

    return 0;
}
```

---

## 3. Abstract Classes

### 3.1 Understanding Abstract Classes

**Definition**: Class with at least one pure virtual function.

**Key Properties:**

1. **Cannot instantiate** - No objects can be created
2. **Can have pointers/references** - For polymorphism
3. **Can have data members** - Regular variables
4. **Can have regular functions** - With implementations
5. **Can have constructors** - Called by derived classes
6. **Must have virtual destructor** - For proper cleanup
7. **Derived stays abstract** - Until all pure virtuals overridden

**Why Abstract Classes?**

- Define common interface for family of classes
- Force implementation of specific functions
- Provide partial implementation (mix concrete and pure virtual)
- Enable polymorphism with guaranteed interface
- Design contracts

```cpp
// abstract_class_complete.cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// WHY: Database is abstract concept - defines interface
class Database {
protected:
    string connectionString;
    bool connected;
    int queryCount;

public:
    // WHY: Constructor - can have in abstract class
    Database(string conn)
        : connectionString(conn), connected(false), queryCount(0) {
        cout << "Database base constructor" << endl;
    }

    // WHY: Pure virtual - each DB connects differently
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual void executeQuery(string query) = 0;

    // WHY: Regular function - same for all databases
    void displayStatus() const {
        cout << "Status: " << (connected ? "Connected" : "Disconnected") << endl;
        cout << "Queries executed: " << queryCount << endl;
    }

    bool isConnected() const {
        return connected;
    }

    // WHY: Virtual destructor - polymorphism cleanup
    virtual ~Database() {
        cout << "Database base destructor" << endl;
    }
};

class MySQLDatabase : public Database {
private:
    int port;

public:
    MySQLDatabase(string conn, int p = 3306)
        : Database(conn), port(p) {
        cout << "MySQL constructor" << endl;
    }

    // WHY: Must implement all pure virtual functions
    void connect() override {
        connected = true;
        cout << "Connected to MySQL at " << connectionString
             << ":" << port << endl;
    }

    void disconnect() override {
        if (connected) {
            connected = false;
            cout << "Disconnected from MySQL" << endl;
        }
    }

    void executeQuery(string query) override {
        if (connected) {
            queryCount++;
            cout << "[MySQL] Executing: " << query << endl;
        } else {
            cout << "[MySQL] Error: Not connected!" << endl;
        }
    }

    ~MySQLDatabase() {
        cout << "MySQL destructor" << endl;
    }
};

class PostgreSQLDatabase : public Database {
private:
    string schema;

public:
    PostgreSQLDatabase(string conn, string sch = "public")
        : Database(conn), schema(sch) {
        cout << "PostgreSQL constructor" << endl;
    }

    void connect() override {
        connected = true;
        cout << "Connected to PostgreSQL: " << connectionString
             << ", Schema: " << schema << endl;
    }

    void disconnect() override {
        if (connected) {
            connected = false;
            cout << "Disconnected from PostgreSQL" << endl;
        }
    }

    void executeQuery(string query) override {
        if (connected) {
            queryCount++;
            cout << "[PostgreSQL] Executing: " << query << endl;
        } else {
            cout << "[PostgreSQL] Error: Not connected!" << endl;
        }
    }

    ~PostgreSQLDatabase() {
        cout << "PostgreSQL destructor" << endl;
    }
};

void runDatabaseOperations(Database* db) {
    db->connect();
    db->executeQuery("SELECT * FROM users");
    db->executeQuery("INSERT INTO logs VALUES ('test')");
    db->displayStatus();
    db->disconnect();
}

int main() {
    // Database db("test");  // ERROR! Cannot instantiate abstract class

    cout << "=== MySQL Operations ===" << endl;
    Database* db1 = new MySQLDatabase("localhost:3306");
    runDatabaseOperations(db1);
    delete db1;

    cout << "\n=== PostgreSQL Operations ===" << endl;
    Database* db2 = new PostgreSQLDatabase("localhost:5432", "myschema");
    runDatabaseOperations(db2);
    delete db2;

    return 0;
}
```

### 3.2 Derived Class Still Abstract

**Important Rule**: If derived class doesn't override ALL pure virtual functions, it remains abstract.

```cpp
// derived_still_abstract.cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void func1() = 0;
    virtual void func2() = 0;
    virtual void func3() = 0;
    virtual ~Base() {}
};

// WHY: Partially derived - doesn't override all pure virtuals
class PartiallyDerived : public Base {
public:
    void func1() override {
        cout << "func1 implemented" << endl;
    }

    void func2() override {
        cout << "func2 implemented" << endl;
    }

    // WHY: func3 NOT overridden - class still abstract!
};

// WHY: Fully derived - overrides all pure virtuals
class FullyDerived : public Base {
public:
    void func1() override {
        cout << "func1 implemented" << endl;
    }

    void func2() override {
        cout << "func2 implemented" << endl;
    }

    void func3() override {
        cout << "func3 implemented" << endl;
    }
};

int main() {
    // Base b;  // ERROR! Base is abstract
    // PartiallyDerived pd;  // ERROR! Still abstract (func3 not overridden)

    FullyDerived fd;  // OK! All pure virtuals overridden
    fd.func1();
    fd.func2();
    fd.func3();

    // WHY: Can still use PartiallyDerived as pointer type
    // Base* ptr = new PartiallyDerived();  // ERROR! Cannot instantiate

    return 0;
}
```

---

## 4. Interfaces in C++

### 4.1 What is an Interface?

**Definition**: Abstract class with ONLY pure virtual functions (and possibly virtual destructor).

**C++ Note**: C++ doesn't have `interface` keyword like Java/C#. We simulate interfaces using abstract classes.

**Interface Characteristics:**

- All member functions are pure virtual
- No data members (convention, not enforced)
- No function implementations
- Defines contract only (WHAT, not HOW)
- Supports multiple inheritance
- Virtual destructor recommended

**Why Interfaces?**

- Define contracts/protocols
- Enable multiple inheritance cleanly
- Decouple interface from implementation
- Support plugin architectures
- Follow Interface Segregation Principle (ISP)

```cpp
// interfaces_complete.cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// WHY: Pure interface - defines WHAT, not HOW
class Printable {
public:
    virtual void print() const = 0;
    virtual ~Printable() {}
};

class Serializable {
public:
    virtual string serialize() const = 0;
    virtual void deserialize(const string& data) = 0;
    virtual ~Serializable() {}
};

class Comparable {
public:
    virtual bool isEqual(const Comparable& other) const = 0;
    virtual bool isLessThan(const Comparable& other) const = 0;
    virtual ~Comparable() {}
};

// WHY: Document implements multiple interfaces
class Document : public Printable, public Serializable, public Comparable {
private:
    string title;
    string content;
    int wordCount;

    void updateWordCount() {
        wordCount = 1;
        for (char c : content) {
            if (c == ' ') wordCount++;
        }
    }

public:
    Document(string t, string c) : title(t), content(c) {
        updateWordCount();
    }

    // WHY: Implement Printable interface
    void print() const override {
        cout << "╔════════════════════════════════╗" << endl;
        cout << "║ DOCUMENT                       ║" << endl;
        cout << "╠════════════════════════════════╣" << endl;
        cout << "║ Title: " << title << endl;
        cout << "║ Words: " << wordCount << endl;
        cout << "╠════════════════════════════════╣" << endl;
        cout << "║ " << content << endl;
        cout << "╚════════════════════════════════╝" << endl;
    }

    // WHY: Implement Serializable interface
    string serialize() const override {
        return "DOC|" + title + "|" + content + "|" + to_string(wordCount);
    }

    void deserialize(const string& data) override {
        // Simple parsing: DOC|title|content|wordCount
        size_t pos1 = data.find('|');
        size_t pos2 = data.find('|', pos1 + 1);
        size_t pos3 = data.find('|', pos2 + 1);

        title = data.substr(pos1 + 1, pos2 - pos1 - 1);
        content = data.substr(pos2 + 1, pos3 - pos2 - 1);
        wordCount = stoi(data.substr(pos3 + 1));
    }

    // WHY: Implement Comparable interface
    bool isEqual(const Comparable& other) const override {
        const Document* doc = dynamic_cast<const Document*>(&other);
        if (!doc) return false;
        return title == doc->title && content == doc->content;
    }

    bool isLessThan(const Comparable& other) const override {
        const Document* doc = dynamic_cast<const Document*>(&other);
        if (!doc) return false;
        return wordCount < doc->wordCount;
    }

    string getTitle() const { return title; }
};

// WHY: Function accepting interface - works with ANY Printable object
void printAnyObject(const Printable& obj) {
    obj.print();
}

int main() {
    Document doc1("Report", "This is quarterly financial report");
    Document doc2("Memo", "Brief meeting notes");
    Document doc3("Report", "This is quarterly financial report");

    cout << "=== Printable Interface ===" << endl;
    printAnyObject(doc1);

    cout << "\n=== Serializable Interface ===" << endl;
    string serialized = doc1.serialize();
    cout << "Serialized: " << serialized << endl;

    Document doc4("", "");
    doc4.deserialize(serialized);
    cout << "Deserialized title: " << doc4.getTitle() << endl;

    cout << "\n=== Comparable Interface ===" << endl;
    cout << "doc1 == doc3? " << (doc1.isEqual(doc3) ? "Yes" : "No") << endl;
    cout << "doc2 < doc1? " << (doc2.isLessThan(doc1) ? "Yes" : "No") << endl;

    return 0;
}
```

### 4.2 Interface Segregation Principle (ISP)

**Principle**: Many small specific interfaces better than one large general-purpose interface.

**Why**: Don't force classes to implement methods they don't use.

```cpp
// interface_segregation.cpp
#include <iostream>
#include <string>
using namespace std;

// ❌ BAD: Single large interface
class BadWorkerInterface {
public:
    virtual void work() = 0;
    virtual void eat() = 0;
    virtual void sleep() = 0;
    virtual void getMaintenance() = 0;  // Only robots need this!
    virtual void charge() = 0;           // Only robots need this!
    virtual ~BadWorkerInterface() {}
};

// ✅ GOOD: Segregated interfaces
class Workable {
public:
    virtual void work() = 0;
    virtual ~Workable() {}
};

class Eatable {
public:
    virtual void eat() = 0;
    virtual ~Eatable() {}
};

class Sleepable {
public:
    virtual void sleep() = 0;
    virtual ~Sleepable() {}
};

class Maintainable {
public:
    virtual void getMaintenance() = 0;
    virtual ~Maintainable() {}
};

class Chargeable {
public:
    virtual void charge() = 0;
    virtual ~Chargeable() {}
};

// WHY: Human only implements relevant interfaces
class Human : public Workable, public Eatable, public Sleepable {
private:
    string name;
    int energy;

public:
    Human(string n) : name(n), energy(100) {}

    void work() override {
        energy -= 20;
        cout << name << " is working (Energy: " << energy << "%)" << endl;
    }

    void eat() override {
        energy = min(100, energy + 30);
        cout << name << " is eating (Energy: " << energy << "%)" << endl;
    }

    void sleep() override {
        energy = 100;
        cout << name << " is sleeping (Energy restored to 100%)" << endl;
    }

    // WHY: Doesn't have getMaintenance() or charge() - not needed!
};

// WHY: Robot only implements relevant interfaces
class Robot : public Workable, public Maintainable, public Chargeable {
private:
    string model;
    int battery;

public:
    Robot(string m) : model(m), battery(100) {}

    void work() override {
        battery -= 15;
        cout << model << " robot is working (Battery: " << battery << "%)" << endl;
    }

    void getMaintenance() override {
        cout << model << " robot getting maintenance check" << endl;
    }

    void charge() override {
        battery = 100;
        cout << model << " robot charging (Battery: 100%)" << endl;
    }

    // WHY: Doesn't have eat() or sleep() - robots don't eat/sleep!
};

void makeWork(Workable& worker) {
    worker.work();
}

int main() {
    Human human("Alice");
    Robot robot("R2D2");

    cout << "=== Human Activities ===" << endl;
    human.work();
    human.eat();
    human.work();
    human.sleep();

    cout << "\n=== Robot Activities ===" << endl;
    robot.work();
    robot.getMaintenance();
    robot.work();
    robot.charge();

    cout << "\n=== Polymorphism via Workable ===" << endl;
    makeWork(human);
    makeWork(robot);

    return 0;
}
```

---

## 5. Virtual Destructors

### 5.1 The Critical Problem

**Problem**: Without virtual destructor, only base destructor called when deleting derived object through base pointer → Memory leak!

**Solution**: ALWAYS make destructor virtual in base class if it has virtual functions.

```cpp
// virtual_destructor_problem.cpp
#include <iostream>
using namespace std;

class Base {
public:
    Base() {
        cout << "Base constructor" << endl;
    }

    // WHY: Non-virtual destructor (DANGEROUS!)
    ~Base() {
        cout << "Base destructor" << endl;
    }

    virtual void someVirtualFunc() {}
};

class Derived : public Base {
private:
    int* largeData;

public:
    Derived() {
        largeData = new int[10000];  // Allocate 40KB
        cout << "Derived constructor - allocated 40KB" << endl;
    }

    ~Derived() {
        delete[] largeData;
        cout << "Derived destructor - freed 40KB" << endl;
    }
};

int main() {
    cout << "=== Problem Demonstration ===" << endl;

    Base* ptr = new Derived();

    // WHY: Only Base destructor called!
    // Derived destructor NOT called → 40KB memory leak!
    delete ptr;

    cout << "\n⚠️  MEMORY LEAK: 40KB not freed!" << endl;
    cout << "Derived destructor was never called!" << endl;

    return 0;
}

```

**Output:**

```
=== Problem Demonstration ===
Base constructor
Derived constructor - allocated 40KB
Base destructor

⚠️  MEMORY LEAK: 40KB not freed!
Derived destructor was never called!
```

### 5.2 The Solution: Virtual Destructor

```cpp
// virtual_destructor_solution.cpp
#include <iostream>
using namespace std;

class Base {
public:
    Base() {
        cout << "Base constructor" << endl;
    }

    // WHY: Virtual destructor - ensures proper cleanup
    virtual ~Base() {
        cout << "Base destructor" << endl;
    }

    virtual void someVirtualFunc() {}
};

class Derived : public Base {
private:
    int* largeData;

public:
    Derived() {
        largeData = new int[10000];
        cout << "Derived constructor - allocated 40KB" << endl;
    }

    ~Derived() {
        delete[] largeData;
        cout << "Derived destructor - freed 40KB" << endl;
    }
};

int main() {
    cout << "=== Solution with Virtual Destructor ===" << endl;

    Base* ptr = new Derived();

    // WHY: Both destructors called in correct order!
    // Derived → Base (reverse of construction)
    delete ptr;

    cout << "\n✅ No memory leak! Both destructors called!" << endl;

    return 0;
}
```

**Output:**

```
=== Solution with Virtual Destructor ===
Base constructor
Derived constructor - allocated 40KB
Derived destructor - freed 40KB
Base destructor

✅ No memory leak! Both destructors called!
```

### 5.3 Pure Virtual Destructor

**Special Case**: Can have pure virtual destructor to make class abstract, BUT must provide body!

**Why Body Required**: Destructors called in chain (derived → base), so base destructor body is needed even if pure virtual.

```cpp
// pure_virtual_destructor.cpp
#include <iostream>
using namespace std;

class AbstractBase {
public:
    AbstractBase() {
        cout << "AbstractBase constructor" << endl;
    }

    // WHY: Pure virtual destructor makes class abstract
    virtual ~AbstractBase() = 0;

    virtual void operation() = 0;
};

// WHY: MUST provide body even though pure virtual!
AbstractBase::~AbstractBase() {
    cout << "AbstractBase destructor (pure virtual)" << endl;
}

class ConcreteDerived : public AbstractBase {
public:
    ConcreteDerived() {
        cout << "ConcreteDerived constructor" << endl;
    }

    ~ConcreteDerived() {
        cout << "ConcreteDerived destructor" << endl;
    }

    void operation() override {
        cout << "ConcreteDerived::operation()" << endl;
    }
};

int main() {
    // AbstractBase ab;  // ERROR! Class is abstract

    // WHY: Can create derived objects
    AbstractBase* ptr = new ConcreteDerived();
    ptr->operation();

    // WHY: Both destructors called (pure virtual body is called)
    delete ptr;

    return 0;
}
```

**Output:**

```
AbstractBase constructor
ConcreteDerived constructor
ConcreteDerived::operation()
ConcreteDerived destructor
AbstractBase destructor (pure virtual)
```

### 5.4 Virtual Destructor Best Practices

**Golden Rules:**

```cpp
// Rule 1: If class has ANY virtual function → destructor MUST be virtual
class GoodBase {
public:
    virtual void func() {}
    virtual ~GoodBase() {}  // ✅ CORRECT
};

// Rule 2: Dangerous - virtual functions but non-virtual destructor
class DangerousBase {
public:
    virtual void func() {}
    ~DangerousBase() {}  // ❌ DANGEROUS! Memory leaks possible
};

// Rule 3: OK - no virtual functions, no need for virtual destructor
class SimpleClass {
public:
    void func() {}  // Not virtual
    ~SimpleClass() {}  // ✅ OK - no polymorphism
};

// Rule 4: Abstract class with pure virtual destructor
class AbstractInterface {
public:
    virtual void operation() = 0;
    virtual ~AbstractInterface() = 0;  // ✅ Makes class abstract
};
AbstractInterface::~AbstractInterface() {}  // Must define
```

---

## Summary

### Key Takeaways

1. **Abstraction Concept** - Hiding implementation, showing only essentials. Data abstraction (encapsulation with private members) and control abstraction (abstract classes with pure virtual functions). Reduces complexity, enhances security, improves maintainability. Examples: ATM (hide network protocols), Car (hide engine mechanics).
2. **Pure Virtual Functions** - Declared with `= 0`, has no implementation in base class. Forces derived classes to override. Use when: base cannot provide meaningful implementation, each derived needs fundamentally different implementation, want to enforce interface contract. Makes class abstract (cannot instantiate).
3. **Virtual vs Pure Virtual** - Virtual: has implementation, optional override, class is concrete. Pure virtual: no implementation (=0), mandatory override, class becomes abstract. Virtual provides default with flexibility, pure virtual enforces implementation in derived classes.
4. **Abstract Classes** - Class with ≥1 pure virtual function. Cannot instantiate directly, can have pointers/references for polymorphism. Can contain: pure virtual functions (must override), regular functions (inherited), data members, constructors/destructors. Used for: defining interfaces, partial implementations, polymorphism.
5. **Derived Still Abstract** - If derived class doesn't override ALL pure virtual functions, it remains abstract. Cannot instantiate until all pure virtuals implemented. Allows multi-level abstraction. Compiler enforces complete implementation before allowing instantiation.
6. **Interfaces in C++** - Simulated with abstract class containing ONLY pure virtual functions. No `interface` keyword like Java. Characteristics: all pure virtual, no data members (convention), defines contract only. Multiple inheritance encouraged for interfaces. Benefits: clear contracts, decoupling, plugin architectures.
7. **Interface Segregation** - Many small specific interfaces better than one large general interface. Don't force classes to implement unused methods. Example: separate Workable, Eatable, Sleepable; Human implements all, Robot only Workable+Maintainable+Chargeable (robots don't eat/sleep).
8. **Virtual Destructors** - CRITICAL for polymorphism. Rule: if class has ANY virtual function, destructor MUST be virtual. Without virtual: only base destructor called when deleting derived via base pointer → memory/resource leak. With virtual: both destructors called in correct order (derived→base).
9. **Pure Virtual Destructor** - Can declare `virtual ~Base() = 0;` to make class abstract. MUST provide body outside class: `Base::~Base() {}`. Why body needed: destructors called in chain during destruction, base destructor executed even if pure virtual. Use to make class abstract when no other pure virtual functions needed.
10. **Abstraction Benefits** - Reduces complexity (simple interface hides complex implementation), enhances security (implementation hidden from users), improves maintainability (change implementation without breaking interface), supports modularity (clear boundaries), enables polymorphism (work with base pointers), enforces contracts (derived must implement). Foundation of large-scale software design.

### Interview Essential Questions

**Q1: What is abstraction? Explain with real-world example and C++ implementation.**

A: Abstraction hides complex implementation details, shows only essential features. Core principle: "Show WHAT to do, hide HOW it's done."

Real-world: ATM machine - User sees: card slot, PIN pad, buttons (WHAT they can do). Hidden: network protocols, encryption, database queries, cash mechanics (HOW it works). Driver sees steering wheel and pedals, not engine combustion or transmission mechanics.

C++ implementation: Two ways - (1) Data abstraction via encapsulation: BankAccount with private balance, public deposit/withdraw. Users cannot directly access balance, must use provided methods. (2) Control abstraction via abstract classes: Shape class with pure virtual draw(), Circle/Rectangle provide implementations. Users see Shape interface, implementations hidden in derived classes.

Benefits: reduces complexity for users, enhances security, improves maintainability, supports modularity, enables polymorphism.

**Q2: What is pure virtual function? When and why use it? How differs from regular virtual?**

A: Pure virtual function has no implementation in base class, declared with `= 0`: `virtual void func() = 0;`. Makes class abstract (cannot instantiate).

When to use: (1) Base class cannot provide meaningful implementation - Shape::draw() because circles and rectangles draw completely differently, (2) Want to force all derived classes to implement function, (3) Defining interface/contract, (4) Each derived class needs fundamentally different implementation.

Why exists: Compiler enforces contract - derived class won't compile unless it overrides all pure virtual functions. Guarantees interface implementation, enables polymorphism with guaranteed behavior.

Difference from regular virtual: Regular virtual has body in base class, override is optional, class stays concrete. Pure virtual has no body (=0), override is mandatory, class becomes abstract. Regular provides default behavior, pure virtual enforces derived implementation. Example: virtual eat() {default impl} - optional override. Pure virtual makeSound()=0 - mandatory override.

**Q3: What is abstract class? Properties, when to use, difference from interface?**

A: Abstract class has ≥1 pure virtual function, cannot instantiate directly.

Properties: (1) Cannot create objects directly, (2) Can have pointers/references for polymorphism, (3) Can have data members (regular variables), (4) Can have regular member functions with implementations, (5) Can mix pure virtual (must override) with regular virtual (optional override), (6) Constructors/destructors allowed, (7) Derived stays abstract until all pure virtuals overridden.

When to use: (1) Define common interface for related classes, (2) Provide partial implementation (some concrete methods, some pure virtual), (3) Force specific functions to be implemented, (4) Enable polymorphism with guaranteed interface. Example: Database abstract class has common connectionString and pure virtual connect/disconnect - MySQL and PostgreSQL provide implementations.

Difference from interface: Interface is abstract class with ONLY pure virtual functions, no data members, no implementations. Abstract class can mix pure virtual with regular functions/data. Interface defines pure contract (WHAT), abstract class can provide partial implementation (some HOW). C++ doesn't have interface keyword, simulated with pure abstract class.

**Q4: Explain virtual destructors. Why critical? What happens without? Pure virtual destructor?**

A: Virtual destructor ensures proper cleanup in polymorphic hierarchies. Critical rule: if class has ANY virtual function, destructor MUST be virtual.

Problem without virtual: When deleting derived object through base pointer, only base destructor called, derived destructor skipped → memory leak, resource leak (files, locks, sockets, etc.). Example: Base* ptr = new Derived(); delete ptr; - if Base destructor not virtual, Derived destructor NOT called. If Derived allocated memory (new[]), that memory leaks forever.

With virtual: Both destructors called in correct order (derived first, then base), guaranteeing complete cleanup. Polymorphism works for destructors too - correct destructor chain selected at runtime via vtable.

Pure virtual destructor: `virtual ~Base() = 0;` makes class abstract but MUST provide body: `Base::~Base() {}`. Why body required: destructors called in chain during object destruction (derived→base), base destructor body executed even if declared pure virtual. Use case: make class abstract when no other pure virtual functions needed - destructor itself makes class abstract.

**Q5: What is Interface Segregation Principle? Why important? Example in C++?**

A: ISP states: many small specific interfaces better than one large general-purpose interface. Don't force classes to implement methods they don't use.

Why important: (1) Reduces coupling - classes depend only on methods they actually use, (2) Improves flexibility - can implement only relevant interfaces, (3) Better maintainability - changes to unused methods don't affect class, (4) Clearer contracts - each interface has focused purpose.

C++ example: Instead of single IWorker interface with work(), eat(), sleep(), getMaintenance(), charge() - segregate into: Workable (work), Eatable (eat), Sleepable (sleep), Maintainable (getMaintenance), Chargeable (charge). Human implements Workable+Eatable+Sleepable (humans don't need maintenance or charging). Robot implements Workable+Maintainable+Chargeable (robots don't eat or sleep). Clean implementation, no dummy methods.

Benefit: Human class doesn't have useless getMaintenance() or charge() methods. Robot doesn't have useless eat() or sleep(). Each class implements exactly what it needs, nothing more. Follows "clients should not be forced to depend on interfaces they don't use."

---