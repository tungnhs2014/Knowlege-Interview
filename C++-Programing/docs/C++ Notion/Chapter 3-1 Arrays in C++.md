# 3.1. Arrays in C++

---

## Table of Contents

1. What are Arrays?
2. Array Declaration and Initialization
3. Accessing and Modifying Array Elements
4. Array Traversal
5. Array Size and Memory
6. Multi-Dimensional Arrays
7. Arrays and Pointers
8. Array Decay
9. Passing Arrays to Functions
10. Character Arrays (C-style Strings)
11. std::array (C++11)
12. Common Mistakes and Best Practices

---

## 1. What are Arrays?

### 1.1 Definition

**An array is a collection of elements of the same data type stored in contiguous memory locations.** Arrays allow storing multiple values under a single identifier and accessing them using an index.

**Core Concept:**

```cpp
// WHY: Store multiple exam scores without 50 separate variables
int scores[50];  // Single array holds all scores
```

### 1.2 Purpose & Benefits

**Why Use Arrays?**

1. **Organized Data Storage**
    - Group related data of same type
    - Manage collections efficiently
2. **Memory Efficiency**
    - Contiguous allocation improves cache performance
    - Predictable memory layout
3. **Fast Random Access**
    - O(1) time complexity for element access
    - Direct access via index calculation
4. **Iteration Support**
    - Easy to loop through elements
    - Bulk operations on data

**Real-World Applications:**

- Storing sensor readings in embedded systems
- Game board representations (chess, tic-tac-toe)
- Image pixel data
- Statistical data analysis

### 1.3 Key Characteristics

**Fixed-Size Collection:**

```cpp
int arr[10];  // Size is fixed at declaration
// arr[15] = 5;  // DANGER: Out of bounds!
```

**Contiguous Memory:**

```
Memory:  [arr[0]][arr[1]][arr[2]][arr[3]][arr[4]]
Address: 0x1000  0x1004  0x1008  0x100C  0x1010
         (Each int is 4 bytes)
```

**Zero-Based Indexing:**

```cpp
int arr[5] = {10, 20, 30, 40, 50};
// arr[0] = 10  (first element)
// arr[4] = 50  (last element)
// arr[5] = ???  (ERROR: out of bounds)
```

---

## 2. Array Declaration and Initialization

### 2.1 Array Declaration Syntax

**Basic Syntax:**

```cpp
data_type array_name[size];
```

**Example:**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Reserve space for 5 integers
    int numbers[5];

    // WHY: Create array for daily temperatures
    float temperatures[7];  // 7 days

    // WHY: Store student grades
    char grades[100];

    // WHY: Boolean flags for task completion
    bool tasksDone[10];

    return 0;
}
```

**Important:** Array size must be a compile-time constant.

```cpp
// ✅ VALID
int arr1[10];              // Literal constant
const int SIZE = 5;
int arr2[SIZE];            // const variable

// ❌ INVALID in standard C++
int n = 5;
int arr3[n];              // Variable size (VLA - not standard)
```

### 2.2 Array Initialization

**Method 1: List Initialization**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Initialize with specific values
    int arr1[5] = {10, 20, 30, 40, 50};

    // WHY: Partial initialization (rest are 0)
    int arr2[5] = {1, 2};  // {1, 2, 0, 0, 0}

    // WHY: Size deduced from initializer list
    int arr3[] = {5, 10, 15, 20};  // Size = 4

    // WHY: Zero-initialize entire array
    int arr4[100] = {0};  // All elements = 0

    // Display arr1
    for (int i = 0; i < 5; i++) {
        cout << arr1[i] << " ";
    }
    cout << endl;

    return 0;
}
```

**Output:**

```
10 20 30 40 50
```

**Method 2: Uniform Initialization (C++11)**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Modern C++11 initialization syntax
    int arr1{1, 2, 3, 4, 5};        // Size deduced
    int arr2[3]{10, 20, 30};        // Explicit size
    double arr3[5]{};               // Zero-initialized

    for (int val : arr1) {
        cout << val << " ";
    }

    return 0;
}
```

**Method 3: Element-by-Element Initialization**

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5];

    // WHY: Initialize with computed values
    for (int i = 0; i < 5; i++) {
        arr[i] = i * i;  // Squares: 0, 1, 4, 9, 16
    }

    for (int i = 0; i < 5; i++) {
        cout << arr[i] << " ";
    }

    return 0;
}
```

**Output:**

```
0 1 4 9 16
```

### 2.3 Important Initialization Rules

```cpp
#include <iostream>
using namespace std;

int main() {
    // ✅ VALID
    int arr1[5] = {1, 2, 3};        // Partial init: {1,2,3,0,0}
    int arr2[5] = {0};              // All zeros
    int arr3[5] = {};               // All zeros (C++11)
    int arr4[5]{1, 2};              // Modern syntax

    // ❌ INVALID
    // int arr5[3] = {1, 2, 3, 4};  // Too many initializers

    // ⚠️ CAUTION
    int arr6[5];                    // Uninitialized - garbage values!

    // WHY: Always initialize to avoid garbage
    for (int i = 0; i < 5; i++) {
        cout << arr6[i] << " ";     // Undefined behavior!
    }

    return 0;
}
```

---

## 3. Accessing and Modifying Array Elements

### 3.1 Accessing Elements

**Syntax:**

```cpp
array_name[index]
```

**Example:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int scores[5] = {85, 92, 78, 95, 88};

    // WHY: Access individual exam scores
    cout << "First exam: " << scores[0] << endl;
    cout << "Third exam: " << scores[2] << endl;
    cout << "Last exam: " << scores[4] << endl;

    // WHY: Use expression as index
    int index = 1;
    cout << "Second exam: " << scores[index] << endl;

    return 0;
}
```

**Output:**

```
First exam: 85
Third exam: 78
Last exam: 88
Second exam: 92
```

### 3.2 Modifying Elements

```cpp
#include <iostream>
using namespace std;

int main() {
    int temperatures[7] = {20, 22, 19, 25, 23, 21, 20};

    cout << "Original temperature on day 3: " << temperatures[2] << endl;

    // WHY: Update temperature reading after correction
    temperatures[2] = 24;

    cout << "Updated temperature on day 3: " << temperatures[2] << endl;

    // WHY: Increment all temperatures by 2 degrees
    for (int i = 0; i < 7; i++) {
        temperatures[i] += 2;
    }

    cout << "After adjustment: ";
    for (int i = 0; i < 7; i++) {
        cout << temperatures[i] << " ";
    }

    return 0;
}
```

**Output:**

```
Original temperature on day 3: 19
Updated temperature on day 3: 24
After adjustment: 22 24 26 27 25 23 22
```

### 3.3 Index Bounds

**Critical Rule:** Valid indices are 0 to (size - 1)

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    // ✅ VALID indices: 0, 1, 2, 3, 4
    cout << arr[0] << endl;  // OK
    cout << arr[4] << endl;  // OK

    // ❌ INVALID indices
    // cout << arr[-1] << endl;   // Negative index - UNDEFINED!
    // cout << arr[5] << endl;    // Out of bounds - UNDEFINED!
    // cout << arr[100] << endl;  // Far out of bounds - CRASH likely!

    // WHY: C++ does NOT check bounds automatically!
    // No runtime error - undefined behavior

    return 0;
}
```

**Consequences of Out-of-Bounds Access:**

- Undefined behavior
- Memory corruption
- Crashes
- Security vulnerabilities

**Best Practice:** Always validate indices!

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int index;

    cout << "Enter index: ";
    cin >> index;

    // WHY: Validate before accessing
    if (index >= 0 && index < 5) {
        cout << "arr[" << index << "] = " << arr[index] << endl;
    } else {
        cout << "Invalid index!" << endl;
    }

    return 0;
}
```

---

## 4. Array Traversal

### 4.1 Traditional for Loop

```cpp
#include <iostream>
using namespace std;

int main() {
    int numbers[6] = {5, 10, 15, 20, 25, 30};
    int size = 6;

    // WHY: Iterate using index
    cout << "Elements: ";
    for (int i = 0; i < size; i++) {
        cout << numbers[i] << " ";
    }
    cout << endl;

    return 0;
}
```

### 4.2 Range-Based for Loop (C++11)

```cpp
#include <iostream>
using namespace std;

int main() {
    int numbers[] = {5, 10, 15, 20, 25, 30};

    // WHY: Cleaner syntax when index not needed
    cout << "Elements: ";
    for (int num : numbers) {
        cout << num << " ";
    }
    cout << endl;

    // WHY: Modify elements with reference
    for (int &num : numbers) {
        num *= 2;  // Double each element
    }

    cout << "After doubling: ";
    for (int num : numbers) {
        cout << num << " ";
    }

    return 0;
}
```

**Output:**

```
Elements: 5 10 15 20 25 30
After doubling: 10 20 30 40 50 60
```

### 4.3 Reverse Traversal

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    int size = 5;

    // WHY: Iterate from end to start
    cout << "Reversed: ";
    for (int i = size - 1; i >= 0; i--) {
        cout << arr[i] << " ";
    }

    return 0;
}
```

**Output:**

```
Reversed: 5 4 3 2 1
```

### 4.4 Common Traversal Operations

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[] = {23, 45, 12, 67, 34};
    int size = 5;

    // WHY: Find sum of all elements
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    cout << "Sum: " << sum << endl;

    // WHY: Find maximum element
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    cout << "Maximum: " << max << endl;

    // WHY: Count elements greater than 30
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (arr[i] > 30) {
            count++;
        }
    }
    cout << "Elements > 30: " << count << endl;

    return 0;
}
```

**Output:**

```
Sum: 181
Maximum: 67
Elements > 30: 3
```

---

## 5. Array Size and Memory

### 5.1 sizeof Operator

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[10];
    char charArr[5];
    double dblArr[8];

    // WHY: Get total array size in bytes
    cout << "sizeof(arr): " << sizeof(arr) << " bytes" << endl;
    cout << "sizeof(charArr): " << sizeof(charArr) << " bytes" << endl;
    cout << "sizeof(dblArr): " << sizeof(dblArr) << " bytes" << endl;

    // WHY: Calculate number of elements
    int numElements = sizeof(arr) / sizeof(arr[0]);
    cout << "Number of elements in arr: " << numElements << endl;

    return 0;
}
```

**Output (typical 64-bit system):**

```
sizeof(arr): 40 bytes
sizeof(charArr): 5 bytes
sizeof(dblArr): 64 bytes
Number of elements in arr: 10
```

### 5.2 Memory Layout

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    // WHY: Verify contiguous memory allocation
    cout << "Array memory addresses:" << endl;
    for (int i = 0; i < 5; i++) {
        cout << "arr[" << i << "] at " << &arr[i] << endl;
    }

    // WHY: Calculate element size
    long long diff = (long long)&arr[1] - (long long)&arr[0];
    cout << "\nBytes between elements: " << diff << endl;

    return 0;
}
```

**Sample Output:**

```
Array memory addresses:
arr[0] at 0x7ffd1234
arr[1] at 0x7ffd1238
arr[2] at 0x7ffd123C
arr[3] at 0x7ffd1240
arr[4] at 0x7ffd1244

Bytes between elements: 4
```

### 5.3 Calculating Array Length

```cpp
#include <iostream>
using namespace std;

int main() {
    int scores[] = {85, 92, 78, 95, 88, 91};

    // WHY: Common idiom to get array length
    int length = sizeof(scores) / sizeof(scores[0]);

    cout << "Array has " << length << " elements" << endl;

    // Display all elements
    for (int i = 0; i < length; i++) {
        cout << scores[i] << " ";
    }

    return 0;
}
```

**Output:**

```
Array has 6 elements
85 92 78 95 88 91
```

---

## 6. Multi-Dimensional Arrays

### 6.1 Two-Dimensional Arrays

**Declaration:**

```cpp
data_type array_name[rows][columns];
```

**Example:**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Represent a 3x4 matrix (3 rows, 4 columns)
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };

    // WHY: Access element at row 1, column 2
    cout << "Element [1][2]: " << matrix[1][2] << endl;

    // WHY: Display entire matrix
    cout << "\nMatrix:" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }

    return 0;
}
```

**Output:**

```
Element [1][2]: 7

Matrix:
1       2       3       4
5       6       7       8
9       10      11      12
```

### 6.2 Memory Layout of 2D Arrays

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    // WHY: 2D arrays are stored in row-major order
    cout << "Memory layout (contiguous):" << endl;
    cout << "&arr[0][0]: " << &arr[0][0] << " -> " << arr[0][0] << endl;
    cout << "&arr[0][1]: " << &arr[0][1] << " -> " << arr[0][1] << endl;
    cout << "&arr[0][2]: " << &arr[0][2] << " -> " << arr[0][2] << endl;
    cout << "&arr[1][0]: " << &arr[1][0] << " -> " << arr[1][0] << endl;
    cout << "&arr[1][1]: " << &arr[1][1] << " -> " << arr[1][1] << endl;
    cout << "&arr[1][2]: " << &arr[1][2] << " -> " << arr[1][2] << endl;

    return 0;
}
```

**Memory Diagram:**

```
[1][2][3][4][5][6]  <- Contiguous in memory
 ^     ^     ^
row 0  row 0 row 1
col 0  col 2 col 0
```

### 6.3 Three-Dimensional Arrays

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Represent 3D space (e.g., voxel data, RGB color cube)
    int cube[2][3][4];

    // WHY: Initialize 3D array
    int value = 1;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 4; k++) {
                cube[i][j][k] = value++;
            }
        }
    }

    // WHY: Display one layer
    cout << "Layer 0:" << endl;
    for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 4; k++) {
            cout << cube[0][j][k] << "\t";
        }
        cout << endl;
    }

    return 0;
}
```

**Output:**

```
Layer 0:
1       2       3       4
5       6       7       8
9       10      11      12
```

### 6.4 Practical Example: Matrix Operations

```cpp
#include <iostream>
using namespace std;

int main() {
    const int ROWS = 2, COLS = 3;
    int matrix1[ROWS][COLS] = {{1, 2, 3}, {4, 5, 6}};
    int matrix2[ROWS][COLS] = {{7, 8, 9}, {10, 11, 12}};
    int result[ROWS][COLS];

    // WHY: Add two matrices
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            result[i][j] = matrix1[i][j] + matrix2[i][j];
        }
    }

    // Display result
    cout << "Matrix Addition Result:" << endl;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            cout << result[i][j] << "\t";
        }
        cout << endl;
    }

    return 0;
}
```

**Output:**

```
Matrix Addition Result:
8       10      12
14      16      18
```

---

## 7. Arrays and Pointers

### 7.1 Array Name as Pointer

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    // WHY: Array name is pointer to first element
    cout << "arr: " << arr << endl;
    cout << "&arr[0]: " << &arr[0] << endl;
    cout << "Same address? " << (arr == &arr[0]) << endl;

    // WHY: Dereference array name to get first element
    cout << "*arr: " << *arr << endl;
    cout << "arr[0]: " << arr[0] << endl;

    return 0;
}
```

**Output:**

```
arr: 0x7ffd12345678
&arr[0]: 0x7ffd12345678
Same address? 1
*arr: 10
arr[0]: 10
```

### 7.2 Pointer Arithmetic with Arrays

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[] = {100, 200, 300, 400, 500};
    int* ptr = arr;  // Pointer to first element

    // WHY: Access elements via pointer arithmetic
    cout << "*(ptr + 0): " << *(ptr + 0) << endl;  // arr[0]
    cout << "*(ptr + 1): " << *(ptr + 1) << endl;  // arr[1]
    cout << "*(ptr + 2): " << *(ptr + 2) << endl;  // arr[2]

    // WHY: Relationship between [] and pointer arithmetic
    cout << "\nEquivalence:" << endl;
    cout << "arr[3] = " << arr[3] << endl;
    cout << "*(arr + 3) = " << *(arr + 3) << endl;

    // WHY: Iterate using pointer
    cout << "\nUsing pointer iteration:" << endl;
    for (int i = 0; i < 5; i++) {
        cout << *(ptr + i) << " ";
    }

    return 0;
}
```

**Output:**

```
*(ptr + 0): 100
*(ptr + 1): 200
*(ptr + 2): 300

Equivalence:
arr[3] = 400
*(arr + 3) = 400

Using pointer iteration:
100 200 300 400 500
```

### 7.3 Array vs Pointer Differences

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int* ptr = arr;

    // WHY: sizeof gives different results
    cout << "sizeof(arr): " << sizeof(arr) << " bytes" << endl;
    cout << "sizeof(ptr): " << sizeof(ptr) << " bytes" << endl;

    // WHY: Array name cannot be reassigned
    // arr = ptr;  // ERROR: array is not modifiable lvalue

    // WHY: Pointer can be reassigned
    ptr = &arr[2];  // OK
    cout << "*ptr after reassignment: " << *ptr << endl;

    return 0;
}
```

**Output (64-bit system):**

```
sizeof(arr): 20 bytes
sizeof(ptr): 8 bytes
*ptr after reassignment: 3
```

---

## 8. Array Decay

### 8.1 What is Array Decay?

**Definition:** Array decay is the implicit conversion of an array to a pointer to its first element.

```cpp
#include <iostream>
using namespace std;

void printSize(int arr[]) {
    // WHY: Array has decayed to pointer here!
    cout << "sizeof(arr) inside function: " << sizeof(arr) << endl;
}

int main() {
    int numbers[10];

    // WHY: sizeof works correctly in original scope
    cout << "sizeof(numbers) in main: " << sizeof(numbers) << endl;

    // WHY: Passing array causes decay to pointer
    printSize(numbers);

    return 0;
}
```

**Output (64-bit system):**

```
sizeof(numbers) in main: 40
sizeof(arr) inside function: 8
```

### 8.2 Consequences of Array Decay

```cpp
#include <iostream>
using namespace std;

// WHY: These three declarations are IDENTICAL!
void func1(int arr[]);
void func2(int arr[10]);  // Size ignored!
void func3(int* arr);     // Decays to pointer

// All become:
void processArray(int* arr) {
    // WHY: Cannot determine array size here
    // int size = sizeof(arr) / sizeof(arr[0]);  // WRONG!
    cout << "Pointer size: " << sizeof(arr) << endl;
}

int main() {
    int data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    processArray(data);  // Decays to int*

    return 0;
}
```

### 8.3 Preventing Information Loss

**Solution 1: Pass size separately**

```cpp
#include <iostream>
using namespace std;

void printArray(int arr[], int size) {
    // WHY: Now we know the actual array size
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

int main() {
    int numbers[] = {5, 10, 15, 20, 25};
    int size = sizeof(numbers) / sizeof(numbers[0]);

    printArray(numbers, size);

    return 0;
}
```

**Solution 2: Pass by reference (preserves size)**

```cpp
#include <iostream>
using namespace std;

// WHY: Reference to array does NOT decay
void printArrayRef(int (&arr)[5]) {
    // WHY: sizeof works correctly!
    int size = sizeof(arr) / sizeof(arr[0]);

    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

int main() {
    int numbers[5] = {1, 2, 3, 4, 5};

    printArrayRef(numbers);  // No decay!

    return 0;
}
```

**Solution 3: Use std::array (modern C++)**

```cpp
#include <iostream>
#include <array>
using namespace std;

// WHY: std::array does NOT decay
void printStdArray(array<int, 5> arr) {
    cout << "Size: " << arr.size() << endl;

    for (int val : arr) {
        cout << val << " ";
    }
    cout << endl;
}

int main() {
    array<int, 5> numbers = {1, 2, 3, 4, 5};

    printStdArray(numbers);

    return 0;
}
```

---

## 9. Passing Arrays to Functions

### 9.1 Basic Array Passing

```cpp
#include <iostream>
using namespace std;

// WHY: Standard way with separate size parameter
void displayArray(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

// WHY: Modify array elements (changes reflect in original)
void doubleElements(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        arr[i] *= 2;
    }
}

int main() {
    int data[] = {5, 10, 15, 20, 25};
    int size = 5;

    cout << "Original: ";
    displayArray(data, size);

    doubleElements(data, size);

    cout << "After doubling: ";
    displayArray(data, size);

    return 0;
}
```

**Output:**

```
Original: 5 10 15 20 25
After doubling: 10 20 30 40 50
```

### 9.2 Const Array Parameters

```cpp
#include <iostream>
using namespace std;

// WHY: const prevents accidental modification
void printSum(const int arr[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
        // arr[i] = 0;  // ERROR: cannot modify const array
    }
    cout << "Sum: " << sum << endl;
}

int main() {
    int values[] = {10, 20, 30, 40, 50};
    printSum(values, 5);

    return 0;
}
```

### 9.3 Passing Multi-Dimensional Arrays

```cpp
#include <iostream>
using namespace std;

// WHY: First dimension optional, others required
void printMatrix(int matrix[][3], int rows) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 3; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
}

int main() {
    int mat[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    printMatrix(mat, 2);

    return 0;
}
```

**Output:**

```
1 2 3
4 5 6
```

### 9.4 Returning Arrays from Functions

**❌ INVALID: Cannot return C-style arrays**

```cpp
// int[] createArray() {  // ERROR!
//     int arr[5] = {1, 2, 3, 4, 5};
//     return arr;  // WRONG: returns pointer to local array
// }
```

**✅ VALID Solutions:**

**Option 1: Return pointer to dynamic array**

```cpp
#include <iostream>
using namespace std;

int* createArray(int size) {
    // WHY: Allocate on heap (persists after function returns)
    int* arr = new int[size];

    for (int i = 0; i < size; i++) {
        arr[i] = i * 10;
    }

    return arr;
}

int main() {
    int* dynamicArr = createArray(5);

    for (int i = 0; i < 5; i++) {
        cout << dynamicArr[i] << " ";
    }

    // WHY: Must manually delete
    delete[] dynamicArr;

    return 0;
}
```

**Option 2: Use std::array**

```cpp
#include <iostream>
#include <array>
using namespace std;

array<int, 5> createArray() {
    // WHY: std::array can be returned by value
    array<int, 5> arr = {10, 20, 30, 40, 50};
    return arr;
}

int main() {
    array<int, 5> myArr = createArray();

    for (int val : myArr) {
        cout << val << " ";
    }

    return 0;
}
```

---

## 10. Character Arrays (C-style Strings)

### 10.1 Character Array Basics

```cpp
#include <iostream>
#include <cstring>  // For C-string functions
using namespace std;

int main() {
    // WHY: Null terminator '\0' marks end of string
    char str1[6] = {'H', 'e', 'l', 'l', 'o', '\0'};

    // WHY: String literal automatically adds '\0'
    char str2[] = "World";

    // WHY: Reserve space for future input
    char str3[100];

    cout << "str1: " << str1 << endl;
    cout << "str2: " << str2 << endl;

    // WHY: String length
    cout << "Length of str2: " << strlen(str2) << endl;

    return 0;
}
```

**Output:**

```
str1: Hello
str2: World
Length of str2: 5
```

### 10.2 Null Terminator Importance

```cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ NO null terminator
    char bad[5] = {'H', 'e', 'l', 'l', 'o'};

    // ✅ WITH null terminator
    char good[6] = {'H', 'e', 'l', 'l', 'o', '\0'};

    cout << "bad: " << bad << endl;   // Undefined! Prints garbage
    cout << "good: " << good << endl;  // Correct

    return 0;
}
```

### 10.3 String Input/Output

```cpp
#include <iostream>
using namespace std;

int main() {
    char name[50];
    char fullName[100];

    // WHY: cin stops at whitespace
    cout << "Enter first name: ";
    cin >> name;
    cout << "You entered: " << name << endl;

    // WHY: cin.getline() reads full line
    cin.ignore();  // Clear newline from buffer
    cout << "Enter full name: ";
    cin.getline(fullName, 100);
    cout << "You entered: " << fullName << endl;

    return 0;
}
```

### 10.4 Common C-string Functions

```cpp
#include <iostream>
#include <cstring>
using namespace std;

int main() {
    char str1[50] = "Hello";
    char str2[50] = "World";
    char str3[50];

    // WHY: String length
    cout << "Length of str1: " << strlen(str1) << endl;

    // WHY: String copy
    strcpy(str3, str1);
    cout << "str3 after copy: " << str3 << endl;

    // WHY: String concatenation
    strcat(str1, " ");
    strcat(str1, str2);
    cout << "str1 after concat: " << str1 << endl;

    // WHY: String comparison
    if (strcmp(str2, "World") == 0) {
        cout << "str2 is 'World'" << endl;
    }

    return 0;
}
```

**Output:**

```
Length of str1: 5
str3 after copy: Hello
str1 after concat: Hello World
str2 is 'World'
```

---

## 11. std::array (C++11)

### 11.1 Introduction to std::array

**Why std::array?**

- Knows its own size
- No pointer decay
- STL container benefits
- Zero overhead over C-arrays

```cpp
#include <iostream>
#include <array>
using namespace std;

int main() {
    // WHY: Modern C++ array with size information
    array<int, 5> arr = {10, 20, 30, 40, 50};

    // WHY: size() method available
    cout << "Size: " << arr.size() << endl;

    // WHY: Access elements
    cout << "First: " << arr[0] << endl;
    cout << "Last: " << arr[arr.size() - 1] << endl;

    return 0;
}
```

### 11.2 std::array vs C-style Arrays

```cpp
#include <iostream>
#include <array>
using namespace std;

void processStdArray(array<int, 5> arr) {
    // WHY: Size preserved!
    cout << "std::array size in function: " << arr.size() << endl;
}

void processCArray(int arr[]) {
    // WHY: Size lost due to decay
    cout << "C-array size in function: " << sizeof(arr) << " (just pointer)" << endl;
}

int main() {
    array<int, 5> stdArr = {1, 2, 3, 4, 5};
    int cArr[5] = {1, 2, 3, 4, 5};

    processStdArray(stdArr);
    processCArray(cArr);

    return 0;
}
```

### 11.3 std::array Member Functions

```cpp
#include <iostream>
#include <array>
#include <algorithm>
using namespace std;

int main() {
    array<int, 6> arr = {30, 10, 50, 20, 40, 60};

    // WHY: front() and back()
    cout << "Front: " << arr.front() << endl;
    cout << "Back: " << arr.back() << endl;

    // WHY: at() with bounds checking
    cout << "Element at index 2: " << arr.at(2) << endl;
    // cout << arr.at(10) << endl;  // Throws exception!

    // WHY: fill() entire array
    array<int, 5> zeros;
    zeros.fill(0);

    // WHY: empty() check
    cout << "Is empty? " << arr.empty() << endl;

    // WHY: Use STL algorithms
    sort(arr.begin(), arr.end());

    cout << "Sorted: ";
    for (int val : arr) {
        cout << val << " ";
    }

    return 0;
}
```

**Output:**

```
Front: 30
Back: 60
Element at index 2: 50
Is empty? 0
Sorted: 10 20 30 40 50 60

```

### 11.4 When to Use std::array vs C-arrays

**Use std::array when:**

- ✅ Size known at compile time
- ✅ Need container features
- ✅ Want automatic bounds checking (at())
- ✅ Writing modern C++ code

**Use C-arrays when:**

- Legacy code compatibility required
- Interfacing with C libraries
- Extreme performance critical (negligible difference usually)

---

## 12. Common Mistakes and Best Practices

### 12.1 Common Mistakes

**Mistake 1: Out-of-bounds access**

```cpp
// ❌ WRONG
int arr[5];
arr[5] = 100;  // Index 5 is out of bounds!
```

**Mistake 2: Uninitialized arrays**

```cpp
// ❌ RISKY
int arr[100];  // Contains garbage values!

// ✅ CORRECT
int arr[100] = {0};  // Initialize to zero
```

**Mistake 3: Array size in function**

```cpp
// ❌ WRONG
void func(int arr[]) {
    int size = sizeof(arr) / sizeof(arr[0]);  // WRONG! arr is pointer
}

// ✅ CORRECT
void func(int arr[], int size) {
    // Use passed size parameter
}
```

**Mistake 4: Returning local array**

```cpp
// ❌ WRONG
int* getArray() {
    int arr[5] = {1, 2, 3, 4, 5};
    return arr;  // Returns pointer to destroyed local array!
}

// ✅ CORRECT
int* getArray() {
    int* arr = new int[5]{1, 2, 3, 4, 5};
    return arr;  // Heap memory persists
}
```

**Mistake 5: sizeof on decayed array**

```cpp
void process(int arr[]) {
    // ❌ WRONG
    int size = sizeof(arr);  // Just pointer size!
}
```

### 12.2 Best Practices

**✅ Always initialize arrays**

```cpp
int arr[100] = {0};           // Zero-initialize
int arr2[5] = {1, 2, 3, 4, 5}; // Full initialization
```

**✅ Validate array indices**

```cpp
if (index >= 0 && index < size) {
    arr[index] = value;
}
```

**✅ Use const for read-only arrays**

```cpp
void displayArray(const int arr[], int size) {
    // Cannot modify arr
}
```

**✅ Prefer std::array in modern C++**

```cpp
#include <array>
array<int, 10> arr;  // Knows size, safer
```

**✅ Use sizeof correctly**

```cpp
int arr[10];
int size = sizeof(arr) / sizeof(arr[0]);  // Correct way
```

**✅ Bounds-checked access with at()**

```cpp
#include <array>
array<int, 5> arr = {1, 2, 3, 4, 5};
try {
    cout << arr.at(10);  // Throws exception
} catch (out_of_range& e) {
    cout << "Index out of range!" << endl;
}
```

---

## Summary

### Key Takeaways

1. **Arrays store multiple values** - Fixed-size collection of same-type elements in contiguous memory
2. **Zero-based indexing** - First element at index 0, last at (size-1)
3. **sizeof for array length** - Calculate size with `sizeof(arr)/sizeof(arr[0])` in original scope
4. **Array decay** - Arrays decay to pointers when passed to functions, losing size information
5. **Always pass size** - When passing arrays to functions, include size as separate parameter
6. **Contiguous memory** - Elements stored sequentially, enabling O(1) random access
7. **Multi-dimensional arrays** - 2D, 3D arrays stored in row-major order
8. **Pointer arithmetic** - `arr[i]` equivalent to `(arr + i)`
9. **C-string null terminator** - Character arrays need `'\0'` to mark string end
10. **std::array advantage** - Modern C++11 alternative that knows its size and doesn't decay

### Interview Essential Points

**Q: What is an array and why use it?**

A: An array is a fixed-size collection of elements of the same type stored in contiguous memory locations. Arrays provide O(1) random access to elements via index calculation (base_address + index * element_size), making them extremely efficient for accessing elements. They're used when you need to store multiple related values, iterate through data, or when cache locality matters for performance. The contiguous memory layout improves cache performance compared to scattered data structures.

**Q: Explain array decay and why it matters.**

A: Array decay is the implicit conversion of an array name to a pointer to its first element. This happens when arrays are passed to functions or used in pointer contexts. During decay, the array loses its size information - sizeof() on a decayed array returns the pointer size (typically 8 bytes on 64-bit systems) rather than the full array size. This is why functions accepting arrays must take a separate size parameter. Understanding decay is crucial to avoid bugs from assuming size information is preserved.

**Q: How are multi-dimensional arrays stored in memory?**

A: Multi-dimensional arrays in C++ are stored in row-major order, meaning elements are stored row by row in contiguous memory. For a 2D array `int arr[3][4]`, all elements of row 0 are stored first, then row 1, then row 2. This is important for: (1) performance - accessing elements row-by-row is cache-friendly due to spatial locality, (2) pointer arithmetic - calculating element addresses requires knowing row size, (3) function parameters - only the first dimension can be omitted when passing to functions because the compiler needs column size for address calculation.

- *Q: What's the difference between arr[i] and (arr + i)?*

A: They are equivalent. `arr[i]` is syntactic sugar for `*(arr + i)`. The subscript operator [] performs pointer arithmetic: it takes the base address (arr), adds i times the element size, then dereferences. For example, if arr is at address 0x1000 and holds ints (4 bytes), arr[3] calculates *(0x1000 + 3*4) = *(0x100C). This equivalence is why array indexing is so fast - it's just address calculation and dereference, both O(1) operations.

**Q: Why can't you return a local array from a function?**

A: Local arrays are allocated on the stack and destroyed when the function returns. Returning a pointer to a local array creates a dangling pointer pointing to deallocated memory. Accessing this memory causes undefined behavior. Solutions: (1) allocate array on heap with new[] and return pointer (caller must delete[]), (2) use std::array which can be returned by value, (3) pass output array as parameter, (4) use std::vector for dynamic sizing.

**Q: Compare C-style arrays vs std::array.**

A: C-style arrays: (1) decay to pointers losing size info, (2) no bounds checking, (3) cannot be copied with assignment, (4) can't be returned from functions, (5) faster compilation. std::array: (1) knows its size via size() method, (2) doesn't decay to pointer, (3) bounds-checked access with at(), (4) can be copied and returned, (5) works with STL algorithms, (6) zero overhead compared to C-arrays. Use std::array in modern C++ for safety; use C-arrays for legacy code or C library interfacing.

**Q: What happens if you access an array out of bounds?**

A: C++ does NOT perform automatic bounds checking for arrays. Out-of-bounds access causes undefined behavior, which might: (1) read/write garbage values, (2) corrupt adjacent memory, (3) crash the program with segmentation fault, (4) create security vulnerabilities (buffer overflow attacks), (5) appear to work but fail unpredictably later. The behavior is non-deterministic and compiler/platform dependent. Always validate indices before access, or use std::array::at() which throws std::out_of_range exception on invalid index.

**Q: How do you properly pass multi-dimensional arrays to functions?**

A: For 2D arrays, you must specify all dimensions except the first: `void func(int arr[][COLS], int rows)`. This is because the compiler needs to know column count to calculate element addresses using row-major order: `arr[i][j]` becomes `*(arr + i*COLS + j)`. Alternative approaches: (1) pass as pointer to array: `void func(int (*arr)[COLS], int rows)`, (2) flatten to 1D and calculate indices manually, (3) use std::vector<vector<int>> for dynamic sizing, (4) use std::array<std::array<int, COLS>, ROWS> for compile-time sizing.

---