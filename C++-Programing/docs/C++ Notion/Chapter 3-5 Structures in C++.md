# 3.5. Structures in C++

---

## Table of Contents

1. What are Structures?
2. Structure Declaration and Definition
3. Accessing and Modifying Structure Members
4. Structure Initialization
5. Member Functions in Structures
6. Structure Size and Memory Layout
7. Structure Padding and Alignment
8. Nested Structures
9. Pointer to Structure
10. Array of Structures
11. Structures with Functions
12. Bit Fields
13. typedef with Structures
14. Anonymous Structures
15. Structure vs Class
16. Self-Referential Structures
17. Best Practices and Common Pitfalls

---

## 1. What are Structures?

### 1.1 Definition

**A structure (struct) is a user-defined data type that groups variables of different data types under a single name.** Unlike arrays which store elements of the same type, structures can combine different types into a logical unit.

**Core Concept:**

```cpp
#include <iostream>
using namespace std;

// WHY: Group related student information
struct Student {
    string name;
    int rollNo;
    float gpa;
};

int main() {
    Student s1;
    s1.name = "Alice";
    s1.rollNo = 101;
    s1.gpa = 3.8;

    cout << "Name: " << s1.name << endl;
    cout << "Roll No: " << s1.rollNo << endl;
    cout << "GPA: " << s1.gpa << endl;

    return 0;
}
```

**Output:**

```
Name: Alice
Roll No: 101
GPA: 3.8
```

### 1.2 Why Structures Exist

**Purpose:**

1. **Logical Grouping**
    - Related data under one name
    - Better code organization
2. **Data Modeling**
    - Represent real-world entities
    - Complex data relationships
3. **Code Reusability**
    - Define once, use many times
    - Easy to pass to functions
4. **Memory Efficiency**
    - Contiguous memory allocation
    - Cache-friendly access

**Real-World Applications:**

- Database records (Employee, Product, Order)
- Graphics programming (Point, Color, Vector)
- Game development (Player, Enemy, Item)
- System programming (File descriptor, Process info)
- Embedded systems (Sensor data, Configuration)

### 1.3 Structures vs Arrays

| Feature | Structure | Array |
| --- | --- | --- |
| **Data types** | Different types | Same type only |
| **Access** | By member name | By index |
| **Purpose** | Group related data | Store multiple similar items |
| **Memory** | Padded for alignment | Contiguous, no padding |
| **Example** | `Student s;` | `int arr[10];` |

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Array stores same type
    int scores[3] = {85, 92, 78};

    // WHY: Structure stores different types
    struct Student {
        string name;
        int roll;
        float gpa;
    } s1 = {"Bob", 102, 3.5};

    cout << "Array: " << scores[0] << endl;
    cout << "Struct: " << s1.name << endl;

    return 0;
}
```

---

## 2. Structure Declaration and Definition

### 2.1 Basic Syntax

**Structure Definition:**

```cpp
struct structure_name {
    data_type member1;
    data_type member2;
    ...
};
```

**Example:**

```cpp
#include <iostream>
using namespace std;

// WHY: Define structure template
struct Point {
    int x;
    int y;
};

int main() {
    // WHY: Create structure variable
    Point p1;
    p1.x = 10;
    p1.y = 20;

    cout << "Point: (" << p1.x << ", " << p1.y << ")" << endl;

    return 0;
}
```

### 2.2 Declaration Methods

**Method 1: Separate declaration**

```cpp
// Define structure
struct Employee {
    int id;
    string name;
    float salary;
};

// Declare variables later
Employee emp1, emp2;
```

**Method 2: Declaration with definition**

```cpp
struct Employee {
    int id;
    string name;
    float salary;
} emp1, emp2;  // Variables declared here
```

**Method 3: Anonymous structure**

```cpp
struct {
    int x;
    int y;
} point1, point2;  // No structure name, only variables
```

### 2.3 C++ vs C Differences

```cpp
#include <iostream>
using namespace std;

struct Date {
    int day, month, year;
};

int main() {
    // C++ way (struct keyword not required)
    Date today;
    today.day = 15;

    // C way (struct keyword required)
    // struct Date tomorrow;  // Not needed in C++

    return 0;
}
```

---

## 3. Accessing and Modifying Structure Members

### 3.1 Dot Operator (.)

```cpp
#include <iostream>
using namespace std;

struct Rectangle {
    int length;
    int width;
};

int main() {
    Rectangle rect;

    // WHY: Use dot operator to access members
    rect.length = 10;
    rect.width = 5;

    cout << "Area: " << rect.length * rect.width << endl;

    // WHY: Read members
    cout << "Length: " << rect.length << endl;
    cout << "Width: " << rect.width << endl;

    return 0;
}
```

**Output:**

```
Area: 50
Length: 10
Width: 5
```

### 3.2 Accessing Nested Members

```cpp
#include <iostream>
using namespace std;

struct Address {
    string city;
    int pincode;
};

struct Person {
    string name;
    Address addr;  // Nested structure
};

int main() {
    Person p;

    // WHY: Chain dot operators for nested access
    p.name = "Charlie";
    p.addr.city = "New York";
    p.addr.pincode = 10001;

    cout << p.name << " lives in " << p.addr.city << endl;

    return 0;
}
```

---

## 4. Structure Initialization

### 4.1 Aggregate Initialization

```cpp
#include <iostream>
using namespace std;

struct Point {
    int x;
    int y;
};

int main() {
    // WHY: Initialize with braces (aggregate initialization)
    Point p1 = {10, 20};
    Point p2 = {30, 40};
    Point p3 = {50};  // y defaults to 0

    cout << "p1: (" << p1.x << ", " << p1.y << ")" << endl;
    cout << "p3: (" << p3.x << ", " << p3.y << ")" << endl;

    return 0;
}
```

**Output:**

```
p1: (10, 20)
p3: (50, 0)
```

### 4.2 Designated Initializers (C++20)

```cpp
#include <iostream>
using namespace std;

struct Config {
    int timeout;
    bool debug;
    string mode;
};

int main() {
    // WHY: Initialize by member name (order independent)
    Config cfg = {
        .debug = true,
        .mode = "verbose",
        .timeout = 30
    };

    cout << "Debug: " << cfg.debug << endl;
    cout << "Mode: " << cfg.mode << endl;

    return 0;
}
```

### 4.3 Member-by-Member Initialization

```cpp
#include <iostream>
using namespace std;

struct Book {
    string title;
    string author;
    int pages;
    float price;
};

int main() {
    Book b;

    // WHY: Initialize one by one
    b.title = "C++ Primer";
    b.author = "Lippman";
    b.pages = 976;
    b.price = 59.99;

    cout << b.title << " by " << b.author << endl;

    return 0;
}
```

### 4.4 Copy Initialization

```cpp
#include <iostream>
using namespace std;

struct Color {
    int r, g, b;
};

int main() {
    Color red = {255, 0, 0};

    // WHY: Copy entire structure
    Color anotherRed = red;

    cout << "R: " << anotherRed.r << endl;

    return 0;
}
```

---

## 5. Member Functions in Structures

### 5.1 Basic Member Functions

```cpp
#include <iostream>
using namespace std;

struct Circle {
    float radius;

    // WHY: Member function to calculate area
    float area() {
        return 3.14159 * radius * radius;
    }

    // WHY: Member function to calculate circumference
    float circumference() {
        return 2 * 3.14159 * radius;
    }
};

int main() {
    Circle c;
    c.radius = 5.0;

    cout << "Area: " << c.area() << endl;
    cout << "Circumference: " << c.circumference() << endl;

    return 0;
}
```

**Output:**

```
Area: 78.5398
Circumference: 31.4159
```

### 5.2 Constructors in Structures

```cpp
#include <iostream>
using namespace std;

struct Vector3D {
    float x, y, z;

    // WHY: Default constructor
    Vector3D() : x(0), y(0), z(0) {}

    // WHY: Parameterized constructor
    Vector3D(float a, float b, float c) : x(a), y(b), z(c) {}

    // WHY: Member function
    void display() {
        cout << "(" << x << ", " << y << ", " << z << ")" << endl;
    }
};

int main() {
    Vector3D v1;           // Default constructor
    Vector3D v2(1, 2, 3);  // Parameterized constructor

    v1.display();
    v2.display();

    return 0;
}
```

**Output:**

```
(0, 0, 0)
(1, 2, 3)
```

### 5.3 Destructor in Structures

```cpp
#include <iostream>
using namespace std;

struct Resource {
    int* data;

    // Constructor
    Resource(int size) {
        data = new int[size];
        cout << "Resource acquired" << endl;
    }

    // WHY: Destructor for cleanup
    ~Resource() {
        delete[] data;
        cout << "Resource released" << endl;
    }
};

int main() {
    Resource r(100);
    // Destructor called automatically when r goes out of scope

    return 0;
}
```

**Output:**

```
Resource acquired
Resource released
```

### 5.4 Access Specifiers in Structures

```cpp
#include <iostream>
using namespace std;

struct BankAccount {
private:
    double balance;  // Private member

public:
    string accountNumber;

    // Constructor
    BankAccount(string accNo, double initial)
        : accountNumber(accNo), balance(initial) {}

    // WHY: Public getter
    double getBalance() {
        return balance;
    }

    // WHY: Public setter with validation
    void deposit(double amount) {
        if (amount > 0) {
            balance += amount;
        }
    }
};

int main() {
    BankAccount acc("123456", 1000.0);

    // acc.balance = 5000;  // ❌ ERROR: private member

    acc.deposit(500);
    cout << "Balance: $" << acc.getBalance() << endl;

    return 0;
}
```

---

## 6. Structure Size and Memory Layout

### 6.1 sizeof Operator

```cpp
#include <iostream>
using namespace std;

struct Small {
    char c;
    int i;
};

struct Large {
    double d1;
    double d2;
    int arr[10];
};

int main() {
    cout << "Size of Small: " << sizeof(Small) << " bytes" << endl;
    cout << "Size of Large: " << sizeof(Large) << " bytes" << endl;
    cout << "Size of char: " << sizeof(char) << endl;
    cout << "Size of int: " << sizeof(int) << endl;
    cout << "Size of double: " << sizeof(double) << endl;

    return 0;
}
```

**Output (typical 64-bit system):**

```
Size of Small: 8 bytes
Size of Large: 56 bytes
Size of char: 1
Size of int: 4
Size of double: 8
```

### 6.2 Memory Layout Visualization

```cpp
#include <iostream>
using namespace std;

struct Data {
    int a;
    char b;
    int c;
};

int main() {
    Data d;

    // WHY: Verify contiguous allocation
    cout << "Address of d: " << &d << endl;
    cout << "Address of d.a: " << &d.a << endl;
    cout << "Address of d.b: " << (void*)&d.b << endl;
    cout << "Address of d.c: " << &d.c << endl;

    cout << "\nSize of Data: " << sizeof(Data) << " bytes" << endl;

    return 0;
}
```

**Sample Output:**

```
Address of d: 0x7ffd12345678
Address of d.a: 0x7ffd12345678
Address of d.b: 0x7ffd1234567C
Address of d.c: 0x7ffd12345680

Size of Data: 12 bytes
```

---

## 7. Structure Padding and Alignment

### 7.1 Why Padding Exists

**Memory Alignment:** CPUs access aligned data faster. Padding ensures members are aligned to their natural boundaries.

```cpp
#include <iostream>
using namespace std;

struct Unoptimized {
    char c;    // 1 byte + 3 bytes padding
    int i;     // 4 bytes
    char d;    // 1 byte + 3 bytes padding
};

struct Optimized {
    int i;     // 4 bytes
    char c;    // 1 byte
    char d;    // 1 byte + 2 bytes padding
};

int main() {
    cout << "Unoptimized size: " << sizeof(Unoptimized) << " bytes" << endl;
    cout << "Optimized size: " << sizeof(Optimized) << " bytes" << endl;

    return 0;
}
```

**Output:**

```
Unoptimized size: 12 bytes
Optimized size: 8 bytes
```

### 7.2 Padding Calculation

```cpp
#include <iostream>
using namespace std;

struct Example {
    char a;    // 1 byte
    // 3 bytes padding
    int b;     // 4 bytes
    char c;    // 1 byte
    // 3 bytes padding (to make total multiple of 4)
};

int main() {
    Example e;

    cout << "Size: " << sizeof(Example) << " bytes" << endl;
    cout << "Expected without padding: " << (1 + 4 + 1) << " bytes" << endl;

    cout << "\nMember offsets:" << endl;
    cout << "a: " << offsetof(Example, a) << endl;
    cout << "b: " << offsetof(Example, b) << endl;
    cout << "c: " << offsetof(Example, c) << endl;

    return 0;
}
```

**Output:**

```
Size: 12 bytes
Expected without padding: 6 bytes

Member offsets:
a: 0
b: 4
c: 8
```

### 7.3 #pragma pack

```cpp
#include <iostream>
using namespace std;

// Normal structure (with padding)
struct Normal {
    char c;
    int i;
    char d;
};

// Packed structure (no padding)
#pragma pack(push, 1)
struct Packed {
    char c;
    int i;
    char d;
};
#pragma pack(pop)

int main() {
    cout << "Normal size: " << sizeof(Normal) << " bytes" << endl;
    cout << "Packed size: " << sizeof(Packed) << " bytes" << endl;

    return 0;
}
```

**Output:**

```
Normal size: 12 bytes
Packed size: 6 bytes
```

**⚠️ Warning:** Packing can slow down access and cause alignment issues!

---

## 8. Nested Structures

### 8.1 Structure within Structure

```cpp
#include <iostream>
using namespace std;

struct Date {
    int day;
    int month;
    int year;
};

struct Employee {
    int id;
    string name;
    Date joiningDate;  // Nested structure
};

int main() {
    Employee emp;
    emp.id = 101;
    emp.name = "David";
    emp.joiningDate.day = 15;
    emp.joiningDate.month = 6;
    emp.joiningDate.year = 2020;

    cout << emp.name << " joined on ";
    cout << emp.joiningDate.day << "/"
         << emp.joiningDate.month << "/"
         << emp.joiningDate.year << endl;

    return 0;
}
```

**Output:**

```
David joined on 15/6/2020
```

### 8.2 Initialization of Nested Structures

```cpp
#include <iostream>
using namespace std;

struct Address {
    string street;
    string city;
    int zip;
};

struct Company {
    string name;
    Address location;
};

int main() {
    // WHY: Nested braces for nested initialization
    Company comp = {
        "TechCorp",
        {"123 Main St", "Boston", 02101}
    };

    cout << comp.name << endl;
    cout << comp.location.street << ", " << comp.location.city << endl;

    return 0;
}
```

### 8.3 Anonymous Nested Structure

```cpp
#include <iostream>
using namespace std;

struct Outer {
    int x;

    // WHY: Anonymous inner structure
    struct {
        int y;
        int z;
    } inner;
};

int main() {
    Outer obj;
    obj.x = 10;
    obj.inner.y = 20;
    obj.inner.z = 30;

    cout << "x: " << obj.x << endl;
    cout << "y: " << obj.inner.y << endl;
    cout << "z: " << obj.inner.z << endl;

    return 0;
}
```

---

## 9. Pointer to Structure

### 9.1 Arrow Operator (->)

```cpp
#include <iostream>
using namespace std;

struct Student {
    string name;
    int age;

    void display() {
        cout << name << ", Age: " << age << endl;
    }
};

int main() {
    Student s = {"Emma", 20};
    Student* ptr = &s;

    // WHY: Two ways to access via pointer
    cout << "Using (*ptr).member: " << (*ptr).name << endl;
    cout << "Using ptr->member: " << ptr->name << endl;

    // WHY: Arrow operator for member functions
    ptr->display();

    return 0;
}
```

**Output:**

```
Using (*ptr).member: Emma
Using ptr->member: Emma
Emma, Age: 20
```

### 9.2 Dynamic Structure Allocation

```cpp
#include <iostream>
using namespace std;

struct Node {
    int data;
    Node* next;
};

int main() {
    // WHY: Allocate structure dynamically
    Node* head = new Node;
    head->data = 10;
    head->next = nullptr;

    Node* second = new Node{20, nullptr};
    head->next = second;

    // Traverse
    Node* current = head;
    while (current != nullptr) {
        cout << current->data << " ";
        current = current->next;
    }

    // Cleanup
    delete head;
    delete second;

    return 0;
}
```

**Output:**

```
10 20
```

---

## 10. Array of Structures

### 10.1 Declaring and Initializing

```cpp
#include <iostream>
using namespace std;

struct Product {
    int id;
    string name;
    float price;
};

int main() {
    // WHY: Array of structures
    Product inventory[3] = {
        {101, "Laptop", 999.99},
        {102, "Mouse", 25.50},
        {103, "Keyboard", 75.00}
    };

    // Display all products
    for (int i = 0; i < 3; i++) {
        cout << inventory[i].name << ": $"
             << inventory[i].price << endl;
    }

    return 0;
}
```

**Output:**

```
Laptop: $999.99
Mouse: $25.5
Keyboard: $75
```

### 10.2 Dynamic Array of Structures

```cpp
#include <iostream>
using namespace std;

struct Score {
    string player;
    int points;
};

int main() {
    int n = 3;

    // WHY: Dynamic allocation
    Score* scores = new Score[n];

    scores[0] = {"Alice", 95};
    scores[1] = {"Bob", 87};
    scores[2] = {"Charlie", 92};

    for (int i = 0; i < n; i++) {
        cout << scores[i].player << ": "
             << scores[i].points << endl;
    }

    delete[] scores;

    return 0;
}
```

---

## 11. Structures with Functions

### 11.1 Pass by Value

```cpp
#include <iostream>
using namespace std;

struct Point {
    int x, y;
};

// WHY: Pass by value (copy created)
void printPoint(Point p) {
    cout << "(" << p.x << ", " << p.y << ")" << endl;
    p.x = 999;  // Doesn't affect original
}

int main() {
    Point pt = {10, 20};

    printPoint(pt);
    cout << "Original: " << pt.x << endl;  // Still 10

    return 0;
}
```

### 11.2 Pass by Reference

```cpp
#include <iostream>
using namespace std;

struct Rectangle {
    int length, width;
};

// WHY: Pass by reference (no copy, can modify)
void scale(Rectangle& rect, int factor) {
    rect.length *= factor;
    rect.width *= factor;
}

int main() {
    Rectangle r = {5, 10};

    scale(r, 2);

    cout << "Scaled: " << r.length << " x " << r.width << endl;

    return 0;
}
```

**Output:**

```
Scaled: 10 x 20
```

### 11.3 Return Structure from Function

```cpp
#include <iostream>
using namespace std;

struct Complex {
    double real, imag;
};

// WHY: Return structure by value
Complex add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

int main() {
    Complex c1 = {3.0, 4.0};
    Complex c2 = {1.5, 2.5};

    Complex sum = add(c1, c2);

    cout << sum.real << " + " << sum.imag << "i" << endl;

    return 0;
}
```

**Output:**

```
4.5 + 6.5i
```

---

## 12. Bit Fields

### 12.1 Basic Bit Fields

```cpp
#include <iostream>
using namespace std;

struct Flags {
    unsigned int flag1 : 1;  // 1 bit
    unsigned int flag2 : 1;  // 1 bit
    unsigned int flag3 : 1;  // 1 bit
    unsigned int value : 5;  // 5 bits
};

int main() {
    Flags f;
    f.flag1 = 1;
    f.flag2 = 0;
    f.flag3 = 1;
    f.value = 31;  // Max for 5 bits (2^5 - 1)

    cout << "Size: " << sizeof(Flags) << " byte" << endl;
    cout << "flag1: " << f.flag1 << endl;
    cout << "value: " << f.value << endl;

    return 0;
}
```

**Output:**

```
Size: 4 bytes
flag1: 1
value: 31
```

### 12.2 Use Cases for Bit Fields

```cpp
#include <iostream>
using namespace std;

// WHY: Embedded systems - compact data representation
struct StatusRegister {
    unsigned int ready      : 1;
    unsigned int error      : 1;
    unsigned int interrupt  : 1;
    unsigned int reserved   : 5;  // Unused bits
};

int main() {
    StatusRegister status = {1, 0, 1, 0};

    if (status.ready && status.interrupt) {
        cout << "Device ready with interrupt pending" << endl;
    }

    return 0;
}
```

---

## 13. typedef with Structures

### 13.1 Creating Type Alias

```cpp
#include <iostream>
using namespace std;

// Method 1: typedef with struct
typedef struct {
    int hours;
    int minutes;
} Time;

// Method 2: typedef after definition
struct Date {
    int day, month, year;
};
typedef Date MyDate;

int main() {
    Time t = {14, 30};
    MyDate d = {25, 12, 2024};

    cout << "Time: " << t.hours << ":" << t.minutes << endl;
    cout << "Date: " << d.day << "/" << d.month << "/" << d.year << endl;

    return 0;
}
```

### 13.2 using vs typedef (C++11)

```cpp
#include <iostream>
using namespace std;

struct Point3D {
    float x, y, z;
};

// Old style
typedef Point3D Vec3;

// New style (C++11) - preferred
using Vector3 = Point3D;

int main() {
    Vec3 v1 = {1, 2, 3};
    Vector3 v2 = {4, 5, 6};

    cout << "v1: (" << v1.x << ", " << v1.y << ", " << v1.z << ")" << endl;

    return 0;
}
```

---

## 14. Anonymous Structures

### 14.1 Unnamed Structures

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: No structure name, only variables
    struct {
        int x;
        int y;
    } point1, point2;

    point1.x = 10;
    point1.y = 20;

    cout << "Point: (" << point1.x << ", " << point1.y << ")" << endl;

    // Cannot create more variables of this "type"

    return 0;
}
```

### 14.2 Anonymous Union in Structure

```cpp
#include <iostream>
using namespace std;

struct Pixel {
    union {  // Anonymous union
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

    cout << "Color value: " << p.color << endl;

    return 0;
}
```

---

## 15. Structure vs Class

### 15.1 Key Differences

| Feature | Structure | Class |
| --- | --- | --- |
| **Default access** | `public` | `private` |
| **Keyword** | `struct` | `class` |
| **Typical use** | Data grouping | OOP with encapsulation |
| **Inheritance** | Public by default | Private by default |

```cpp
#include <iostream>
using namespace std;

struct MyStruct {
    int x;  // Public by default
};

class MyClass {
    int x;  // Private by default
public:
    int y;
};

int main() {
    MyStruct s;
    s.x = 10;  // ✅ OK: public

    MyClass c;
    // c.x = 20;  // ❌ ERROR: private
    c.y = 30;  // ✅ OK: explicitly public

    return 0;
}
```

### 15.2 When to Use Each

**Use struct when:**

- Simple data aggregation
- No complex behavior
- Public access is appropriate
- C compatibility needed

**Use class when:**

- Need encapsulation
- Complex behavior/invariants
- Private data members
- Full OOP features

---

## 16. Self-Referential Structures

### 16.1 Linked List Node

```cpp
#include <iostream>
using namespace std;

struct Node {
    int data;
    Node* next;  // Pointer to same type
};

int main() {
    // WHY: Create linked list
    Node* head = new Node{1, nullptr};
    head->next = new Node{2, nullptr};
    head->next->next = new Node{3, nullptr};

    // Traverse
    Node* current = head;
    while (current != nullptr) {
        cout << current->data << " -> ";
        current = current->next;
    }
    cout << "NULL" << endl;

    // Cleanup (simplified - should use proper loop)
    delete head->next->next;
    delete head->next;
    delete head;

    return 0;
}
```

**Output:**

```
1 -> 2 -> 3 -> NULL
```

### 16.2 Tree Node

```cpp
#include <iostream>
using namespace std;

struct TreeNode {
    int data;
    TreeNode* left;
    TreeNode* right;
};

TreeNode* createNode(int value) {
    TreeNode* node = new TreeNode;
    node->data = value;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}

int main() {
    TreeNode* root = createNode(10);
    root->left = createNode(5);
    root->right = createNode(15);

    cout << "Root: " << root->data << endl;
    cout << "Left child: " << root->left->data << endl;
    cout << "Right child: " << root->right->data << endl;

    return 0;
}
```

---

## 17. Best Practices and Common Pitfalls

### 17.1 Common Mistakes

**Mistake 1: Forgetting initialization**

```cpp
// ❌ WRONG: Uninitialized members contain garbage
struct Data {
    int x;
    float y;
};
Data d;  // x and y have garbage values!

// ✅ CORRECT: Always initialize
Data d2 = {0, 0.0};
```

**Mistake 2: Ignoring alignment**

```cpp
// ❌ INEFFICIENT: Bad ordering
struct Bad {
    char c1;    // 1 + 3 padding
    int i;      // 4
    char c2;    // 1 + 3 padding
};  // Total: 12 bytes

// ✅ EFFICIENT: Good ordering
struct Good {
    int i;      // 4
    char c1;    // 1
    char c2;    // 1 + 2 padding
};  // Total: 8 bytes
```

**Mistake 3: Unsafe pointer usage**

```cpp
// ❌ WRONG: Dangling pointer
struct Node* getBadNode() {
    Node local = {10, nullptr};
    return &local;  // Local destroyed!
}

// ✅ CORRECT: Dynamic allocation
struct Node* getGoodNode() {
    return new Node{10, nullptr};
}
```

### 17.2 Best Practices

**✅ Initialize all members**

```cpp
struct Config {
    int timeout = 30;    // Default value
    bool debug = false;
};
```

**✅ Use const for read-only parameters**

```cpp
void display(const Employee& emp) {
    // Cannot modify emp
}
```

**✅ Consider alignment when ordering members**

```cpp
struct Optimized {
    double d;  // 8 bytes (largest first)
    int i;     // 4 bytes
    short s;   // 2 bytes
    char c;    // 1 byte
};  // Minimal padding
```

**✅ Use constructors for initialization**

```cpp
struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int a, int b) : x(a), y(b) {}
};
```

---

## Summary

### Key Takeaways

1. **Structures group different types** - Unlike arrays, structures combine variables of different data types under one name
2. **Public by default** - Struct members are public by default (unlike classes which are private by default)
3. **Dot operator for access** - Use `.` for direct access, `>` for pointer access
4. **Padding for alignment** - Compiler adds padding bytes to align members to natural boundaries for performance
5. **Can have member functions** - C++ structures support constructors, destructors, and member functions
6. **Nested structures allowed** - Structures can contain other structures as members
7. **sizeof includes padding** - Structure size is sum of member sizes plus padding
8. **Self-referential via pointers** - Can contain pointers to same type (for linked lists, trees)
9. **typedef creates aliases** - Make structure names shorter and more convenient
10. **Pass by reference preferred** - Avoid expensive copies when passing structures to functions

### Interview Essential Points

**Q: What is a structure and how does it differ from an array?**

A: A structure is a user-defined data type that groups variables of different types under a single name, while an array stores multiple elements of the same type. Structures use member names for access (e.g., `student.name`), arrays use indices (`arr[0]`). Structures are ideal for representing entities with multiple attributes (like a Person with name, age, address), while arrays store collections of similar items. Memory-wise, structures may have padding between members for alignment, while array elements are stored contiguously with no padding.

**Q: Explain structure padding and why it exists.**

A: Structure padding is extra bytes inserted by the compiler between members to ensure proper memory alignment. CPUs access data faster when it's aligned to addresses that are multiples of the data size (e.g., 4-byte int at address divisible by 4). Without padding, accessing misaligned data requires multiple memory operations. Example: `struct {char c; int i;}` becomes 8 bytes (1 byte char + 3 bytes padding + 4 bytes int) instead of 5, because the int must start at a 4-byte boundary. You can optimize by ordering largest members first. Use `#pragma pack` to remove padding if needed (at performance cost).

**Q: What is the difference between structure and class in C++?**

A: The only technical differences are: (1) Default access - struct members are public by default, class members are private by default. (2) Default inheritance - struct inherits publicly by default, class inherits privately. Functionally, they're identical - both support member functions, constructors, destructors, inheritance, and polymorphism. Convention: use struct for simple data aggregates (POD types) with public data and minimal behavior; use class for objects with encapsulation, private data, and complex behavior. The choice is stylistic once you add access specifiers.

**Q: How do you access structure members through a pointer?**

A: Two methods: (1) Dereference-then-dot: `(*ptr).member` - dereference pointer first, then use dot operator. (2) Arrow operator: `ptr->member` - shorthand for the same operation. Arrow operator is preferred for readability. Example: `Student* ptr = &s; cout << ptr->name;` is cleaner than `cout << (*ptr).name;`. Internally, both do the same thing - dereference the pointer to access the structure, then access the member by offset from base address.

**Q: What are self-referential structures and where are they used?**

A: Self-referential structures contain pointers to the same structure type as members, enabling recursive data structures. Cannot contain the actual structure (infinite size), only pointers. Example: `struct Node { int data; Node* next; };`. Used extensively in: (1) Linked lists - each node points to next, (2) Trees - nodes have left/right child pointers, (3) Graphs - nodes contain pointers to adjacent nodes. Essential for dynamic data structures where size isn't known at compile time. The pointer breaks the recursion - it's always the same size regardless of how many nodes exist.

**Q: How can you minimize structure size?**

A: Three approaches: (1) Order members largest to smallest - reduces padding by grouping similar-sized types: `struct {double d; int i; short s; char c;}` has less padding than random ordering. (2) Use bit fields for flags - `unsigned int flag : 1;` uses 1 bit instead of whole byte. (3) Use `#pragma pack(1)` to remove all padding - but this hurts performance as CPU must do multiple accesses for misaligned data. Best practice: optimize ordering first, only pack if memory is critical (embedded systems). Always measure - modern compilers are smart about alignment.

**Q: Explain nested structures with initialization syntax.**

A: Nested structures are structures containing other structures as members. Access uses chained dot operators: `outer.inner.member`. Initialization requires nested braces matching the structure hierarchy: `Outer o = {{inner_values}, outer_values};`. Example: `struct Address {string city; int zip;}; struct Person {string name; Address addr;};` initialized as `Person p = {"Alice", {"NYC", 10001}};`. Can also initialize step by step: `p.addr.city = "NYC";`. Nested structures model has-a relationships (Person has an Address) and are common in real-world data modeling.

**Q: What are bit fields and when should you use them?**

A: Bit fields allow specifying exact number of bits for structure members: `unsigned int flag : 1;` uses 1 bit instead of full int (4 bytes). Syntax: `type name : num_bits;`. Mainly used in: (1) Memory-constrained environments (embedded systems, communication protocols), (2) Hardware register mapping where each bit has specific meaning, (3) Flags/options that need compact storage. Limitations: Cannot take address of bit field, no arrays of bit fields, implementation-defined packing order. Trade-off: saves memory but costs CPU time (masking/shifting operations). Use only when memory is more precious than performance.

---