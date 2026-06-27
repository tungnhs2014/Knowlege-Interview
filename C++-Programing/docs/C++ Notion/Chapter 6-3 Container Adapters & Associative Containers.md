# 6.3. Container Adapters & Associative Containers

---

## Table of Contents

1. Container Adapters Overview
2. stack - LIFO Container
3. queue - FIFO Container
4. priority_queue - Heap Container
5. Associative Containers - set and map
6. Summary
7. Interview Preparation

---

## 1. Container Adapters Overview

### 1.1 What are Container Adapters?

**Container adapters provide a restricted interface to other containers, designed for specific data structure patterns.**

**Why Adapters Exist:**

Sometimes you don't want full container functionality - you want to enforce specific access patterns. For example, a stack should only allow push/pop from one end, not random access or insertion in the middle.

```cpp
// WHY: Adapters restrict interface to prevent misuse

// With deque - can do anything
deque<int> d;
d.push_back(1);
d.push_front(2);
d[0] = 99;  // Can access anywhere - might break intended use

// With stack - enforced LIFO
stack<int> s;
s.push(1);
s.push(2);
// s[0] = 99;  // ERROR! Cannot access arbitrary positions
int top = s.top();  // Can only access top
```

### 1.2 The Three Standard Adapters

| Adapter | Pattern | Underlying Container | Use Case |
| --- | --- | --- | --- |
| **stack** | LIFO (Last In First Out) | deque (default) | Undo/redo, function calls, DFS |
| **queue** | FIFO (First In First Out) | deque (default) | Task scheduling, BFS |
| **priority_queue** | Heap (Largest first) | vector (default) | Dijkstra, event simulation |

**Key Concept:** Adapters don't store data themselves - they wrap another container and restrict its interface.

---

## 2. stack - LIFO Container

### 2.1 What is stack?

**stack is a container adapter that provides LIFO (Last In First Out) access pattern.**

Header: `#include <stack>`

**Why stack Exists:**

Many algorithms naturally follow LIFO pattern - the last thing added is the first thing removed. Examples: function call stack, undo operations, expression evaluation, depth-first search.

```
LIFO Visualization:

Push 1:  [1]           Pop: [1] → returns 1, stack becomes []
Push 2:  [1][2]        Pop: [1][2] → returns 2, stack becomes [1]
Push 3:  [1][2][3]     Pop: [1][2][3] → returns 3, stack becomes [1][2]
         ↑ TOP              ↑ Always remove from top
```

### 2.2 Basic Operations

```cpp
#include <iostream>
#include <stack>
using namespace std;

int main() {
    // WHY: stack enforces LIFO - cannot accidentally violate pattern
    stack<int> s;

    // Push - Add to top: O(1)
    s.push(10);
    s.push(20);
    s.push(30);

    cout << "Stack size: " << s.size() << endl;
    cout << "Top element: " << s.top() << endl;

    // Pop - Remove from top: O(1)
    s.pop();  // Removes 30
    cout << "After pop, top: " << s.top() << endl;

    // Check if empty
    cout << "Empty? " << (s.empty() ? "Yes" : "No") << endl;

    // Access only top - LIFO enforced
    while(!s.empty()) {
        cout << s.top() << " ";
        s.pop();
    }

    return 0;
}
```

**Output:**

```
Stack size: 3
Top element: 30
After pop, top: 20
Empty? No
20 10
```

### 2.3 stack Operations Summary

| Operation | Time | Description |
| --- | --- | --- |
| `push(x)` | O(1) | Add element to top |
| `pop()` | O(1) | Remove top element (no return!) |
| `top()` | O(1) | Access top element |
| `size()` | O(1) | Number of elements |
| `empty()` | O(1) | Check if empty |

**Critical Note:** `pop()` does NOT return the element - it just removes it. Use `top()` to get the value before `pop()`.

### 2.4 Choosing Underlying Container

```cpp
#include <iostream>
#include <stack>
#include <vector>
#include <list>
using namespace std;

int main() {
    // WHY: Different underlying containers for different needs

    // Default: uses deque
    stack<int> s1;

    // Explicitly use vector
    stack<int, vector<int>> s2;

    // Use list
    stack<int, list<int>> s3;

    // All provide same stack interface
    s1.push(1);
    s2.push(2);
    s3.push(3);

    cout << s1.top() << " " << s2.top() << " " << s3.top() << endl;

    return 0;
}
```

**Output:**

```
1 2 3
```

**Choosing Container:**

- **deque** (default): Good all-around performance
- **vector**: Better cache locality, use if mostly push/pop
- **list**: Use if need iterator stability (rare for stack)

### 2.5 Real-World Use Cases

**Use Case 1: Balanced Parentheses**

```cpp
#include <iostream>
#include <stack>
#include <string>
using namespace std;

bool isBalanced(string expr) {
    // WHY: Stack naturally tracks opening brackets
    stack<char> s;

    for(char ch : expr) {
        if(ch == '(' || ch == '{' || ch == '[') {
            s.push(ch);
        }
        else if(ch == ')' || ch == '}' || ch == ']') {
            if(s.empty()) return false;

            char top = s.top();
            s.pop();

            if((ch == ')' && top != '(') ||
               (ch == '}' && top != '{') ||
               (ch == ']' && top != '[')) {
                return false;
            }
        }
    }

    return s.empty();
}

int main() {
    cout << "({[]}) is " << (isBalanced("({[]})") ? "balanced" : "not balanced") << endl;
    cout << "({[}]) is " << (isBalanced("({[}])") ? "balanced" : "not balanced") << endl;

    return 0;
}
```

**Output:**

```
({[]}) is balanced
({[}]) is not balanced
```

**Use Case 2: Undo/Redo Functionality**

```cpp
#include <iostream>
#include <stack>
#include <string>
using namespace std;

class TextEditor {
    string text;
    stack<string> undoStack;
    stack<string> redoStack;

public:
    // WHY: Stacks naturally model undo/redo history
    void write(string newText) {
        undoStack.push(text);  // Save current state
        text = newText;
        while(!redoStack.empty()) redoStack.pop();  // Clear redo
    }

    void undo() {
        if(undoStack.empty()) return;
        redoStack.push(text);
        text = undoStack.top();
        undoStack.pop();
    }

    void redo() {
        if(redoStack.empty()) return;
        undoStack.push(text);
        text = redoStack.top();
        redoStack.pop();
    }

    string getText() { return text; }
};

int main() {
    TextEditor editor;

    editor.write("Hello");
    cout << "Text: " << editor.getText() << endl;

    editor.write("Hello World");
    cout << "Text: " << editor.getText() << endl;

    editor.undo();
    cout << "After undo: " << editor.getText() << endl;

    editor.redo();
    cout << "After redo: " << editor.getText() << endl;

    return 0;
}
```

**Output:**

```
Text: Hello
Text: Hello World
After undo: Hello
After redo: Hello World
```

---

## 3. queue - FIFO Container

### 3.1 What is queue?

**queue is a container adapter that provides FIFO (First In First Out) access pattern.**

Header: `#include <queue>`

**Why queue Exists:**

Many real-world scenarios follow FIFO - the first person in line is served first. Examples: task scheduling, breadth-first search, printer job queue, customer service.

```
FIFO Visualization:

Enqueue 1:  [1]              Dequeue: [1] → returns 1, queue becomes []
Enqueue 2:  [1][2]           Dequeue: [1][2] → returns 1, queue becomes [2]
Enqueue 3:  [1][2][3]        Dequeue: [1][2][3] → returns 1, queue becomes [2][3]
            ↑FRONT ↑BACK               ↑Remove from front
```

### 3.2 Basic Operations

```cpp
#include <iostream>
#include <queue>
using namespace std;

int main() {
    // WHY: queue enforces FIFO - first in, first out
    queue<int> q;

    // Push - Add to back: O(1)
    q.push(10);
    q.push(20);
    q.push(30);

    cout << "Queue size: " << q.size() << endl;
    cout << "Front: " << q.front() << endl;
    cout << "Back: " << q.back() << endl;

    // Pop - Remove from front: O(1)
    q.pop();  // Removes 10
    cout << "After pop, front: " << q.front() << endl;

    // Process entire queue
    cout << "Queue contents: ";
    while(!q.empty()) {
        cout << q.front() << " ";
        q.pop();
    }

    return 0;
}
```

**Output:**

```
Queue size: 3
Front: 10
Back: 30
After pop, front: 20
Queue contents: 20 30
```

### 3.3 queue Operations Summary

| Operation | Time | Description |
| --- | --- | --- |
| `push(x)` | O(1) | Add element to back |
| `pop()` | O(1) | Remove front element |
| `front()` | O(1) | Access front element |
| `back()` | O(1) | Access back element |
| `size()` | O(1) | Number of elements |
| `empty()` | O(1) | Check if empty |

### 3.4 Real-World Use Case: Task Scheduler

```cpp
#include <iostream>
#include <queue>
#include <string>
using namespace std;

struct Task {
    string name;
    int priority;  // Not used in basic queue
};

class TaskScheduler {
    queue<Task> tasks;

public:
    // WHY: FIFO ensures fair scheduling - first come, first served
    void addTask(string name) {
        tasks.push({name, 0});
        cout << "Added task: " << name << endl;
    }

    void processTasks() {
        cout << "\nProcessing tasks in order:\n";
        while(!tasks.empty()) {
            Task t = tasks.front();
            cout << "Processing: " << t.name << endl;
            tasks.pop();
        }
    }
};

int main() {
    TaskScheduler scheduler;

    scheduler.addTask("Send Email");
    scheduler.addTask("Write Report");
    scheduler.addTask("Review Code");

    scheduler.processTasks();

    return 0;
}
```

**Output:**

```
Added task: Send Email
Added task: Write Report
Added task: Review Code

Processing tasks in order:
Processing: Send Email
Processing: Write Report
Processing: Review Code
```

---

## 4. priority_queue - Heap Container

### 4.1 What is priority_queue?

**priority_queue is a container adapter that provides access to the largest (or smallest) element in O(1) time.**

Header: `#include <queue>`

**Why priority_queue Exists:**

Sometimes order matters more than arrival time - you want to process the most important task first, not the oldest. priority_queue implements a max-heap by default, keeping the largest element at the top.

```
Max-Heap Visualization:

         50
        /  \
       30   20
      / \
     10  5

Top is always largest: 50
After pop: Next largest becomes top
```

### 4.2 Basic Operations

```cpp
#include <iostream>
#include <queue>
using namespace std;

int main() {
    // WHY: Automatically maintains largest element at top
    priority_queue<int> pq;

    // Push - Add element: O(log n)
    pq.push(30);
    pq.push(10);
    pq.push(50);
    pq.push(20);

    cout << "Priority queue size: " << pq.size() << endl;
    cout << "Top (largest): " << pq.top() << endl;

    // Pop - Remove largest: O(log n)
    cout << "Elements in priority order: ";
    while(!pq.empty()) {
        cout << pq.top() << " ";
        pq.pop();
    }

    return 0;
}
```

**Output:**

```
Priority queue size: 4
Top (largest): 50
Elements in priority order: 50 30 20 10
```

### 4.3 priority_queue Operations Summary

| Operation | Time | Description |
| --- | --- | --- |
| `push(x)` | O(log n) | Add element, maintain heap |
| `pop()` | O(log n) | Remove top element |
| `top()` | O(1) | Access top (largest) |
| `size()` | O(1) | Number of elements |
| `empty()` | O(1) | Check if empty |

### 4.4 Min-Heap (Smallest Element First)

```cpp
#include <iostream>
#include <queue>
#include <vector>
using namespace std;

int main() {
    // WHY: Sometimes need smallest element first

    // Max-heap (default) - largest first
    priority_queue<int> maxHeap;
    maxHeap.push(30);
    maxHeap.push(10);
    maxHeap.push(50);
    cout << "Max-heap top: " << maxHeap.top() << endl;  // 50

    // Min-heap - smallest first
    priority_queue<int, vector<int>, greater<int>> minHeap;
    minHeap.push(30);
    minHeap.push(10);
    minHeap.push(50);
    cout << "Min-heap top: " << minHeap.top() << endl;  // 10

    return 0;
}
```

**Output:**

```
Max-heap top: 50
Min-heap top: 10
```

### 4.5 Custom Comparator

```cpp
#include <iostream>
#include <queue>
#include <string>
using namespace std;

struct Task {
    string name;
    int priority;

    // WHY: Define how to compare tasks
    bool operator<(const Task& other) const {
        return priority < other.priority;  // Higher priority = larger
    }
};

int main() {
    priority_queue<Task> pq;

    // WHY: Tasks processed by priority, not insertion order
    pq.push({"Email", 2});
    pq.push({"Bug Fix", 5});
    pq.push({"Meeting", 3});
    pq.push({"Coffee", 1});

    cout << "Tasks by priority:\n";
    while(!pq.empty()) {
        Task t = pq.top();
        cout << t.name << " (priority " << t.priority << ")\n";
        pq.pop();
    }

    return 0;
}
```

**Output:**

```
Tasks by priority:
Bug Fix (priority 5)
Meeting (priority 3)
Email (priority 2)
Coffee (priority 1)
```

### 4.6 Real-World Use Case: Event Simulation

```cpp
#include <iostream>
#include <queue>
#include <string>
using namespace std;

struct Event {
    int time;
    string description;

    // WHY: Process events in time order
    bool operator>(const Event& other) const {
        return time > other.time;  // Earlier time = higher priority
    }
};

int main() {
    // WHY: Min-heap to process earliest events first
    priority_queue<Event, vector<Event>, greater<Event>> events;

    events.push({5, "Meeting ends"});
    events.push({1, "Alarm rings"});
    events.push({3, "Email arrives"});
    events.push({2, "Coffee ready"});

    cout << "Event timeline:\n";
    while(!events.empty()) {
        Event e = events.top();
        cout << "Time " << e.time << ": " << e.description << endl;
        events.pop();
    }

    return 0;
}
```

**Output:**

```
Event timeline:
Time 1: Alarm rings
Time 2: Coffee ready
Time 3: Email arrives
Time 5: Meeting ends
```

---

## 5. Associative Containers - set and map

### 5.1 What are Associative Containers?

**Associative containers store elements in sorted order using a balanced binary search tree (Red-Black Tree), enabling O(log n) search, insert, and delete.**

**Why They Exist:**

Sequence containers require O(n) search. Associative containers provide O(log n) search by maintaining sorted order automatically.

```
Sequence (vector):        Associative (set):
[5, 2, 8, 1, 9]          Internally maintained as:
Find 8? O(n)                      5
                                /   \
                               2     8
                              /     / \
                             1     6   9
                          Find 8? O(log n)
```

### 5.2 set - Unique Sorted Elements

**set stores unique elements in sorted order.**

Header: `#include <set>`

```cpp
#include <iostream>
#include <set>
using namespace std;

int main() {
    // WHY: Automatic sorting and uniqueness
    set<int> s;

    // Insert - O(log n)
    s.insert(30);
    s.insert(10);
    s.insert(50);
    s.insert(20);
    s.insert(30);  // Duplicate - ignored!

    cout << "Set size: " << s.size() << endl;  // 4, not 5

    // Elements automatically sorted
    cout << "Set elements: ";
    for(int x : s) cout << x << " ";
    cout << endl;

    // Find - O(log n)
    if(s.find(20) != s.end()) {
        cout << "20 found" << endl;
    }

    // Count - O(log n)
    cout << "Count of 30: " << s.count(30) << endl;  // 1 (exists)
    cout << "Count of 99: " << s.count(99) << endl;  // 0 (doesn't exist)

    // Erase - O(log n)
    s.erase(30);
    cout << "After erase: ";
    for(int x : s) cout << x << " ";

    return 0;
}
```

**Output:**

```
Set size: 4
Set elements: 10 20 30 50
20 found
Count of 30: 1
Count of 99: 0
After erase: 10 20 50
```

### 5.3 set Operations

| Operation | Time | Description |
| --- | --- | --- |
| `insert(x)` | O(log n) | Add element (no duplicates) |
| `erase(x)` | O(log n) | Remove element |
| `find(x)` | O(log n) | Search for element |
| `count(x)` | O(log n) | 0 or 1 (exists or not) |
| `lower_bound(x)` | O(log n) | First element ≥ x |
| `upper_bound(x)` | O(log n) | First element > x |
| `size()` | O(1) | Number of elements |

### 5.4 multiset - Allows Duplicates

```cpp
#include <iostream>
#include <set>
using namespace std;

int main() {
    // WHY: When you need sorted elements but allow duplicates
    multiset<int> ms;

    ms.insert(30);
    ms.insert(10);
    ms.insert(30);  // Allowed!
    ms.insert(20);
    ms.insert(30);  // Allowed!

    cout << "Multiset size: " << ms.size() << endl;  // 5
    cout << "Count of 30: " << ms.count(30) << endl;  // 3

    cout << "Elements: ";
    for(int x : ms) cout << x << " ";

    return 0;
}
```

**Output:**

```
Multiset size: 5
Count of 30: 3
Elements: 10 20 30 30 30
```

### 5.5 map - Key-Value Pairs

**map stores unique keys with associated values, sorted by key.**

Header: `#include <map>`

```cpp
#include <iostream>
#include <map>
#include <string>
using namespace std;

int main() {
    // WHY: Fast lookup of values by key
    map<string, int> ages;

    // Insert - O(log n)
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 28;
    ages.insert({"David", 35});
    ages.insert(make_pair("Eve", 27));

    cout << "Map size: " << ages.size() << endl;

    // Access - O(log n)
    cout << "Alice's age: " << ages["Alice"] << endl;

    // operator[] creates element if not exists!
    cout << "Frank's age: " << ages["Frank"] << endl;  // Creates Frank with value 0
    cout << "After accessing Frank, size: " << ages.size() << endl;

    // at() throws exception if not exists (safer)
    try {
        cout << ages.at("George") << endl;
    } catch(const out_of_range& e) {
        cout << "George not found" << endl;
    }

    // Iterate - sorted by key
    cout << "\nAll entries (sorted by name):\n";
    for(auto& pair : ages) {
        cout << pair.first << ": " << pair.second << endl;
    }

    // Find - O(log n)
    if(ages.find("Bob") != ages.end()) {
        cout << "\nBob found" << endl;
    }

    return 0;
}
```

**Output:**

```
Map size: 5
Alice's age: 25
Frank's age: 0
After accessing Frank, size: 6
George not found

All entries (sorted by name):
Alice: 25
Bob: 30
Charlie: 28
David: 35
Eve: 27
Frank: 0

Bob found
```

### 5.6 map Operations

| Operation | Time | Description |
| --- | --- | --- |
| `insert({k,v})` | O(log n) | Add key-value pair |
| `erase(k)` | O(log n) | Remove by key |
| `find(k)` | O(log n) | Search for key |
| `[k]` | O(log n) | Access/insert (creates if missing!) |
| `at(k)` | O(log n) | Access (throws if missing) |
| `count(k)` | O(log n) | 0 or 1 |
| `size()` | O(1) | Number of pairs |

### 5.7 multimap - Multiple Values per Key

```cpp
#include <iostream>
#include <map>
#include <string>
using namespace std;

int main() {
    // WHY: When one key can have multiple values
    multimap<string, int> scores;

    scores.insert({"Alice", 90});
    scores.insert({"Alice", 85});  // Same key, different value - OK!
    scores.insert({"Bob", 95});
    scores.insert({"Alice", 88});  // Another value for Alice

    cout << "Total entries: " << scores.size() << endl;
    cout << "Alice's count: " << scores.count("Alice") << endl;

    // Get all values for a key
    cout << "\nAlice's scores: ";
    auto range = scores.equal_range("Alice");
    for(auto it = range.first; it != range.second; ++it) {
        cout << it->second << " ";
    }

    return 0;
}
```

**Output:**

```
Total entries: 4
Alice's count: 3

Alice's scores: 85 88 90
```

### 5.8 Real-World Use Cases

**Use Case 1: Word Frequency Counter**

```cpp
#include <iostream>
#include <map>
#include <string>
#include <sstream>
using namespace std;

int main() {
    string text = "the quick brown fox jumps over the lazy dog the fox";

    // WHY: map automatically maintains sorted word counts
    map<string, int> wordCount;

    istringstream iss(text);
    string word;
    while(iss >> word) {
        wordCount[word]++;  // Creates key if not exists, increments
    }

    cout << "Word frequencies (alphabetically):\n";
    for(auto& pair : wordCount) {
        cout << pair.first << ": " << pair.second << endl;
    }

    return 0;
}
```

**Output:**

```
Word frequencies (alphabetically):
brown: 1
dog: 1
fox: 2
jumps: 1
lazy: 1
over: 1
quick: 1
the: 3
```

**Use Case 2: Phone Directory**

```cpp
#include <iostream>
#include <map>
#include <string>
using namespace std;

int main() {
    // WHY: Fast lookup by name, automatically sorted
    map<string, string> phoneBook;

    phoneBook["Alice"] = "555-1234";
    phoneBook["Bob"] = "555-5678";
    phoneBook["Charlie"] = "555-9012";

    string name;
    cout << "Enter name: ";
    cin >> name;

    auto it = phoneBook.find(name);
    if(it != phoneBook.end()) {
        cout << name << "'s number: " << it->second << endl;
    } else {
        cout << name << " not found" << endl;
    }

    return 0;
}
```

### 5.9 set/map vs unordered_set/unordered_map

| Feature | set/map | unordered_set/unordered_map |
| --- | --- | --- |
| Implementation | Red-Black Tree | Hash Table |
| Order | Sorted | No order |
| Insert/Find/Erase | O(log n) | O(1) average |
| Iteration | Sorted order | Random order |
| Use when | Need sorted order | Just need fast lookup |

---

## Summary

### Key Takeaways

1. **Container adapters restrict interfaces to enforce specific patterns** - stack enforces LIFO (last in first out), queue enforces FIFO (first in first out), and priority_queue maintains largest element at top. This prevents accidental misuse and makes code intention clear.
2. **stack is perfect for LIFO algorithms** like balanced parentheses checking, undo/redo functionality, depth-first search, and expression evaluation. Real-world function call stack is implemented as a stack data structure.
3. **queue implements fair scheduling with FIFO** - first task added is first task processed. Use for task queues, breadth-first search, printer job queues, and any scenario requiring fair ordering.
4. **priority_queue uses heap structure for O(log n) operations** maintaining largest (or smallest with custom comparator) element at top. Critical for Dijkstra's algorithm, event-driven simulation, and any scenario where importance matters more than insertion order.
5. **set automatically maintains unique sorted elements** with O(log n) insert, find, and erase using Red-Black Tree. Duplicates are silently ignored. Use multiset when duplicates needed but sorted order still required.
6. **map associates unique keys with values, sorted by key** enabling O(log n) lookups. Critical difference: operator[] creates missing keys (dangerous), while at() throws exception (safer). Always check with find() or count() before using operator[].
7. **Red-Black Trees provide guaranteed O(log n) operations** unlike hash tables which have O(1) average but O(n) worst case. Use set/map when you need guaranteed performance or sorted iteration order.
8. **multiset and multimap allow duplicate keys** while maintaining sorted order. multimap's equal_range() efficiently retrieves all values for a key in O(log n + k) where k is number of values.
9. **Container adapters default to sensible underlying containers** - stack and queue use deque (efficient at both ends), priority_queue uses vector (efficient heap operations). Can specify different containers if needed.
10. **Choose associative over sequence when search frequency matters** - if you're searching frequently, O(log n) with set/map beats O(n) with vector. But if mostly iterating, vector's cache performance wins despite worse search complexity.

---

## Interview Preparation

### Q1: Explain the difference between stack, queue, and priority_queue. Give a real-world use case for each.

**Answer:**

stack, queue, and priority_queue are container adapters that provide restricted interfaces for specific access patterns.

stack implements LIFO (Last In First Out) - the most recently added element is removed first. It provides push() to add to top, pop() to remove from top, and top() to view the top element, all in O(1) time. Internally uses deque by default.

Real-world use case: Function call stack. When function A calls function B which calls function C, the calls are stacked. When C returns, control goes back to B (most recent caller). When B returns, control goes back to A. The undo functionality in text editors also uses a stack - most recent action is undone first.

queue implements FIFO (First In First Out) - the first element added is removed first. It provides push() to add to back, pop() to remove from front, front() and back() to access ends, all in O(1). Uses deque by default.

Real-world use case: Task scheduling system. Tasks are queued in order received and processed first-come-first-served. A printer queue processes jobs in the order they were submitted, ensuring fairness.

priority_queue maintains elements in a max-heap (largest element at top by default). It provides push() in O(log n) to add elements, pop() in O(log n) to remove the largest, and top() in O(1) to view the largest. Uses vector by default implementing binary heap.

Real-world use case: Hospital emergency room. Patients are prioritized by severity, not arrival time. A patient with critical condition is treated before a patient with minor injury, even if the latter arrived first. Dijkstra's shortest path algorithm uses priority_queue to always process the closest unvisited node next.

Key insight: These adapters don't implement storage - they wrap other containers and restrict the interface to enforce the pattern. This prevents bugs from accidentally accessing elements in ways that violate the intended data structure semantics.

---

### Q2: Compare set and map. When would you use each? What's the difference between operator[] and at() for map?

**Answer:**

set and map are both associative containers using Red-Black Tree for O(log n) operations, but serve different purposes.

set stores unique elements in sorted order. It's like a mathematical set - checking membership, adding elements, removing elements. The elements themselves are the keys. Operations: insert(), find(), erase(), count() all in O(log n). Iteration yields elements in sorted order.

map stores key-value pairs where keys are unique and sorted. It associates data with keys for fast lookup. The key is used for ordering and searching, the value is arbitrary data associated with that key. Operations: insert({k,v}), find(k), erase(k), operator[k], at(k) all in O(log n).

Use set when you only care about presence or absence of values and want automatic sorting and uniqueness. Examples: storing unique user IDs, maintaining a sorted list of active connections, or checking if a word exists in a dictionary.

Use map when you need to associate data with keys for fast retrieval. Examples: storing user profiles by username, caching computed results by input parameters, or maintaining configuration settings by name.

operator[] vs at() for map - critical difference:

operator[] returns reference to value associated with key. If key doesn't exist, it creates a new entry with default-constructed value and returns reference to it. This is convenient but dangerous:

```cpp
map<string, int> m;
int age = m["Alice"];  // Creates Alice with value 0 if missing!
```

at() returns reference to value if key exists, otherwise throws out_of_range exception. It never creates entries:

```cpp
try {
    int age = m.at("Alice");  // Throws if Alice doesn't exist
} catch(out_of_range& e) {
    // Handle missing key
}
```

Best practice: Use find() or count() to check existence first, or use at() if you can handle exceptions. Only use operator[] when you intentionally want to create missing keys (like counting word frequency) or when you're certain the key exists.

---

### Q3: How does priority_queue work internally? How would you implement a min-heap? Explain with custom comparator for a struct.

**Answer:**

priority_queue implements a binary max-heap using a vector by default. A binary heap is a complete binary tree stored in an array where parent nodes are larger than (or smaller than, for min-heap) their children.

Internal representation uses vector with heap property:

- Element at index i has children at indices 2*i+1 and 2*i+2
- Element at index i has parent at index (i-1)/2
- Max-heap property: parent ≥ children at every node

Operations:

- push(x): Add x at end (O(1)), then bubble up to maintain heap property (O(log n))
- pop(): Swap root with last element, remove last, bubble down new root (O(log n))
- top(): Return first element (O(1))

To implement min-heap (smallest element at top), use greater<int> comparator:

```cpp
// Max-heap (default) - largest at top
priority_queue<int> maxHeap;

// Min-heap - smallest at top
priority_queue<int, vector<int>, greater<int>> minHeap;
```

Custom comparator for struct:

```cpp
struct Task {
    string name;
    int priority;
    int arrival_time;

    // WHY: Define comparison for priority_queue
    // Returns true if this < other (lower priority)
    bool operator<(const Task& other) const {
        // Higher priority number = higher importance
        if(priority != other.priority)
            return priority < other.priority;
        // If same priority, earlier arrival first
        return arrival_time > other.arrival_time;
    }
};

priority_queue<Task> pq;
pq.push({"Email", 2, 100});
pq.push({"Bug", 5, 105});
pq.push({"Meeting", 2, 95});

// Order: Bug(5,105), Meeting(2,95), Email(2,100)
```

Alternative using lambda or function object:

```cpp
auto cmp = [](const Task& a, const Task& b) {
    if(a.priority != b.priority)
        return a.priority < b.priority;
    return a.arrival_time > b.arrival_time;
};

priority_queue<Task, vector<Task>, decltype(cmp)> pq(cmp);
```

Why heap structure: O(log n) insertions and deletions are much better than keeping a sorted vector (O(n) insertions) or unsorted vector (O(n) to find max). Heap provides perfect balance for priority queue operations.

---

### Q4: When would you use multiset/multimap instead of set/map? Give a practical example where duplicates are necessary.

**Answer:**

Use multiset/multimap when you need to allow duplicate keys but still want sorted order and O(log n) operations. Regular set/map silently ignore duplicate keys.

multiset allows duplicate elements while maintaining sorted order. All operations same as set but count() can return > 1.

Practical example: Leaderboard/scoreboard. Multiple players can have the same score, and you want scores sorted. Using set would lose players with duplicate scores:

```cpp
// BAD: Using set loses duplicate scores
set<int> scores;
scores.insert(100);
scores.insert(95);
scores.insert(100);  // Ignored! Only one 100
// Result: {95, 100} - lost one player with score 100

// GOOD: Using multiset keeps all scores
multiset<int, greater<int>> scores;  // Descending order
scores.insert(100);
scores.insert(95);
scores.insert(100);  // Kept!
// Result: {100, 100, 95} - preserved both players with 100
```

multimap allows multiple values per key while keeping keys sorted. Useful when natural key has multiple associated values.

Practical example: Student course enrollment. One student (key) can be enrolled in multiple courses (values):

```cpp
multimap<string, string> enrollment;
enrollment.insert({"Alice", "CS101"});
enrollment.insert({"Alice", "MATH201"});
enrollment.insert({"Alice", "PHYS101"});
enrollment.insert({"Bob", "CS101"});

// Get all courses for Alice
cout << "Alice's courses: ";
auto range = enrollment.equal_range("Alice");
for(auto it = range.first; it != range.second; ++it) {
    cout << it->second << " ";
}
// Output: CS101 MATH201 PHYS101
```

Another example: Event timeline. Multiple events can occur at the same timestamp:

```cpp
multimap<int, string> timeline;
timeline.insert({10, "Meeting starts"});
timeline.insert({10, "Email arrives"});  // Same time, both kept
timeline.insert({15, "Phone call"});
```

Key operations:

- count(key) returns number of elements with that key
- equal_range(key) returns pair of iterators to all elements with that key
- find(key) returns iterator to first element with that key

When to still use set/map: When duplicates are errors or meaningless (user IDs, unique product codes) or when you only care about existence (visited nodes in graph traversal).

---

### Q5: Compare set/map with unordered_set/unordered_map. Which would you choose for a word frequency counter and why?

**Answer:**

set/map and unordered_set/unordered_map both provide fast lookups but use different implementations with different trade-offs.

set/map use Red-Black Tree (balanced BST):

- Implementation: Self-balancing binary search tree
- Time complexity: O(log n) for insert, find, erase - guaranteed
- Iteration order: Sorted by key
- Memory: Higher overhead per element (color bit + pointers)
- Predictability: Consistent performance, no worst case degradation

unordered_set/unordered_map use hash tables:

- Implementation: Hash table with chaining for collisions
- Time complexity: O(1) average for insert, find, erase; O(n) worst case
- Iteration order: No guaranteed order (hash order)
- Memory: Bucket array + element storage
- Predictability: Usually faster but can degrade with poor hash function or many collisions

For word frequency counter specifically, I would choose unordered_map:

```cpp
unordered_map<string, int> wordCount;
string word;
while(cin >> word) {
    wordCount[word]++;  // O(1) average vs O(log n)
}
```

Reasoning: Word frequency counting has two phases - counting (lots of insertions/updates) and reporting. During counting, unordered_map's O(1) average operations are faster than map's O(log n), especially with large vocabularies. We don't need sorted order during counting.

For reporting, if sorted output is needed:

```cpp
// After counting with unordered_map, sort if needed
vector<pair<string,int>> sorted(wordCount.begin(), wordCount.end());
sort(sorted.begin(), sorted.end());
```

This is still faster than using map throughout because sorting n elements once (O(n log n)) is typically faster than n insertions at O(log n) each, especially when n is large and many words are repeats (fewer unique words than total words).

However, use map instead of unordered_map when:

- Need results in sorted order as you go (real-time sorted display)
- Hash function is poor for your key type (causes many collisions)
- Worst-case performance guarantees are critical (real-time systems)
- Memory is extremely constrained (unordered_map has higher overhead for buckets)

Benchmark example: Counting 1 million words with 100k unique words:

- unordered_map: ~100ms (mostly O(1) lookups)
- map: ~300ms (all O(log n) = log(100k) ≈ 17 comparisons per lookup)

The performance gap widens with larger datasets, making unordered_map the clear choice for pure lookup performance when sorted order isn't required.