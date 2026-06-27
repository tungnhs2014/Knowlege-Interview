# 10.5. Callbacks

---

## Table of Contents

1. Introduction to Callbacks
2. Function Pointers as Callbacks
3. Functors (Function Objects)
4. Lambda Expressions
5. std::function
6. std::bind
7. Member Function Callbacks
8. Callback Patterns
9. Real-World Examples
10. Best Practices
11. Summary

---

## 1. Introduction to Callbacks

### What is a Callback?

A **callback** is a function that is passed as an argument to another function and is executed at a later time. The receiving function can "call back" the passed function when needed.

**Think of it as:** Giving someone your phone number so they can call you back later.

### Why Do We Need Callbacks?

1. **Event-Driven Programming**: Handle user interactions
2. **Asynchronous Operations**: Get notified when operations complete
3. **Customization**: Allow users to customize behavior
4. **Separation of Concerns**: Decouple "what" from "when"
5. **Higher-Order Functions**: Enable functional programming

### Callback Mechanisms in C++

| Mechanism | Stateful | Flexibility |
| --- | --- | --- |
| Function Pointer | ❌ No | Low |
| Functor | ✅ Yes | Medium |
| Lambda | ✅ Yes | High |
| std::function | ✅ Yes | Highest |

---

## 2. Function Pointers as Callbacks

### Function Pointer Syntax

```cpp
// Syntax: return_type (*pointer_name)(parameter_types);
int (*funcPtr)(int, int);  // Pointer to function taking two ints
```

### Basic Function Pointer Example

```cpp
#include <iostream>
using namespace std;

// WHY: Simple functions to demonstrate callbacks
int add(int a, int b) { return a + b; }
int subtract(int a, int b) { return a - b; }
int multiply(int a, int b) { return a * b; }

int main() {
    // WHY: Declare function pointer
    int (*operation)(int, int);

    // Point to different functions
    operation = add;
    cout << "Add: " << operation(10, 5) << endl;

    operation = subtract;
    cout << "Subtract: " << operation(10, 5) << endl;

    operation = multiply;
    cout << "Multiply: " << operation(10, 5) << endl;

    return 0;
}
```

**Output:**

```
Add: 15
Subtract: 5
Multiply: 50
```

### Function Pointer as Callback Parameter

```cpp
#include <iostream>
using namespace std;

int square(int x) { return x * x; }
int cube(int x) { return x * x * x; }

// WHY: Function that takes a callback
void processArray(int arr[], int size, int (*callback)(int)) {
    cout << "Processed: ";
    for (int i = 0; i < size; i++) {
        cout << callback(arr[i]) << " ";
    }
    cout << endl;
}

int main() {
    int numbers[] = {1, 2, 3, 4, 5};
    int size = 5;

    // WHY: Pass different callbacks
    cout << "Squares:" << endl;
    processArray(numbers, size, square);

    cout << "Cubes:" << endl;
    processArray(numbers, size, cube);

    return 0;
}
```

**Output:**

```
Squares:
Processed: 1 4 9 16 25
Cubes:
Processed: 1 8 27 64 125
```

### Using typedef for Cleaner Syntax

```cpp
#include <iostream>
using namespace std;

// WHY: typedef makes function pointer syntax cleaner
typedef int (*MathOperation)(int, int);
// Or C++11 using:
using MathOp = int (*)(int, int);

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

// WHY: Much cleaner parameter declaration
void calculate(int x, int y, MathOperation op) {
    cout << "Result: " << op(x, y) << endl;
}

int main() {
    calculate(10, 5, add);
    calculate(10, 5, mul);
    return 0;
}
```

**Output:**

```
Result: 15
Result: 50
```

### Limitations of Function Pointers

```cpp
#include <iostream>
using namespace std;

int main() {
    int multiplier = 3;

    // WHY: Function pointers CANNOT capture state!
    // This won't work:
    // auto func = [multiplier](int x) { return x * multiplier; };
    // int (*ptr)(int) = func;  // ERROR: Capturing lambda!

    // Only non-capturing lambdas convert to function pointers
    auto nonCapturing = [](int x) { return x * 2; };
    int (*ptr)(int) = nonCapturing;  // OK!

    cout << ptr(5) << endl;
    return 0;
}
```

---

## 3. Functors (Function Objects)

### What is a Functor?

A **functor** is a class that overloads `operator()`, allowing objects of that class to be called like functions. Unlike function pointers, functors can maintain state.

### Basic Functor Example

```cpp
#include <iostream>
using namespace std;

// WHY: Functor class with operator()
class Adder {
    int value;
public:
    Adder(int v) : value(v) {}

    // WHY: Overload operator() to make object callable
    int operator()(int x) const {
        return x + value;
    }
};

int main() {
    Adder addFive(5);
    Adder addTen(10);

    // WHY: Call functor like a function
    cout << "5 + 5 = " << addFive(5) << endl;
    cout << "5 + 10 = " << addTen(5) << endl;

    return 0;
}
```

**Output:**

```
5 + 5 = 10
5 + 10 = 15
```

### Functor with State

```cpp
#include <iostream>
using namespace std;

// WHY: Functor that maintains state across calls
class Counter {
    int count;
public:
    Counter() : count(0) {}

    int operator()() {
        return ++count;  // WHY: Increment and return
    }

    int getCount() const { return count; }
};

int main() {
    Counter counter;

    // WHY: Each call increments internal state
    cout << "Call 1: " << counter() << endl;
    cout << "Call 2: " << counter() << endl;
    cout << "Call 3: " << counter() << endl;
    cout << "Total: " << counter.getCount() << endl;

    return 0;
}
```

**Output:**

```
Call 1: 1
Call 2: 2
Call 3: 3
Total: 3
```

### Functor as Callback with Templates

```cpp
#include <iostream>
#include <vector>
using namespace std;

// WHY: Comparator functor for sorting
class DescendingCompare {
public:
    bool operator()(int a, int b) const {
        return a > b;  // WHY: Descending order
    }
};

// WHY: Template accepts any callable
template<typename Compare>
void sortPair(int& a, int& b, Compare comp) {
    if (comp(b, a)) {
        swap(a, b);
    }
}

int main() {
    int x = 5, y = 10;

    DescendingCompare desc;
    sortPair(x, y, desc);

    cout << "After sort (desc): " << x << ", " << y << endl;

    return 0;
}
```

**Output:**

```
After sort (desc): 10, 5
```

---

## 4. Lambda Expressions

### What is a Lambda?

A **lambda expression** (C++11) is an anonymous function that can be defined inline. It can capture variables from its surrounding scope.

### Lambda Syntax

```cpp
[capture](parameters) -> return_type { body }
```

### Basic Lambda Examples

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Simple lambda with no capture
    auto greet = []() {
        cout << "Hello, Lambda!" << endl;
    };
    greet();

    // WHY: Lambda with parameters
    auto add = [](int a, int b) {
        return a + b;
    };
    cout << "Sum: " << add(3, 4) << endl;

    // WHY: Lambda with explicit return type
    auto divide = [](double a, double b) -> double {
        return a / b;
    };
    cout << "Division: " << divide(10.0, 3.0) << endl;

    return 0;
}
```

**Output:**

```
Hello, Lambda!
Sum: 7
Division: 3.33333
```

### Capture Modes

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;
    int y = 20;

    // WHY: Capture by value [=]
    auto byValue = [=]() {
        cout << "By value: x=" << x << ", y=" << y << endl;
        // x = 15;  // ERROR: Cannot modify
    };

    // WHY: Capture by reference [&]
    auto byRef = [&]() {
        x = 15;  // OK: Can modify
        cout << "By ref: x=" << x << endl;
    };

    // WHY: Specific captures [x, &y]
    auto mixed = [x, &y]() {
        // x is by value, y is by reference
        y = 25;
        cout << "Mixed: x=" << x << ", y=" << y << endl;
    };

    byValue();
    byRef();
    mixed();

    cout << "After: x=" << x << ", y=" << y << endl;

    return 0;
}
```

**Output:**

```
By value: x=10, y=20
By ref: x=15
Mixed: x=15, y=25
After: x=15, y=25
```

### Mutable Lambdas

```cpp
#include <iostream>
using namespace std;

int main() {
    int counter = 0;

    // WHY: mutable allows modifying captured-by-value variables
    auto increment = [counter]() mutable {
        counter++;  // OK with mutable
        return counter;
    };

    cout << "Call 1: " << increment() << endl;
    cout << "Call 2: " << increment() << endl;
    cout << "Original counter: " << counter << endl;  // Still 0!

    return 0;
}
```

**Output:**

```
Call 1: 1
Call 2: 2
Original counter: 0
```

### Lambda as Callback

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> numbers = {5, 2, 8, 1, 9, 3};

    // WHY: Lambda as callback to sort
    sort(numbers.begin(), numbers.end(),
         [](int a, int b) { return a > b; });  // Descending

    cout << "Sorted descending: ";
    for (int n : numbers) cout << n << " ";
    cout << endl;

    // WHY: Lambda with capture for filtering
    int threshold = 4;
    auto it = find_if(numbers.begin(), numbers.end(),
                      [threshold](int x) { return x < threshold; });

    if (it != numbers.end()) {
        cout << "First below " << threshold << ": " << *it << endl;
    }

    return 0;
}
```

**Output:**

```
Sorted descending: 9 8 5 3 2 1
First below 4: 3
```

### Generic Lambdas (C++14)

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: auto parameters make generic lambda (C++14)
    auto print = [](auto value) {
        cout << "Value: " << value << endl;
    };

    print(42);
    print(3.14);
    print("Hello");
    print(string("World"));

    // WHY: Generic lambda for comparisons
    auto greater = [](auto a, auto b) {
        return a > b;
    };

    cout << "5 > 3: " << greater(5, 3) << endl;
    cout << "2.5 > 3.5: " << greater(2.5, 3.5) << endl;

    return 0;
}
```

**Output:**

```
Value: 42
Value: 3.14
Value: Hello
Value: World
5 > 3: 1
2.5 > 3.5: 0
```

---

## 5. std::function

### What is std::function?

**std::function** (C++11) is a general-purpose polymorphic function wrapper. It can store any callable: functions, lambdas, functors, or bind expressions.

### Basic std::function Usage

```cpp
#include <iostream>
#include <functional>
using namespace std;

int add(int a, int b) { return a + b; }

class Multiplier {
public:
    int operator()(int a, int b) { return a * b; }
};

int main() {
    // WHY: std::function can hold any callable with matching signature
    function<int(int, int)> operation;

    // Hold a regular function
    operation = add;
    cout << "Add: " << operation(5, 3) << endl;

    // Hold a functor
    operation = Multiplier();
    cout << "Multiply: " << operation(5, 3) << endl;

    // Hold a lambda
    operation = [](int a, int b) { return a - b; };
    cout << "Subtract: " << operation(5, 3) << endl;

    return 0;
}
```

**Output:**

```
Add: 8
Multiply: 15
Subtract: 2
```

### std::function as Callback Parameter

```cpp
#include <iostream>
#include <functional>
#include <vector>
using namespace std;

// WHY: Use std::function for flexible callback parameter
void processData(const vector<int>& data,
                 function<void(int)> callback) {
    for (int value : data) {
        callback(value);
    }
}

int main() {
    vector<int> numbers = {1, 2, 3, 4, 5};

    // WHY: Pass different callbacks
    cout << "Doubled: ";
    processData(numbers, [](int n) { cout << n * 2 << " "; });
    cout << endl;

    int sum = 0;
    processData(numbers, [&sum](int n) { sum += n; });
    cout << "Sum: " << sum << endl;

    return 0;
}
```

**Output:**

```
Doubled: 2 4 6 8 10
Sum: 15
```

### Storing Callbacks in Containers

```cpp
#include <iostream>
#include <functional>
#include <vector>
#include <string>
using namespace std;

class EventHandler {
    vector<function<void(const string&)>> handlers;

public:
    // WHY: Store callbacks for later execution
    void addHandler(function<void(const string&)> handler) {
        handlers.push_back(handler);
    }

    void trigger(const string& event) {
        cout << "Event triggered: " << event << endl;
        for (auto& handler : handlers) {
            handler(event);
        }
    }
};

int main() {
    EventHandler eh;

    // WHY: Register multiple callbacks
    eh.addHandler([](const string& e) {
        cout << "Handler 1: " << e << endl;
    });

    eh.addHandler([](const string& e) {
        cout << "Handler 2: Processing " << e << endl;
    });

    eh.trigger("ButtonClick");

    return 0;
}
```

**Output:**

```
Event triggered: ButtonClick
Handler 1: ButtonClick
Handler 2: Processing ButtonClick
```

### Checking if std::function is Valid

```cpp
#include <iostream>
#include <functional>
using namespace std;

void executeIfValid(function<void()> callback) {
    // WHY: Check if std::function is empty
    if (callback) {
        callback();
    } else {
        cout << "No callback provided" << endl;
    }
}

int main() {
    function<void()> valid = []() { cout << "Valid callback!" << endl; };
    function<void()> empty;  // Empty std::function

    executeIfValid(valid);
    executeIfValid(empty);
    executeIfValid(nullptr);  // Also empty

    return 0;
}
```

**Output:**

```
Valid callback!
No callback provided
No callback provided
```

---

## 6. std::bind

### What is std::bind?

**std::bind** (C++11) creates a callable wrapper that binds arguments to a function. It allows partial application and argument reordering.

### Basic std::bind Usage

```cpp
#include <iostream>
#include <functional>
using namespace std;

int add(int a, int b, int c) {
    return a + b + c;
}

int main() {
    using namespace std::placeholders;

    // WHY: Bind all arguments
    auto addFixed = bind(add, 1, 2, 3);
    cout << "Fixed: " << addFixed() << endl;  // 1+2+3=6

    // WHY: Bind some arguments, leave placeholders
    auto add10 = bind(add, 10, _1, _2);
    cout << "10+5+3: " << add10(5, 3) << endl;  // 10+5+3=18

    // WHY: Reorder arguments
    auto reordered = bind(add, _3, _1, _2);
    cout << "Reordered(1,2,3): " << reordered(1, 2, 3) << endl;  // 3+1+2=6

    return 0;
}
```

**Output:**

```
Fixed: 6
10+5+3: 18
Reordered(1,2,3): 6
```

### Partial Application

```cpp
#include <iostream>
#include <functional>
using namespace std;

int multiply(int a, int b) {
    return a * b;
}

int main() {
    using namespace std::placeholders;

    // WHY: Create specialized functions via partial application
    auto double_it = bind(multiply, 2, _1);
    auto triple_it = bind(multiply, 3, _1);
    auto times_ten = bind(multiply, 10, _1);

    cout << "Double 5: " << double_it(5) << endl;
    cout << "Triple 5: " << triple_it(5) << endl;
    cout << "Times 10 of 5: " << times_ten(5) << endl;

    return 0;
}
```

**Output:**

```
Double 5: 10
Triple 5: 15
Times 10 of 5: 50
```

### Lambda vs bind

```cpp
#include <iostream>
#include <functional>
using namespace std;

int process(int a, int b, int c) {
    return a * 100 + b * 10 + c;
}

int main() {
    using namespace std::placeholders;

    // Using bind
    auto withBind = bind(process, 1, _1, 3);

    // WHY: Equivalent lambda (often clearer)
    auto withLambda = [](int b) { return process(1, b, 3); };

    cout << "bind: " << withBind(2) << endl;
    cout << "lambda: " << withLambda(2) << endl;

    // WHY: Lambda is often preferred for readability
    return 0;
}
```

**Output:**

```
bind: 123
lambda: 123
```

---

## 7. Member Function Callbacks

### Calling Member Functions

```cpp
#include <iostream>
#include <functional>
using namespace std;

class Calculator {
public:
    int add(int a, int b) { return a + b; }
    int multiply(int a, int b) { return a * b; }
};

int main() {
    Calculator calc;

    // WHY: Using std::function with member functions
    function<int(Calculator*, int, int)> op;

    op = &Calculator::add;
    cout << "Add: " << op(&calc, 5, 3) << endl;

    op = &Calculator::multiply;
    cout << "Multiply: " << op(&calc, 5, 3) << endl;

    return 0;
}
```

**Output:**

```
Add: 8
Multiply: 15
```

### Using std::bind with Member Functions

```cpp
#include <iostream>
#include <functional>
using namespace std;

class Greeter {
    string prefix;
public:
    Greeter(string p) : prefix(p) {}

    void greet(const string& name) const {
        cout << prefix << " " << name << "!" << endl;
    }
};

int main() {
    Greeter hello("Hello");
    Greeter goodbye("Goodbye");

    using namespace std::placeholders;

    // WHY: Bind member function to specific object
    auto greetWithHello = bind(&Greeter::greet, &hello, _1);
    auto greetWithGoodbye = bind(&Greeter::greet, &goodbye, _1);

    greetWithHello("Alice");
    greetWithGoodbye("Bob");

    return 0;
}
```

**Output:**

```
Hello Alice!
Goodbye Bob!
```

### Lambda Capturing this

```cpp
#include <iostream>
#include <functional>
#include <vector>
using namespace std;

class Timer {
    int interval;
    vector<function<void()>> callbacks;

public:
    Timer(int ms) : interval(ms) {}

    void onTick(function<void()> callback) {
        callbacks.push_back(callback);
    }

    void tick() {
        cout << "Tick at " << interval << "ms" << endl;
        for (auto& cb : callbacks) {
            cb();
        }
    }
};

class Counter {
    int count = 0;
public:
    void setup(Timer& timer) {
        // WHY: Capture 'this' to access member variables
        timer.onTick([this]() {
            count++;
            cout << "Count: " << count << endl;
        });
    }

    int getCount() const { return count; }
};

int main() {
    Timer timer(1000);
    Counter counter;

    counter.setup(timer);

    timer.tick();
    timer.tick();
    timer.tick();

    return 0;
}
```

**Output:**

```
Tick at 1000ms
Count: 1
Tick at 1000ms
Count: 2
Tick at 1000ms
Count: 3
```

---

## 8. Callback Patterns

### Synchronous Callbacks

```cpp
#include <iostream>
#include <functional>
#include <vector>
using namespace std;

// WHY: Callback executed immediately
void forEach(const vector<int>& data, function<void(int)> callback) {
    for (int value : data) {
        callback(value);  // Called synchronously
    }
}

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    cout << "Squared: ";
    forEach(nums, [](int n) { cout << n*n << " "; });
    cout << endl;

    return 0;
}
```

### Completion Callbacks

```cpp
#include <iostream>
#include <functional>
using namespace std;

// WHY: Callback called when operation completes
void performTask(const string& task,
                 function<void(bool, const string&)> onComplete) {
    cout << "Starting: " << task << endl;

    // Simulate task...
    bool success = (task.length() > 3);
    string result = success ? "Task completed" : "Task failed";

    // WHY: Notify via callback
    onComplete(success, result);
}

int main() {
    performTask("Download", [](bool ok, const string& msg) {
        cout << (ok ? "SUCCESS: " : "FAILURE: ") << msg << endl;
    });

    performTask("Go", [](bool ok, const string& msg) {
        cout << (ok ? "SUCCESS: " : "FAILURE: ") << msg << endl;
    });

    return 0;
}
```

**Output:**

```
Starting: Download
SUCCESS: Task completed
Starting: Go
FAILURE: Task failed
```

### Observer Pattern

```cpp
#include <iostream>
#include <functional>
#include <vector>
using namespace std;

class Subject {
    int value = 0;
    vector<function<void(int)>> observers;

public:
    void addObserver(function<void(int)> observer) {
        observers.push_back(observer);
    }

    void setValue(int v) {
        value = v;
        notify();
    }

    void notify() {
        for (auto& observer : observers) {
            observer(value);
        }
    }
};

int main() {
    Subject subject;

    // WHY: Multiple observers react to changes
    subject.addObserver([](int v) {
        cout << "Observer A: Value is " << v << endl;
    });

    subject.addObserver([](int v) {
        cout << "Observer B: Value changed to " << v << endl;
    });

    subject.setValue(10);
    subject.setValue(20);

    return 0;
}
```

**Output:**

```
Observer A: Value is 10
Observer B: Value changed to 10
Observer A: Value is 20
Observer B: Value changed to 20

```

### Event Handler Pattern

```cpp
#include <iostream>
#include <functional>
#include <map>
#include <string>
using namespace std;

class EventEmitter {
    map<string, vector<function<void()>>> listeners;

public:
    void on(const string& event, function<void()> handler) {
        listeners[event].push_back(handler);
    }

    void emit(const string& event) {
        if (listeners.count(event)) {
            for (auto& handler : listeners[event]) {
                handler();
            }
        }
    }
};

int main() {
    EventEmitter emitter;

    emitter.on("click", []() { cout << "Button clicked!" << endl; });
    emitter.on("click", []() { cout << "Analytics: click event" << endl; });
    emitter.on("hover", []() { cout << "Mouse hover detected" << endl; });

    cout << "--- Triggering click ---" << endl;
    emitter.emit("click");

    cout << "--- Triggering hover ---" << endl;
    emitter.emit("hover");

    return 0;
}

```

**Output:**

```
--- Triggering click ---
Button clicked!
Analytics: click event
--- Triggering hover ---
Mouse hover detected
```

---

## 9. Real-World Examples

### Button Click Handler

```cpp
#include <iostream>
#include <functional>
#include <string>
using namespace std;

class Button {
    string label;
    function<void()> onClick;

public:
    Button(const string& lbl) : label(lbl) {}

    void setOnClick(function<void()> handler) {
        onClick = handler;
    }

    void click() {
        cout << "[" << label << "] clicked" << endl;
        if (onClick) onClick();
    }
};

int main() {
    Button submitBtn("Submit");
    Button cancelBtn("Cancel");

    submitBtn.setOnClick([]() {
        cout << "Form submitted!" << endl;
    });

    cancelBtn.setOnClick([]() {
        cout << "Operation cancelled" << endl;
    });

    submitBtn.click();
    cancelBtn.click();

    return 0;
}
```

**Output:**

```
[Submit] clicked
Form submitted!
[Cancel] clicked
Operation cancelled
```

### Custom Sorting

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
using namespace std;

struct Person {
    string name;
    int age;
};

void sortPeople(vector<Person>& people,
                function<bool(const Person&, const Person&)> compare) {
    sort(people.begin(), people.end(), compare);
}

int main() {
    vector<Person> people = {
        {"Alice", 30}, {"Bob", 25}, {"Charlie", 35}
    };

    // Sort by age
    sortPeople(people, [](const Person& a, const Person& b) {
        return a.age < b.age;
    });

    cout << "Sorted by age:" << endl;
    for (const auto& p : people) {
        cout << p.name << " (" << p.age << ")" << endl;
    }

    // Sort by name
    sortPeople(people, [](const Person& a, const Person& b) {
        return a.name < b.name;
    });

    cout << "\nSorted by name:" << endl;
    for (const auto& p : people) {
        cout << p.name << " (" << p.age << ")" << endl;
    }

    return 0;
}
```

**Output:**

```
Sorted by age:
Bob (25)
Alice (30)
Charlie (35)

Sorted by name:
Alice (30)
Bob (25)
Charlie (35)
```

### Simple Timer with Callback

```cpp
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
using namespace std;

class SimpleTimer {
public:
    // WHY: Execute callback after delay
    static void setTimeout(function<void()> callback, int delayMs) {
        cout << "Timer set for " << delayMs << "ms" << endl;
        this_thread::sleep_for(chrono::milliseconds(delayMs));
        callback();
    }
};

int main() {
    cout << "Starting timer..." << endl;

    SimpleTimer::setTimeout([]() {
        cout << "Timer fired!" << endl;
    }, 100);

    cout << "Done" << endl;
    return 0;
}
```

**Output:**

```
Starting timer...
Timer set for 100ms
Timer fired!
Done
```

---

## 10. Best Practices

### ✅ DO: Use std::function for Stored Callbacks

```cpp
class Widget {
    // GOOD: Can store any callable
    function<void()> callback;
public:
    void setCallback(function<void()> cb) { callback = cb; }
};
```

### ✅ DO: Prefer Lambdas for Simple Callbacks

```cpp
// GOOD: Clear and concise
sort(vec.begin(), vec.end(), [](int a, int b) { return a > b; });
```

### ✅ DO: Use Templates for Maximum Performance

```cpp
// GOOD: No std::function overhead
template<typename Callback>
void process(Callback cb) {
    cb();
}
```

### ✅ DO: Check if std::function is Valid

```cpp
function<void()> callback;
if (callback) {  // Check before calling
    callback();
}
```

### ❌ DON'T: Capture by Reference if Callback Outlives Scope

```cpp
function<void()> createDanglingCallback() {
    int local = 42;
    // BAD: local will be destroyed!
    return [&local]() { cout << local; };
}
```

### ❌ DON'T: Overuse std::bind

```cpp
// AVOID: Hard to read
auto f = bind(func, _2, _1, 42, _3);

// BETTER: Use lambda
auto f = [](int a, int b, int c) { return func(b, a, 42, c); };
```

---

## 11. Summary

### Key Takeaways

1. **Function Pointers**: Simplest callbacks, no state
2. **Functors**: Classes with `operator()`, can hold state
3. **Lambdas**: Inline anonymous functions, capture state
4. **std::function**: Type-erased wrapper for any callable
5. **std::bind**: Partial application, argument binding

### Comparison Table

| Feature | Function Ptr | Functor | Lambda | std::function |
| --- | --- | --- | --- | --- |
| State | ❌ | ✅ | ✅ | ✅ |
| Inline | ❌ | ❌ | ✅ | ❌ |
| Store | ❌ | ✅ | Hard | ✅ |
| Performance | Best | Best | Best | Overhead |

### When to Use What

| Situation | Recommendation |
| --- | --- |
| Simple callback, no state | Function pointer or lambda |
| Callback with state | Lambda or functor |
| Store callback for later | std::function |
| STL algorithms | Lambda |
| Member function callback | std::bind or capturing lambda |
| Maximum performance | Template + lambda |

### Keywords Covered

✅ Callbacks (5)
✅ Function pointers (6)
✅ Functors (4)
✅ Lambda expressions (8)
✅ std::function (6)
✅ std::bind (4)
✅ Capture lists (4)
✅ Placeholders (2)
✅ operator() (3)
✅ Callable objects (2)
✅ Event handlers (3)
✅ Observer pattern (2)
✅ Higher-order functions (2)
✅ Type erasure (1)
✅ Partial application (2)

---