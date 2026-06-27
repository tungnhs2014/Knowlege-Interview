# 5.4. Inheritance & Polymorphism

---

## Table of Contents

1. Inheritance Types
2. Inheritance Modes and Access Control
3. Diamond Problem and Virtual Inheritance
4. Polymorphism and Virtual Functions
5. vtable and vptr Mechanism
6. override and final Keywords
7. Object Slicing Problem
8. Summary

---

## 1. Inheritance Types

### 1.1 Understanding Inheritance

**Definition**: Mechanism where a class (derived/child) acquires properties and methods from another class (base/parent).

**Why Inheritance?**

- Code reusability (DRY principle)
- Establishes "is-a" relationship
- Hierarchical classification
- Polymorphism enabler
- Reduces code duplication

**Key Terms:**

- **Base Class** (Parent/Super class) - Class being inherited from
- **Derived Class** (Child/Sub class) - Class that inherits
- **Inheritance Hierarchy** - Tree of inheritance relationships

### 1.2 Single Inheritance

**Definition**: Derived class inherits from ONE base class.

**Syntax**: `class Derived : access_specifier Base { };`

```cpp
// single_inheritance.cpp
#include <iostream>
using namespace std;

// WHY: Animal is base class with common properties
class Animal {
protected:
    string name;
    int age;

public:
    Animal(string n, int a) : name(n), age(a) {
        cout << "Animal constructor" << endl;
    }

    void eat() {
        cout << name << " is eating" << endl;
    }

    void sleep() {
        cout << name << " is sleeping" << endl;
    }

    ~Animal() {
        cout << "Animal destructor" << endl;
    }
};

// WHY: Dog "is-a" Animal (inherits Animal properties)
class Dog : public Animal {
private:
    string breed;

public:
    Dog(string n, int a, string b) : Animal(n, a), breed(b) {
        cout << "Dog constructor" << endl;
    }

    void bark() {
        cout << name << " barks: Woof! Woof!" << endl;
    }

    void displayInfo() {
        cout << "Name: " << name << ", Age: " << age
             << ", Breed: " << breed << endl;
    }

    ~Dog() {
        cout << "Dog destructor" << endl;
    }
};

int main() {
    Dog myDog("Buddy", 3, "Golden Retriever");

    // WHY: Can access inherited methods
    myDog.eat();
    myDog.sleep();

    // WHY: Can access own methods
    myDog.bark();
    myDog.displayInfo();

    return 0;
}
```

**Output:**

```
Animal constructor
Dog constructor
Buddy is eating
Buddy is sleeping
Buddy barks: Woof! Woof!
Name: Buddy, Age: 3, Breed: Golden Retriever
Dog destructor
Animal destructor
```

### 1.3 Multiple Inheritance

**Definition**: Derived class inherits from TWO or more base classes.

**Syntax**: `class Derived : access Base1, access Base2 { };`

**When to Use**: Object genuinely has properties from multiple unrelated classes.

**Caution**: Can lead to diamond problem (see section 3).

```cpp
// multiple_inheritance.cpp
#include <iostream>
#include <string>
using namespace std;

class Person {
protected:
    string name;
    int age;

public:
    Person(string n, int a) : name(n), age(a) {
        cout << "Person constructor" << endl;
    }

    void displayPerson() {
        cout << "Name: " << name << ", Age: " << age << endl;
    }
};

class Employee {
protected:
    int empId;
    double salary;

public:
    Employee(int id, double sal) : empId(id), salary(sal) {
        cout << "Employee constructor" << endl;
    }

    void displayEmployee() {
        cout << "ID: " << empId << ", Salary: $" << salary << endl;
    }
};

// WHY: Manager "is-a" Person AND "is-an" Employee
class Manager : public Person, public Employee {
private:
    string department;

public:
    Manager(string n, int a, int id, double sal, string dept)
        : Person(n, a), Employee(id, sal), department(dept) {
        cout << "Manager constructor" << endl;
    }

    void displayManager() {
        displayPerson();
        displayEmployee();
        cout << "Department: " << department << endl;
    }
};

int main() {
    Manager mgr("Alice", 35, 1001, 95000, "Engineering");
    cout << "\n=== Manager Info ===" << endl;
    mgr.displayManager();

    return 0;
}
```

**Constructor Order**: Person → Employee → Manager (left to right in inheritance list)

### 1.4 Multilevel Inheritance

**Definition**: Class derived from another derived class (chain of inheritance).

**Syntax**: A → B → C (C inherits B, B inherits A)

```cpp
// multilevel_inheritance.cpp
#include <iostream>
using namespace std;

// WHY: Level 1 - Base class
class Vehicle {
protected:
    string brand;
    int year;

public:
    Vehicle(string b, int y) : brand(b), year(y) {
        cout << "Vehicle constructor" << endl;
    }

    void startEngine() {
        cout << brand << " engine started" << endl;
    }
};

// WHY: Level 2 - Derived from Vehicle
class Car : public Vehicle {
protected:
    int numDoors;

public:
    Car(string b, int y, int doors) : Vehicle(b, y), numDoors(doors) {
        cout << "Car constructor" << endl;
    }

    void drive() {
        cout << "Driving " << brand << " car with " << numDoors << " doors" << endl;
    }
};

// WHY: Level 3 - Derived from Car
class ElectricCar : public Car {
private:
    int batteryCapacity;  // kWh

public:
    ElectricCar(string b, int y, int doors, int battery)
        : Car(b, y, doors), batteryCapacity(battery) {
        cout << "ElectricCar constructor" << endl;
    }

    void charge() {
        cout << "Charging " << brand << " (" << batteryCapacity << " kWh battery)" << endl;
    }

    void displayInfo() {
        cout << "\n=== Electric Car Info ===" << endl;
        cout << "Brand: " << brand << endl;
        cout << "Year: " << year << endl;
        cout << "Doors: " << numDoors << endl;
        cout << "Battery: " << batteryCapacity << " kWh" << endl;
    }
};

int main() {
    ElectricCar tesla("Tesla Model S", 2024, 4, 100);

    // WHY: Can access methods from all levels
    tesla.startEngine();  // From Vehicle
    tesla.drive();        // From Car
    tesla.charge();       // From ElectricCar
    tesla.displayInfo();

    return 0;
}
```

**Constructor Chain**: Vehicle → Car → ElectricCar (top to bottom)

**Destructor Chain**: ElectricCar → Car → Vehicle (bottom to top)

### 1.5 Hierarchical Inheritance

**Definition**: Multiple derived classes inherit from ONE base class.

**Syntax**: One parent, many children

```cpp
// hierarchical_inheritance.cpp
#include <iostream>
using namespace std;

// WHY: Common base for all shapes
class Shape {
protected:
    string color;

public:
    Shape(string c) : color(c) {}

    void displayColor() {
        cout << "Color: " << color << endl;
    }
};

// WHY: Circle "is-a" Shape
class Circle : public Shape {
private:
    double radius;

public:
    Circle(string c, double r) : Shape(c), radius(r) {}

    void draw() {
        cout << "Drawing " << color << " circle" << endl;
    }

    double area() {
        return 3.14159 * radius * radius;
    }
};

// WHY: Rectangle "is-a" Shape
class Rectangle : public Shape {
private:
    double width, height;

public:
    Rectangle(string c, double w, double h)
        : Shape(c), width(w), height(h) {}

    void draw() {
        cout << "Drawing " << color << " rectangle" << endl;
    }

    double area() {
        return width * height;
    }
};

// WHY: Triangle "is-a" Shape
class Triangle : public Shape {
private:
    double base, height;

public:
    Triangle(string c, double b, double h)
        : Shape(c), base(b), height(h) {}

    void draw() {
        cout << "Drawing " << color << " triangle" << endl;
    }

    double area() {
        return 0.5 * base * height;
    }
};

int main() {
    Circle c("Red", 5.0);
    Rectangle r("Blue", 10.0, 6.0);
    Triangle t("Green", 8.0, 4.0);

    c.draw();
    cout << "Area: " << c.area() << endl;

    r.draw();
    cout << "Area: " << r.area() << endl;

    t.draw();
    cout << "Area: " << t.area() << endl;

    return 0;
}
```

### 1.6 Hybrid Inheritance

**Definition**: Combination of two or more types of inheritance.

**Example**: Multilevel + Multiple inheritance

```cpp
// hybrid_inheritance.cpp
#include <iostream>
using namespace std;

// Level 1: Base class
class Vehicle {
protected:
    string type;
public:
    Vehicle(string t) : type(t) {
        cout << "Vehicle constructor" << endl;
    }
};

// Level 2: Derived from Vehicle (Multilevel)
class FourWheeler : public Vehicle {
public:
    FourWheeler() : Vehicle("4-Wheeler") {
        cout << "FourWheeler constructor" << endl;
    }
};

// Separate hierarchy
class Engine {
protected:
    int horsepower;
public:
    Engine(int hp) : horsepower(hp) {
        cout << "Engine constructor" << endl;
    }
};

// Hybrid: Multiple inheritance (FourWheeler + Engine) + Multilevel
class Car : public FourWheeler, public Engine {
private:
    string brand;

public:
    Car(string b, int hp) : FourWheeler(), Engine(hp), brand(b) {
        cout << "Car constructor" << endl;
    }

    void display() {
        cout << "Brand: " << brand << endl;
        cout << "Type: " << type << endl;
        cout << "Power: " << horsepower << " HP" << endl;
    }
};

int main() {
    Car myCar("Toyota", 200);
    cout << "\n=== Car Info ===" << endl;
    myCar.display();

    return 0;
}
```

---

## 2. Inheritance Modes and Access Control

### 2.1 Three Inheritance Modes

**Modes:**

1. **public** - Most common, maintains access levels
2. **protected** - Makes public members protected
3. **private** - Makes all inherited members private

**Syntax:**

```cpp
class Derived : public Base { };     // Public inheritance
class Derived : protected Base { };  // Protected inheritance
class Derived : private Base { };    // Private inheritance
```

### 2.2 Access Control Table

**Critical Reference:**

| Base Class Member | public Inheritance | protected Inheritance | private Inheritance |
| --- | --- | --- | --- |
| **public** | public | protected | private |
| **protected** | protected | protected | private |
| **private** | NOT accessible | NOT accessible | NOT accessible |

**Key Rule**: Private members are NEVER accessible in derived class, regardless of inheritance mode.

```cpp
// inheritance_modes.cpp
#include <iostream>
using namespace std;

class Base {
public:
    int publicVar;
protected:
    int protectedVar;
private:
    int privateVar;

public:
    Base() : publicVar(1), protectedVar(2), privateVar(3) {}
};

// WHY: Public inheritance - maintains access levels
class PublicDerived : public Base {
public:
    void access() {
        publicVar = 10;      // OK - still public
        protectedVar = 20;   // OK - still protected
        // privateVar = 30;  // ERROR! Private not accessible
    }
};

// WHY: Protected inheritance - public becomes protected
class ProtectedDerived : protected Base {
public:
    void access() {
        publicVar = 10;      // OK - now protected in ProtectedDerived
        protectedVar = 20;   // OK - still protected
        // privateVar = 30;  // ERROR! Private not accessible
    }
};

// WHY: Private inheritance - all become private
class PrivateDerived : private Base {
public:
    void access() {
        publicVar = 10;      // OK - now private in PrivateDerived
        protectedVar = 20;   // OK - now private in PrivateDerived
        // privateVar = 30;  // ERROR! Private not accessible
    }
};

int main() {
    PublicDerived pd;
    pd.publicVar = 100;  // OK - public member

    ProtectedDerived prd;
    // prd.publicVar = 100;  // ERROR! Now protected, can't access from main

    PrivateDerived pvd;
    // pvd.publicVar = 100;  // ERROR! Now private, can't access from main

    return 0;
}
```

### 2.3 When to Use Each Mode

**public inheritance** (99% of cases):

- "is-a" relationship
- Derived class IS a type of Base class
- Example: Dog is-a Animal, Car is-a Vehicle
- **Most common in practice**

**protected inheritance** (rare):

- "implemented-in-terms-of" for protected interface
- Very uncommon in practice

**private inheritance** (rare):

- "implemented-in-terms-of" relationship
- Composition alternative (prefer composition)
- Example: Stack implemented using List

**Best Practice**: Use public inheritance by default unless you have a specific reason not to.

---

## 3. Diamond Problem and Virtual Inheritance

### 3.1 The Diamond Problem

**Problem**: When class inherits from two classes that share common base class, causing ambiguity.

**Diagram:**

```
        A
       / \
      B   C
       \ /
        D
```

D gets TWO copies of A's members (one through B, one through C) → Ambiguity!

```cpp
// diamond_problem.cpp
#include <iostream>
using namespace std;

class Base {
public:
    int value;

    Base() : value(0) {
        cout << "Base constructor" << endl;
    }

    void show() {
        cout << "Base::show() - value = " << value << endl;
    }
};

class Derived1 : public Base {
public:
    Derived1() {
        cout << "Derived1 constructor" << endl;
    }
};

class Derived2 : public Base {
public:
    Derived2() {
        cout << "Derived2 constructor" << endl;
    }
};

// WHY: Final has TWO copies of Base (through Derived1 and Derived2)
class Final : public Derived1, public Derived2 {
public:
    Final() {
        cout << "Final constructor" << endl;
    }
};

int main() {
    Final f;

    // WHY: Ambiguous! Which Base::value? From Derived1 or Derived2?
    // f.value = 10;  // ERROR: ambiguous
    // f.show();      // ERROR: ambiguous

    // WHY: Must specify which Base
    f.Derived1::value = 10;
    f.Derived2::value = 20;

    cout << "Via Derived1: " << f.Derived1::value << endl;
    cout << "Via Derived2: " << f.Derived2::value << endl;

    f.Derived1::show();
    f.Derived2::show();

    return 0;
}
```

**Output:**

```
Base constructor
Derived1 constructor
Base constructor
Derived2 constructor
Final constructor
Via Derived1: 10
Via Derived2: 20
Base::show() - value = 10
Base::show() - value = 20
```

**Problem**: Two Base constructors called, two copies of Base members!

### 3.2 Solution: Virtual Inheritance

**Solution**: Use `virtual` keyword in inheritance to share single copy of base class.

**Syntax**: `class Derived : virtual public Base { };`

```cpp
// virtual_inheritance_solution.cpp
#include <iostream>
using namespace std;

class Base {
public:
    int value;

    Base() : value(0) {
        cout << "Base constructor" << endl;
    }

    void show() {
        cout << "Base::show() - value = " << value << endl;
    }
};

// WHY: virtual inheritance - share single Base
class Derived1 : virtual public Base {
public:
    Derived1() {
        cout << "Derived1 constructor" << endl;
    }
};

class Derived2 : virtual public Base {
public:
    Derived2() {
        cout << "Derived2 constructor" << endl;
    }
};

// WHY: Final has only ONE copy of Base
class Final : public Derived1, public Derived2 {
public:
    Final() {
        cout << "Final constructor" << endl;
    }
};

int main() {
    Final f;

    // WHY: No ambiguity! Only one Base
    f.value = 42;
    f.show();

    cout << "Value: " << f.value << endl;

    return 0;
}
```

**Output:**

```
Base constructor
Derived1 constructor
Derived2 constructor
Final constructor
Base::show() - value = 42
Value: 42
```

**Solution**: Only ONE Base constructor called, single shared copy!

### 3.3 Virtual Inheritance Details

**How it works:**

- Most derived class (Final) is responsible for initializing virtual base
- Only one instance of virtual base class exists
- Slight performance overhead (extra pointer indirection)

**When to use:**

- Multiple inheritance creating diamond shape
- Need shared base class state

**When NOT to use:**

- Single inheritance (no diamond)
- Performance-critical code (has small overhead)

---

## 4. Polymorphism and Virtual Functions

### 4.1 Types of Polymorphism

**Two Types:**

1. **Compile-Time (Static) Polymorphism**
    - Function overloading
    - Operator overloading
    - Resolved at compile time
    - No runtime overhead
2. **Runtime (Dynamic) Polymorphism**
    - Virtual functions
    - Function overriding
    - Resolved at runtime via vtable
    - Slight runtime overhead

```cpp
// polymorphism_types.cpp
#include <iostream>
using namespace std;

// Compile-time polymorphism: Function overloading
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    double add(double a, double b) {
        return a + b;
    }

    int add(int a, int b, int c) {
        return a + b + c;
    }
};

// Runtime polymorphism: Virtual functions
class Animal {
public:
    virtual void makeSound() {
        cout << "Animal makes a sound" << endl;
    }

    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    void makeSound() override {
        cout << "Dog: Woof! Woof!" << endl;
    }
};

class Cat : public Animal {
public:
    void makeSound() override {
        cout << "Cat: Meow! Meow!" << endl;
    }
};

int main() {
    cout << "=== Compile-Time Polymorphism ===" << endl;
    Calculator calc;
    cout << calc.add(5, 3) << endl;
    cout << calc.add(2.5, 3.7) << endl;
    cout << calc.add(1, 2, 3) << endl;

    cout << "\n=== Runtime Polymorphism ===" << endl;
    Animal* animals[3];
    animals[0] = new Animal();
    animals[1] = new Dog();
    animals[2] = new Cat();

    for (int i = 0; i < 3; i++) {
        animals[i]->makeSound();  // Polymorphic call
        delete animals[i];
    }

    return 0;
}
```

### 4.2 Virtual Functions Deep Dive

**Why virtual keyword?**

- Enables runtime polymorphism
- Allows derived class to override base class method
- Function call resolved at runtime (dynamic binding)

**Without virtual:**

```cpp
Base* ptr = new Derived();
ptr->func();  // Calls Base::func() (early binding)
```

**With virtual:**

```cpp
Base* ptr = new Derived();
ptr->func();  // Calls Derived::func() (late binding)
```

```cpp
// virtual_functions_example.cpp
#include <iostream>
using namespace std;

class Shape {
protected:
    string color;

public:
    Shape(string c) : color(c) {}

    // WHY: virtual - enables polymorphism
    virtual void draw() {
        cout << "Drawing a shape" << endl;
    }

    virtual double area() {
        return 0.0;
    }

    // WHY: Virtual destructor for proper cleanup
    virtual ~Shape() {
        cout << "Shape destructor" << endl;
    }
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(string c, double r) : Shape(c), radius(r) {}

    void draw() override {
        cout << "Drawing " << color << " circle" << endl;
    }

    double area() override {
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
    Rectangle(string c, double w, double h)
        : Shape(c), width(w), height(h) {}

    void draw() override {
        cout << "Drawing " << color << " rectangle" << endl;
    }

    double area() override {
        return width * height;
    }

    ~Rectangle() {
        cout << "Rectangle destructor" << endl;
    }
};

void displayShapeInfo(Shape* shape) {
    // WHY: Polymorphism - calls correct derived class method
    shape->draw();
    cout << "Area: " << shape->area() << endl;
    cout << endl;
}

int main() {
    Shape* shapes[2];
    shapes[0] = new Circle("Red", 5.0);
    shapes[1] = new Rectangle("Blue", 10.0, 6.0);

    for (int i = 0; i < 2; i++) {
        displayShapeInfo(shapes[i]);
        delete shapes[i];  // Virtual destructor ensures proper cleanup
    }

    return 0;
}
```

---

## 5. vtable and vptr Mechanism

### 5.1 Understanding vtable and vptr

**Core Concepts:**

**vtable (Virtual Table)**:

- Table of function pointers maintained PER CLASS
- Contains addresses of virtual functions
- One vtable per class (not per object)
- Created at compile time

**vptr (Virtual Pointer)**:

- Hidden pointer added by compiler as first member of object
- Points to class's vtable
- One vptr per object containing virtual functions
- Set in constructor

**Why this mechanism?**

- Enables runtime polymorphism
- Determines which function to call at runtime
- Efficient implementation of virtual functions

### 5.2 How vtable/vptr Works

**Memory Layout:**

```
Object in memory:
┌──────────────┐
│ vptr         │ ← Points to vtable
├──────────────┤
│ member1      │
│ member2      │
│ ...          │
└──────────────┘

vtable:
┌──────────────┐
│ &func1       │ ← Address of virtual function 1
│ &func2       │ ← Address of virtual function 2
│ &func3       │ ← Address of virtual function 3
└──────────────┘
```

```cpp
// vtable_vptr_example.cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void func1() {
        cout << "Base::func1()" << endl;
    }

    virtual void func2() {
        cout << "Base::func2()" << endl;
    }

    virtual void func3() {
        cout << "Base::func3()" << endl;
    }

    virtual ~Base() {}
};

class Derived1 : public Base {
public:
    // WHY: Override only func1
    void func1() override {
        cout << "Derived1::func1()" << endl;
    }

    // func2 and func3 use Base versions
};

class Derived2 : public Derived1 {
public:
    // WHY: Override func1 and func2
    void func1() override {
        cout << "Derived2::func1()" << endl;
    }

    void func2() override {
        cout << "Derived2::func2()" << endl;
    }

    // func3 uses Base version
};

int main() {
    cout << "=== vtable Demonstration ===" << endl;

    Base* ptr;

    // WHY: vptr points to Base's vtable
    ptr = new Base();
    ptr->func1();  // Base::func1
    ptr->func2();  // Base::func2
    ptr->func3();  // Base::func3
    delete ptr;

    cout << endl;

    // WHY: vptr points to Derived1's vtable
    ptr = new Derived1();
    ptr->func1();  // Derived1::func1
    ptr->func2();  // Base::func2 (not overridden)
    ptr->func3();  // Base::func3 (not overridden)
    delete ptr;

    cout << endl;

    // WHY: vptr points to Derived2's vtable
    ptr = new Derived2();
    ptr->func1();  // Derived2::func1
    ptr->func2();  // Derived2::func2
    ptr->func3();  // Base::func3 (not overridden)
    delete ptr;

    return 0;
}
```

**Output:**

```
=== vtable Demonstration ===
Base::func1()
Base::func2()
Base::func3()

Derived1::func1()
Base::func2()
Base::func3()

Derived2::func1()
Derived2::func2()
Base::func3()
```

### 5.3 vtable Construction

**Base Class vtable:**

```
Base_vtable:
[0] → Base::func1()
[1] → Base::func2()
[2] → Base::func3()
```

**Derived1 Class vtable:**

```
Derived1_vtable:
[0] → Derived1::func1()  ← Overridden
[1] → Base::func2()      ← Inherited
[2] → Base::func3()      ← Inherited

```

**Derived2 Class vtable:**

```
Derived2_vtable:
[0] → Derived2::func1()  ← Overridden
[1] → Derived2::func2()  ← Overridden
[2] → Base::func3()      ← Inherited
```

### 5.4 Performance Impact

**Cost of Virtual Functions:**

1. **Memory overhead**: vptr added to each object (typically 8 bytes on 64-bit)
2. **Runtime overhead**: Extra indirection (vptr → vtable → function)
3. **Less optimization**: Compiler can't inline virtual functions easily

**Benchmark:**

```
Direct call:     ~1 ns
Virtual call:    ~3-4 ns
```

**When to use:**

- When you need polymorphism (runtime type determination)
- Benefit outweighs small performance cost
- Most modern applications: negligible impact

**When NOT to use:**

- Tight inner loops with millions of calls
- Embedded systems with strict performance requirements
- When compile-time polymorphism (templates) suffices

---

## 6. override and final Keywords

### 6.1 The override Keyword (C++11)

**Purpose**: Explicitly indicate function overrides base class virtual function.

**Why override?**

- Catch errors at compile time
- Prevents accidental function hiding
- Makes code intention clear
- Safer refactoring

**Without override:**

```cpp
class Base {
public:
    virtual void func() { }
};

class Derived : public Base {
public:
    void func(int x) { }  // Oops! Different signature, doesn't override
    // Compiles fine but doesn't do what you expect
};
```

**With override:**

```cpp
class Derived : public Base {
public:
    void func(int x) override { }  // ERROR! Doesn't match base signature
};
```

```cpp
// override_keyword.cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void display() {
        cout << "Base::display()" << endl;
    }

    virtual void show() const {
        cout << "Base::show() const" << endl;
    }

    virtual ~Base() {}
};

class DerivedCorrect : public Base {
public:
    // WHY: override catches signature mismatches
    void display() override {  // OK - matches base
        cout << "DerivedCorrect::display()" << endl;
    }

    void show() const override {  // OK - matches base (const)
        cout << "DerivedCorrect::show() const" << endl;
    }
};

class DerivedWrong : public Base {
public:
    // WHY: Without override, this compiles but doesn't override
    void display(int x) {  // Different signature!
        cout << "DerivedWrong::display(int)" << endl;
    }

    // WHY: With override, compiler catches error
    // void show() override {  // ERROR! Missing const
    //     cout << "DerivedWrong::show()" << endl;
    // }
};

int main() {
    Base* ptr;

    ptr = new DerivedCorrect();
    ptr->display();  // Calls DerivedCorrect::display()
    ptr->show();     // Calls DerivedCorrect::show()
    delete ptr;

    cout << endl;

    ptr = new DerivedWrong();
    ptr->display();  // Calls Base::display() (not overridden!)
    delete ptr;

    return 0;
}
```

### 6.2 The final Keyword (C++11)

**Purpose**: Prevent function from being overridden OR class from being inherited.

**Two Uses:**

1. **final function** - Cannot be overridden in derived classes
2. **final class** - Cannot be inherited

**Why final?**

- Safety: Prevent accidental modification
- Optimization: Compiler can devirtualize calls
- Design: Communicate "this should not be changed"

```cpp
// final_keyword.cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void canOverride() {
        cout << "Base::canOverride()" << endl;
    }

    // WHY: final - cannot be overridden
    virtual void cannotOverride() final {
        cout << "Base::cannotOverride() - FINAL" << endl;
    }

    virtual ~Base() {}
};

class Derived : public Base {
public:
    // WHY: OK - canOverride is not final
    void canOverride() override {
        cout << "Derived::canOverride()" << endl;
    }

    // WHY: ERROR - cannot override final function
    // void cannotOverride() override {
    //     cout << "Derived::cannotOverride()" << endl;
    // }
};

// WHY: final class - cannot be inherited
class FinalClass final {
public:
    void display() {
        cout << "FinalClass::display()" << endl;
    }
};

// WHY: ERROR - cannot inherit from final class
// class TryInherit : public FinalClass {
// };

int main() {
    Derived d;
    d.canOverride();
    d.cannotOverride();

    FinalClass fc;
    fc.display();

    return 0;
}
```

### 6.3 override vs final Comparison

| Aspect | override | final |
| --- | --- | --- |
| **Purpose** | Indicate overriding intent | Prevent overriding/inheritance |
| **Compile error if** | Not overriding base virtual | Try to override/inherit |
| **Use case** | Safety, clarity | Design decision, optimization |
| **On functions** | Yes | Yes |
| **On classes** | No | Yes |

---

## 7. Object Slicing Problem

### 7.1 Understanding Object Slicing

**Definition**: When derived class object is assigned to base class object (by value), derived class specific members are "sliced off".

**Problem**: Loss of derived class data, polymorphism doesn't work.

**Why it happens**: Base class object only has space for base class members.

```cpp
// object_slicing_problem.cpp
#include <iostream>
using namespace std;

class Base {
protected:
    int baseData;

public:
    Base(int val = 0) : baseData(val) {}

    virtual void display() {
        cout << "Base: baseData = " << baseData << endl;
    }

    virtual ~Base() {}
};

class Derived : public Base {
private:
    int derivedData;  // WHY: This will be sliced off!

public:
    Derived(int base, int derived)
        : Base(base), derivedData(derived) {}

    void display() override {
        cout << "Derived: baseData = " << baseData
             << ", derivedData = " << derivedData << endl;
    }
};

int main() {
    Derived d(10, 20);

    cout << "=== Original Derived Object ===" << endl;
    d.display();

    cout << "\n=== Object Slicing (by value) ===" << endl;
    Base b = d;  // WHY: derivedData is sliced off!
    b.display(); // Calls Base::display(), not Derived::display()

    cout << "\n=== No Slicing (by pointer) ===" << endl;
    Base* ptr = &d;
    ptr->display();  // Calls Derived::display() via polymorphism

    cout << "\n=== No Slicing (by reference) ===" << endl;
    Base& ref = d;
    ref.display();  // Calls Derived::display() via polymorphism

    return 0;
}
```

**Output:**

```
=== Original Derived Object ===
Derived: baseData = 10, derivedData = 20

=== Object Slicing (by value) ===
Base: baseData = 10

=== No Slicing (by pointer) ===
Derived: baseData = 10, derivedData = 20

=== No Slicing (by reference) ===
Derived: baseData = 10, derivedData = 20
```

### 7.2 Object Slicing in Function Calls

**Dangerous:**

```cpp
void processShape(Shape s) {  // Pass by value - SLICING!
    s.draw();  // Always calls Shape::draw()
}
```

**Safe:**

```cpp
void processShape(Shape& s) {  // Pass by reference - NO slicing
    s.draw();  // Calls correct derived class draw()
}

void processShape(Shape* s) {  // Pass by pointer - NO slicing
    s->draw();  // Calls correct derived class draw()
}
```

```cpp
// slicing_in_functions.cpp
#include <iostream>
using namespace std;

class Animal {
public:
    virtual void makeSound() {
        cout << "Animal sound" << endl;
    }

    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    void makeSound() override {
        cout << "Woof! Woof!" << endl;
    }
};

// WHY: Pass by value - SLICING occurs
void byValue(Animal a) {
    a.makeSound();  // Always calls Animal::makeSound()
}

// WHY: Pass by reference - NO slicing
void byReference(Animal& a) {
    a.makeSound();  // Polymorphic - calls correct version
}

// WHY: Pass by pointer - NO slicing
void byPointer(Animal* a) {
    a->makeSound();  // Polymorphic - calls correct version
}

int main() {
    Dog dog;

    cout << "Direct call:" << endl;
    dog.makeSound();

    cout << "\nPass by value (SLICING):" << endl;
    byValue(dog);  // Slicing! Calls Animal::makeSound()

    cout << "\nPass by reference (NO slicing):" << endl;
    byReference(dog);  // Polymorphism works

    cout << "\nPass by pointer (NO slicing):" << endl;
    byPointer(&dog);  // Polymorphism works

    return 0;
}
```

### 7.3 Preventing Object Slicing

**Methods:**

1. **Use pointers or references for polymorphism**
    
    ```cpp
    Base* ptr = new Derived();  // Good
    Base& ref = derived_obj;    // Good
    Base obj = derived_obj;     // BAD - slicing
    ```
    
2. **Delete copy constructor/assignment in base**
    
    ```cpp
    class Base {
    public:
        Base(const Base&) = delete;
        Base& operator=(const Base&) = delete;
    };
    ```
    
3. **Make base class abstract (pure virtual functions)**
    
    ```cpp
    class Base {
    public:
        virtual void func() = 0;  // Cannot instantiate Base
    };
    ```
    

---

## Summary

### Key Takeaways

1. **Inheritance Types** - Single (one parent), Multiple (two+ parents), Multilevel (chain A→B→C), Hierarchical (one parent, many children), Hybrid (combination). Use public inheritance for "is-a" relationships. Constructor order: base→derived, destructor order: derived→base.
2. **Inheritance Modes** - public (maintains access), protected (public→protected), private (all→private). Private base members NEVER accessible. public inheritance most common (99%). Table: Base public→public/protected/private in derived depends on mode. Use public unless specific reason.
3. **Diamond Problem** - Occurs in multiple inheritance when derived class inherits from two classes sharing common base (A→B,C→D). Results in two copies of base class members, ambiguity. Solution: virtual inheritance (`class B : virtual public A`). Only one base copy, no ambiguity. Slight performance cost.
4. **Polymorphism Types** - Compile-time (function/operator overloading, resolved compile-time, no overhead). Runtime (virtual functions, resolved runtime via vtable, small overhead). Virtual keyword enables runtime polymorphism. Allows base pointer to call derived functions dynamically.
5. **vtable & vptr Mechanism** - vtable: table of function pointers per class, contains virtual function addresses. vptr: hidden pointer in each object, points to class vtable, set in constructor. How works: object→vptr→vtable→function address. Memory overhead: 8 bytes per object (64-bit). Runtime overhead: ~3-4ns vs ~1ns direct call.
6. **override Keyword (C++11)** - Explicitly marks function as overriding base virtual. Compiler error if not actually overriding (catches signature mismatches). Benefits: safety, prevents errors, clear intent. Always use when overriding. Example: `void func() override { }`
7. **final Keyword (C++11)** - Prevents function override or class inheritance. Two uses: (1) final function cannot be overridden, (2) final class cannot be inherited. Benefits: safety, optimization (devirtualization), design clarity. Use when method/class should never change.
8. **Object Slicing Problem** - When derived object assigned to base object by value, derived members "sliced off". Lost: derived data, polymorphism behavior. Causes: pass/return by value, assignment. Solution: use pointers/references for polymorphism. Delete copy constructor in base, or make base abstract.
9. **Virtual Function Performance** - Memory cost: vptr per object (~8 bytes). Runtime cost: extra indirection through vtable (~2-3ns overhead). When acceptable: most applications, benefit outweighs cost. When avoid: tight loops, embedded systems. Modern CPUs handle well, negligible in most code.
10. **Best Practices** - Use public inheritance for "is-a". Always override keyword when overriding. Always virtual destructor in base with virtuals. Prefer references/pointers over by-value for polymorphism. Use final sparingly, for explicit design. Make base abstract to prevent slicing. Document inheritance relationships clearly.

### Interview Essential Questions

**Q1: Explain the five types of inheritance with real-world examples. When would you use each?**

A: Five types: (1) Single: Dog→Animal (one parent), most common, clear hierarchy. (2) Multiple: TA→Student,Teacher (two parents), use when object genuinely has multiple unrelated aspects, rare due to diamond problem. (3) Multilevel: ElectricCar→Car→Vehicle (chain), use for progressive specialization, clear hierarchy like classification system. (4) Hierarchical: Circle,Rectangle,Triangle→Shape (one parent, many children), use for common base with many variations, like shapes or vehicles. (5) Hybrid: combination of above types, use for complex relationships like Car→FourWheeler+Engine (multilevel+multiple).

Real-world: Manager inherits Person (identity) and Employee (work) - multiple inheritance. Tesla inherits Car inherits Vehicle - multilevel. Circle, Square inherit Shape - hierarchical.

Use single by default (90%+ cases). Multiple rare, causes diamond problem, prefer composition. Multilevel for natural hierarchies. Hierarchical for families of related classes. Public inheritance for "is-a", private for "implemented-in-terms-of" (rare).

**Q2: What is diamond problem in C++? How do you solve it? What is cost of solution?**

A: Diamond problem occurs in multiple inheritance: class D inherits from B and C, both B and C inherit from A. Creates diamond shape: A→B,C→D. Problem: D has two copies of A's members (one through B, one through C), causing ambiguity when accessing A's members - compiler doesn't know which copy to use.

Example: `d.value` ambiguous - is it from B's copy of A or C's copy? Must qualify: `d.B::value` or `d.C::value`.

Solution: Virtual inheritance. Declare B and C as `class B : virtual public A`. This ensures only ONE shared copy of A exists in D. No ambiguity, can access directly: `d.value`.

Cost of virtual inheritance: (1) Memory overhead: extra pointer to virtual base (8 bytes on 64-bit), (2) Initialization complexity: most derived class (D) must directly initialize virtual base A, (3) Performance: extra indirection when accessing virtual base members, (4) Complexity: harder to understand/maintain.

Use when: genuinely need multiple inheritance with common base. Avoid when: can redesign using composition or interfaces. Most production code avoids diamond problem entirely through design.

**Q3: Explain vtable and vptr. How does C++ implement virtual functions? What is performance cost?**

A: vtable (virtual table): table of function pointers maintained per class, contains addresses of all virtual functions for that class. Created at compile time. One vtable per class (not per object). If derived overrides function, vtable contains derived function address; otherwise base function address.

vptr (virtual pointer): hidden pointer added by compiler as first member of each object with virtual functions. Points to class's vtable. One vptr per object. Set in constructor to point to correct class vtable.

How works: When calling virtual function through base pointer, runtime process: (1) Access object's vptr, (2) Follow vptr to vtable, (3) Look up function address in vtable using index, (4) Call function at that address. This is runtime polymorphism.

Example: Base* ptr = new Derived(); ptr->func(); → accesses Derived object's vptr → points to Derived_vtable → gets Derived::func address → calls Derived::func.

Performance cost: (1) Memory: 8 bytes vptr per object (64-bit), (2) Call overhead: ~2-3ns extra vs direct call (~1ns). Extra indirection: vptr→vtable→function. (3) Cache effects: vtable access may miss cache. (4) Compiler optimization: can't inline virtual calls easily.

Acceptable when: polymorphism needed, most applications (negligible), benefit outweighs small cost. Avoid when: tight inner loops millions of times, embedded systems, can use templates instead.

**Q4: What is object slicing? How to prevent it? Demonstrate with code showing problem and solution.**

A: Object slicing: when derived class object assigned to base class object by value, derived class-specific members are "sliced off", only base class part remains. Results in: (1) loss of derived data, (2) polymorphism doesn't work, (3) calls base function instead of derived.

Why happens: base object only has memory for base members, no space for derived members. Copy constructor copies only base part.

Problem code:

```cpp
Derived d(10, 20);  // derivedData=20
Base b = d;         // SLICING! derivedData lost
b.display();        // Calls Base::display, not Derived::display
```

Pass by value slicing:

```cpp
void func(Base b) { b.display(); }  // Slicing when called
func(derived_obj);  // Loses polymorphism
```

Prevention: (1) Use pointers/references for polymorphism: `Base* ptr = &derived; Base& ref = derived;` (2) Delete copy constructor in base: `Base(const Base&) = delete;` (3) Make base abstract with pure virtual: `virtual void func() = 0;` prevents instantiating base. (4) Pass by pointer/reference: `void func(Base& b)` or `void func(Base* b)`.

Best practice: always use pointer/reference for polymorphic types, never pass/return base class by value when using inheritance hierarchy.

**Q5: Compare override and final keywords. When and why use each? What compile errors do they catch?**

A: Both C++11 features for safer inheritance.

override keyword: Explicitly states function intends to override base virtual function. Compiler checks: (1) base has function with same name, (2) base function is virtual, (3) signatures exactly match (parameters, const, return type). If any check fails: compile error.

Catches: (1) Typos: `void dispaly() override` - error, no base function. (2) Signature mismatch: `void show() override` when base is `show() const` - error. (3) Not virtual: overriding non-virtual - error. (4) Hidden functions: prevents accidentally creating new function instead of overriding.

Use when: always when overriding virtual functions. Makes intent explicit, catches errors early, documents code.

final keyword: Two uses - (1) final function: prevents derived classes from overriding. Syntax: `virtual void func() final;`. (2) final class: prevents inheritance. Syntax: `class Base final { };`.

Catches: (1) Trying override final function - compile error. (2) Trying inherit final class - compile error.

Use when: (1) Design decision: method/class should never be changed. (2) Optimization: compiler can devirtualize final virtual functions. (3) Safety: prevent accidental modification in large codebase.

Difference: override about correctness of overriding, final about prevention of overriding. Use override always when overriding, use final rarely when have specific reason to prevent changes.

---