# 2.5. Recursion

---

## Table of Contents

1. Recursion Fundamentals
2. Types of Recursion
3. Tail Recursion & Optimization
4. Recursion vs Iteration
5. Common Recursive Problems

---

## 1. Recursion Fundamentals

### 1.1 What is Recursion?

**Recursion:** Function that calls itself to solve smaller instances of the same problem.

**Two Essential Components:**

1. **Base Case** - Stopping condition
2. **Recursive Case** - Self-call with simpler problem

```cpp
#include <iostream>
using namespace std;

// WHY: Classic recursive example
int factorial(int n) {
    // Base case: stops recursion
    if (n <= 1) {
        return 1;
    }

    // Recursive case: reduces problem size
    return n * factorial(n - 1);
}

int main() {
    cout << factorial(5) << endl;  // 120
    return 0;
}
```

**How it executes:**

```
factorial(5)
├─ 5 * factorial(4)
│      ├─ 4 * factorial(3)
│      │      ├─ 3 * factorial(2)
│      │      │      ├─ 2 * factorial(1)
│      │      │      │      └─ returns 1
│      │      │      └─ returns 2 * 1 = 2
│      │      └─ returns 3 * 2 = 6
│      └─ returns 4 * 6 = 24
└─ returns 5 * 24 = 120
```

### 1.2 Stack Memory in Recursion

**Call Stack Visualization:**

```
Initial:                     factorial(3) called:
┌──────────┐               ┌──────────────┐
│  main()  │               │factorial(3)  │ ← Stack grows
└──────────┘               │  n = 3       │
                           ├──────────────┤
                           │  main()      │
                           └──────────────┘

factorial(2) called:         factorial(1) called:
┌──────────────┐           ┌──────────────┐
│factorial(2)  │           │factorial(1)  │ ← Maximum depth
│  n = 2       │           │  n = 1       │
├──────────────┤           ├──────────────┤
│factorial(3)  │           │factorial(2)  │
│  n = 3       │           │  n = 2       │
├──────────────┤           ├──────────────┤
│  main()      │           │factorial(3)  │
└──────────────┘           │  n = 3       │
                           ├──────────────┤
                           │  main()      │
                           └──────────────┘

Returns propagate back:
factorial(1) returns 1
factorial(2) returns 2
factorial(3) returns 6
```

### 1.3 Stack Frame Contents

```cpp
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}
```

**Each stack frame contains:**

- Function parameters (`n`)
- Local variables (if any)
- Return address
- Saved registers

**Memory usage:** O(n) for linear recursion, O(n) depth

---

## 2. Types of Recursion

### 2.1 Direct Recursion

**Definition:** Function calls itself directly.

```cpp
// WHY: Function calls itself
int power(int base, int exp) {
    if (exp == 0) return 1;
    return base * power(base, exp - 1);  // Direct self-call
}
```

### 2.2 Indirect Recursion

**Definition:** Function calls another function which eventually calls the first function.

```cpp
#include <iostream>
using namespace std;

// Forward declaration
void functionB(int n);

// WHY: functionA calls functionB
void functionA(int n) {
    if (n > 0) {
        cout << "A: " << n << " ";
        functionB(n - 1);  // Calls B
    }
}

// WHY: functionB calls functionA
void functionB(int n) {
    if (n > 1) {
        cout << "B: " << n << " ";
        functionA(n / 2);  // Calls A - indirect recursion!
    }
}

int main() {
    functionA(10);
    return 0;
}
```

**Output:**

```
A: 10 B: 9 A: 4 B: 3 A: 1
```

### 2.3 Mutual Recursion

**Definition:** Two or more functions call each other.

```cpp
#include <iostream>
using namespace std;

// Forward declarations
bool isEven(int n);
bool isOdd(int n);

// WHY: Mutual recursion - each calls the other
bool isEven(int n) {
    if (n == 0) return true;
    return isOdd(n - 1);  // Calls isOdd
}

bool isOdd(int n) {
    if (n == 0) return false;
    return isEven(n - 1);  // Calls isEven
}

int main() {
    cout << boolalpha;
    cout << "Is 10 even? " << isEven(10) << endl;
    cout << "Is 7 odd? " << isOdd(7) << endl;
    return 0;
}
```

**Output:**

```
Is 10 even? true
Is 7 odd? true
```

**Call sequence for `isEven(4)`:**

```
isEven(4) → isOdd(3) → isEven(2) → isOdd(1) → isEven(0) → true
```

---

## 3. Tail Recursion & Optimization

### 3.1 What is Tail Recursion?

**Tail recursion:** Recursive call is the LAST operation in the function (nothing after it).

**Non-tail recursive:**

```cpp
// WHY: Multiplication happens AFTER recursive call returns
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);  // Operation AFTER call
}
```

**Tail recursive:**

```cpp
// WHY: Nothing happens after recursive call
int factorialTail(int n, int accumulator = 1) {
    if (n <= 1) return accumulator;
    return factorialTail(n - 1, n * accumulator);  // LAST operation
}
```

### 3.2 Tail Call Optimization (TCO)

**What is TCO?**
Compiler optimization that converts tail-recursive functions into loops.

**How it works:**

```
Normal Recursion:           Tail Call Optimized:
┌──────────────┐           ┌──────────────┐
│ Push frame   │           │ Reuse frame  │
│ Call self    │    →      │ Update args  │
│ Pop frame    │           │ Jump to top  │
│ Return       │           └──────────────┘
└──────────────┘           (No new frames!)
```

**Comparison:**

```cpp
#include <iostream>
using namespace std;

// Non-tail: Stack grows
int factorialNormal(int n) {
    if (n <= 1) return 1;
    return n * factorialNormal(n - 1);  // Waits for result
}

// Tail recursive: Can be optimized
int factorialTail(int n, int acc = 1) {
    if (n <= 1) return acc;
    return factorialTail(n - 1, n * acc);  // Returns immediately
}

int main() {
    // factorialNormal(100000);  // Stack overflow!
    cout << factorialTail(10) << endl;  // OK - optimized to loop
    return 0;
}
```

**Compiler transformation (with -O2):**

```cpp
// Tail recursive version
int factorialTail(int n, int acc) {
    if (n <= 1) return acc;
    return factorialTail(n - 1, n * acc);
}

// Compiler converts to (approximately):
int factorialTail(int n, int acc) {
    while (n > 1) {
        acc = n * acc;
        n = n - 1;
    }
    return acc;
}
```

### 3.3 TCO Requirements

**✅ Can be optimized:**

```cpp
// 1. Pure tail call
int sum(int n, int acc = 0) {
    if (n == 0) return acc;
    return sum(n - 1, n + acc);  // Last operation
}

// 2. Conditional tail calls
int gcd(int a, int b) {
    if (b == 0) return a;
    return gcd(b, a % b);  // Tail call in both branches
}
```

**❌ Cannot be optimized:**

```cpp
// 1. Operation after call
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);  // Multiply AFTER return
}

// 2. Multiple calls
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);  // Two calls + addition
}

// 3. Destructors after call (C++-specific)
int func(int n) {
    string temp = "data";
    if (n == 0) return 0;
    return func(n - 1);  // temp destroyed AFTER call
}
```

### 3.4 Compiler Support

**GCC/Clang:**

```bash
# -O0: No optimization
g++ -O0 code.cpp  # Tail calls NOT optimized

# -O2/-O3: Tail call optimization enabled
g++ -O2 code.cpp  # Tail calls optimized

# Specific flag
g++ -foptimize-sibling-calls code.cpp
```

**Checking if optimized:**

```cpp
#include <iostream>
using namespace std;

int tailRecursive(int n, int acc = 0) {
    if (n == 0) return acc;
    return tailRecursive(n - 1, n + acc);
}

int main() {
    // If optimized: runs fine
    // If not optimized: stack overflow
    cout << tailRecursive(1000000) << endl;
    return 0;
}
```

**Compile and test:**

```bash
# Without optimization - crashes
g++ -O0 test.cpp && ./a.out  # Segmentation fault

# With optimization - works
g++ -O2 test.cpp && ./a.out  # Outputs result
```

### 3.5 Manual Tail Recursion to Loop

```cpp
// Tail recursive version
int sum(int n, int acc = 0) {
    if (n == 0) return acc;
    return sum(n - 1, n + acc);
}

// Manual conversion to loop (what compiler does)
int sumIterative(int n) {
    int acc = 0;
    while (n > 0) {
        acc = n + acc;
        n = n - 1;
    }
    return acc;
}
```

---

## 4. Recursion vs Iteration

### 4.1 Detailed Comparison

| Aspect | Recursion | Iteration |
| --- | --- | --- |
| **Stack Usage** | O(n) - each call adds frame | O(1) - constant space |
| **Performance** | Slower - function call overhead | Faster - direct jumps |
| **Code Clarity** | Often cleaner for tree/graph problems | Can be more verbose |
| **Memory** | Limited by stack size (~1-8MB) | Limited by available RAM |
| **Debugging** | Harder - deep call stacks | Easier - linear flow |
| **Compiler** | May optimize (TCO) | Already optimal |

### 4.2 Performance Comparison

```cpp
#include <iostream>
#include <chrono>
using namespace std;

// Recursive fibonacci - exponential time
int fibRecursive(int n) {
    if (n <= 1) return n;
    return fibRecursive(n - 1) + fibRecursive(n - 2);
}

// Iterative fibonacci - linear time
int fibIterative(int n) {
    if (n <= 1) return n;
    int prev = 0, curr = 1;
    for (int i = 2; i <= n; i++) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    int result1 = fibRecursive(40);
    auto end = chrono::high_resolution_clock::now();
    auto duration1 = chrono::duration_cast<chrono::milliseconds>(end - start);

    start = chrono::high_resolution_clock::now();
    int result2 = fibIterative(40);
    end = chrono::high_resolution_clock::now();
    auto duration2 = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Recursive: " << duration1.count() << "ms" << endl;
    cout << "Iterative: " << duration2.count() << "ms" << endl;
    cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x" << endl;

    return 0;
}
```

**Typical Output:**

```
Recursive: 842ms
Iterative: 0ms
Speedup: ∞x  (too fast to measure!)
```

### 4.3 Stack Usage Comparison

```cpp
#include <iostream>
using namespace std;

// Recursive - uses stack
int sumRecursive(int n) {
    if (n == 0) return 0;
    return n + sumRecursive(n - 1);
}

// Iterative - constant stack
int sumIterative(int n) {
    int total = 0;
    for (int i = 1; i <= n; i++) {
        total += i;
    }
    return total;
}

int main() {
    // sumRecursive(1000000);  // Stack overflow!
    cout << sumIterative(1000000) << endl;  // Works fine
    return 0;
}
```

### 4.4 When to Use Each

**Use Recursion:**

```cpp
// ✅ Tree traversal - naturally recursive
struct Node {
    int data;
    Node *left, *right;
};

void inorder(Node* root) {
    if (root == nullptr) return;
    inorder(root->left);
    cout << root->data << " ";
    inorder(root->right);
}

// ✅ Divide and conquer algorithms
int binarySearch(int arr[], int left, int right, int target) {
    if (left > right) return -1;
    int mid = left + (right - left) / 2;
    if (arr[mid] == target) return mid;
    if (arr[mid] > target)
        return binarySearch(arr, left, mid - 1, target);
    return binarySearch(arr, mid + 1, right, target);
}
```

**Use Iteration:**

```cpp
// ✅ Simple sequences
int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// ✅ Performance-critical code
int fibonacci(int n) {
    if (n <= 1) return n;
    int prev = 0, curr = 1;
    for (int i = 2; i <= n; i++) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}
```

---

## 5. Common Recursive Problems

### 5.1 Factorial

```cpp
#include <iostream>
using namespace std;

// Non-tail recursive
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Tail recursive (optimizable)
int factorialTail(int n, int acc = 1) {
    if (n <= 1) return acc;
    return factorialTail(n - 1, n * acc);
}

// Iterative
int factorialIter(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int main() {
    cout << "5! = " << factorial(5) << endl;
    cout << "5! = " << factorialTail(5) << endl;
    cout << "5! = " << factorialIter(5) << endl;
    return 0;
}
```

**Output:**

```
5! = 120
5! = 120
5! = 120
```

### 5.2 Fibonacci

```cpp
#include <iostream>
using namespace std;

// Naive recursive - O(2^n) time!
int fibRecursive(int n) {
    if (n <= 1) return n;
    return fibRecursive(n - 1) + fibRecursive(n - 2);
}

// Tail recursive with accumulator
int fibTail(int n, int a = 0, int b = 1) {
    if (n == 0) return a;
    if (n == 1) return b;
    return fibTail(n - 1, b, a + b);
}

// Iterative - O(n) time, O(1) space
int fibIterative(int n) {
    if (n <= 1) return n;
    int prev = 0, curr = 1;
    for (int i = 2; i <= n; i++) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}

int main() {
    for (int i = 0; i < 10; i++) {
        cout << fibIterative(i) << " ";
    }
    cout << endl;
    return 0;
}
```

**Output:**

```
0 1 1 2 3 5 8 13 21 34
```

### 5.3 Greatest Common Divisor (GCD)

```cpp
#include <iostream>
using namespace std;

// Euclidean algorithm - tail recursive
int gcd(int a, int b) {
    if (b == 0) return a;
    return gcd(b, a % b);  // Tail call - can be optimized
}

// Iterative version
int gcdIterative(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int main() {
    cout << "GCD(48, 18) = " << gcd(48, 18) << endl;
    cout << "GCD(100, 35) = " << gcdIterative(100, 35) << endl;
    return 0;
}
```

**Output:**

```
GCD(48, 18) = 6
GCD(100, 35) = 5
```

### 5.4 Power Function

```cpp
#include <iostream>
using namespace std;

// Simple recursive
int power(int base, int exp) {
    if (exp == 0) return 1;
    return base * power(base, exp - 1);
}

// Optimized - O(log n)
int powerFast(int base, int exp) {
    if (exp == 0) return 1;
    int half = powerFast(base, exp / 2);
    if (exp % 2 == 0)
        return half * half;
    else
        return base * half * half;
}

int main() {
    cout << "2^10 = " << power(2, 10) << endl;
    cout << "2^10 = " << powerFast(2, 10) << endl;
    return 0;
}
```

**Output:**

```
2^10 = 1024
2^10 = 1024
```

### 5.5 Sum of Digits

```cpp
#include <iostream>
using namespace std;

// Recursive
int sumOfDigits(int n) {
    if (n == 0) return 0;
    return (n % 10) + sumOfDigits(n / 10);
}

// Iterative
int sumOfDigitsIter(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

int main() {
    cout << "Sum of digits of 12345: " << sumOfDigits(12345) << endl;
    cout << "Sum of digits of 12345: " << sumOfDigitsIter(12345) << endl;
    return 0;
}
```

**Output:**

```
Sum of digits of 12345: 15
Sum of digits of 12345: 15
```

### 5.6 Binary Search

```cpp
#include <iostream>
using namespace std;

// Recursive binary search
int binarySearchRecursive(int arr[], int left, int right, int target) {
    if (left > right) return -1;

    int mid = left + (right - left) / 2;

    if (arr[mid] == target)
        return mid;

    if (arr[mid] > target)
        return binarySearchRecursive(arr, left, mid - 1, target);

    return binarySearchRecursive(arr, mid + 1, right, target);
}

// Iterative binary search
int binarySearchIterative(int arr[], int size, int target) {
    int left = 0, right = size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (arr[mid] == target)
            return mid;

        if (arr[mid] > target)
            right = mid - 1;
        else
            left = mid + 1;
    }

    return -1;
}

int main() {
    int arr[] = {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78};
    int size = sizeof(arr) / sizeof(arr[0]);

    cout << "Index of 23: " << binarySearchRecursive(arr, 0, size - 1, 23) << endl;
    cout << "Index of 100: " << binarySearchIterative(arr, size, 100) << endl;

    return 0;
}
```

**Output:**

```
Index of 23: 5
Index of 100: -1
```

---

## Summary

### Key Takeaways

1. **Recursion basics** - Function calls itself; needs base case and recursive case
2. **Stack memory** - Each call creates stack frame; O(n) space for depth n
3. **Types** - Direct (calls self), indirect (calls another that calls back), mutual (two+ call each other)
4. **Tail recursion** - Recursive call is last operation; enables optimization
5. **TCO** - Compiler converts tail recursion to loops; requires -O2 in GCC
6. **Recursion vs iteration** - Recursion: cleaner for trees/graphs but slower, uses more stack; Iteration: faster, constant space
7. **Performance** - Naive fibonacci O(2^n), iteration O(n); stack limits ~1M calls
8. **TCO requirements** - Pure tail call, no operations after, no destructors after call
9. **Common problems** - Factorial, fibonacci, GCD, power, tree traversal, binary search
10. **Best practices** - Use recursion for naturally recursive structures; convert to iteration for performance

### Interview Essential Points

**Q: What is recursion and what are its key components?**
A: Recursion is when a function calls itself to solve smaller instances of same problem. Two essential components: (1) Base case - stopping condition that prevents infinite recursion, (2) Recursive case - self-call with reduced problem size. Example: `factorial(n) = n * factorial(n-1)` with base `factorial(1) = 1`.

**Q: How does recursion use stack memory?**
A: Each recursive call creates new stack frame containing parameters, local variables, return address. Frames stack up until base case reached, then unwind as functions return. Space complexity O(n) for recursion depth n. Stack overflow occurs if depth exceeds stack limit (typically 1-8MB).

**Q: What is tail recursion and why is it important?**
A: Tail recursion is when recursive call is the last operation in function - nothing happens after it returns. Important because compilers can optimize it to loops (Tail Call Optimization), eliminating stack growth. Example: `fact(n, acc) = fact(n-1, n*acc)` is tail recursive, `fact(n) = n * fact(n-1)` is not.

**Q: Explain tail call optimization (TCO).**
A: TCO is compiler optimization that converts tail-recursive functions into loops. Instead of creating new stack frames, compiler reuses current frame by updating parameters and jumping to function start. Enabled with -O2 in GCC. Converts recursive calls to iterative loops at machine code level, preventing stack overflow.

**Q: When can TCO NOT be applied?**
A: TCO cannot be applied when: (1) Operation exists after recursive call (e.g., `n * fact(n-1)`), (2) Multiple recursive calls (e.g., fibonacci), (3) Destructors run after call (C++-specific), (4) Virtual function calls (runtime binding), (5) Low optimization levels (-O0).

**Q: Difference between direct and indirect recursion?**
A: Direct recursion: function calls itself directly (`A calls A`). Indirect recursion: function calls another function that eventually calls first function (`A calls B calls A`). Mutual recursion: two or more functions call each other (`A calls B, B calls A`). All create call stacks but indirect/mutual harder to optimize.

**Q: Recursion vs iteration - when to use each?**
A: Use recursion for: (1) Naturally recursive structures (trees, graphs), (2) Divide-and-conquer algorithms, (3) Backtracking problems, (4) When code clarity is priority. Use iteration for: (1) Simple sequences (factorial, fibonacci), (2) Performance-critical code, (3) Large input sizes (avoid stack overflow), (4) When space efficiency matters. Iteration is generally faster and uses constant stack space.

**Q: Why is naive recursive fibonacci so slow?**
A: Exponential time O(2^n) due to redundant calculations. `fib(5)` calls `fib(4)` and `fib(3)`, but `fib(4)` also calls `fib(3)` - massive duplication. Each call spawns two more calls creating binary tree of calls. Iterative version is O(n) time, O(1) space. Can optimize with memoization (dynamic programming) to O(n) time.

---