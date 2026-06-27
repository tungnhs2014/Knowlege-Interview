# 3.4. Strings in C++

---

## Table of Contents

1. What are Strings?
2. C-Style Strings (char arrays)
3. std::string Class
4. String Declaration and Initialization
5. Accessing String Characters
6. String Length and Capacity
7. String Concatenation
8. String Comparison
9. String Modification Methods
10. String Searching Methods
11. Substring Operations
12. String Conversion Methods
13. String Streams
14. Raw String Literals (C++11)
15. string_view (C++17)
16. C-Style vs std::string Performance
17. Best Practices and Common Pitfalls

---

## 1. What are Strings?

### 1.1 Definition

**A string is a sequence of characters used to represent text.** In C++, strings can be handled in two ways: C-style strings (character arrays) and std::string objects (C++ string class).

**Core Concept:**

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Two ways to represent strings in C++

    // C-style string (char array)
    char cstr[] = "Hello";

    // C++ string (std::string object)
    string cppstr = "World";

    cout << "C-style: " << cstr << endl;
    cout << "C++ string: " << cppstr << endl;

    return 0;
}
```

**Output:**

```
C-style: Hello
C++ string: World
```

### 1.2 Why Two String Types?

**Purpose:**

1. **C-Style Strings (char[])**
    - Legacy compatibility with C
    - Low-level memory control
    - Performance-critical scenarios
    - Interfacing with C libraries
2. **std::string Class**
    - Automatic memory management
    - Rich functionality (methods)
    - Safer (no buffer overflows)
    - Modern C++ standard

**Real-World Applications:**

- Text processing (editors, parsers)
- User input handling
- File I/O operations
- Network communication (protocols)
- Database queries
- Configuration files
- JSON/XML parsing

---

## 2. C-Style Strings (char arrays)

### 2.1 Null Terminator

**Critical Concept:** C-style strings MUST end with `'\0'` (null terminator)

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Null terminator marks string end
    char str1[] = "Hello";  // Compiler adds '\0' automatically
    char str2[] = {'H', 'e', 'l', 'l', 'o', '\0'};  // Manual

    cout << "str1: " << str1 << endl;
    cout << "str2: " << str2 << endl;

    // WHY: Size includes null terminator
    cout << "Size of str1: " << sizeof(str1) << " bytes" << endl;  // 6

    return 0;
}
```

**Output:**

```
str1: Hello
str2: Hello
Size of str1: 6 bytes
```

### 2.2 C-Style String Functions

```cpp
#include <iostream>
#include <cstring>  // WHY: C string functions header
using namespace std;

int main() {
    char str1[50] = "Hello";
    char str2[50] = "World";
    char str3[50];

    // WHY: strlen() - get length (excluding '\0')
    cout << "Length of str1: " << strlen(str1) << endl;

    // WHY: strcpy() - copy string
    strcpy(str3, str1);
    cout << "str3 after copy: " << str3 << endl;

    // WHY: strcat() - concatenate strings
    strcat(str1, " ");
    strcat(str1, str2);
    cout << "After concatenation: " << str1 << endl;

    // WHY: strcmp() - compare strings
    if (strcmp(str2, "World") == 0) {
        cout << "Strings are equal" << endl;
    }

    return 0;
}
```

**Output:**

```
Length of str1: 5
str3 after copy: Hello
After concatenation: Hello World
Strings are equal
```

### 2.3 Dangers of C-Style Strings

```cpp
#include <iostream>
#include <cstring>
using namespace std;

int main() {
    // ❌ DANGER: Buffer overflow!
    char small[5] = "Hi";
    // strcpy(small, "This is too long");  // CRASH!

    // ✅ SAFER: Use strncpy with size limit
    char safe[10];
    strncpy(safe, "Hello World", 9);
    safe[9] = '\0';  // WHY: Manual null terminator
    cout << "Safe copy: " << safe << endl;

    return 0;
}
```

---

## 3. std::string Class

### 3.1 Introduction

**std::string is a class from the C++ Standard Library that manages character sequences automatically.**

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Automatic memory management
    string str = "Hello";

    cout << "String: " << str << endl;
    cout << "Length: " << str.length() << endl;
    cout << "Size: " << str.size() << endl;

    return 0;
}
```

### 3.2 Advantages over C-Style

| Feature | C-Style (char[]) | std::string |
| --- | --- | --- |
| **Memory** | Manual management | Automatic |
| **Safety** | Buffer overflow risk | Bounds-checked |
| **Resizing** | Fixed size | Dynamic |
| **Copying** | strcpy() needed | Assignment works |
| **Concatenation** | strcat() needed | + operator |
| **Comparison** | strcmp() needed | == operator |
| **Functions** | Limited (cstring) | Rich methods |

---

## 4. String Declaration and Initialization

### 4.1 Various Initialization Methods

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Multiple ways to initialize

    // 1. Direct initialization
    string str1 = "Hello";

    // 2. Constructor initialization
    string str2("World");

    // 3. Copy initialization
    string str3(str1);

    // 4. Substring initialization
    string str4(str1, 0, 3);  // "Hel"

    // 5. Fill initialization
    string str5(5, 'A');  // "AAAAA"

    // 6. Empty string
    string str6;

    cout << "str1: " << str1 << endl;
    cout << "str2: " << str2 << endl;
    cout << "str3: " << str3 << endl;
    cout << "str4: " << str4 << endl;
    cout << "str5: " << str5 << endl;
    cout << "str6 empty: " << str6.empty() << endl;

    return 0;
}
```

**Output:**

```
str1: Hello
str2: World
str3: Hello
str4: Hel
str5: AAAAA
str6 empty: 1
```

### 4.2 Converting Between Types

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Convert C-style to std::string
    char cstr[] = "C-style";
    string str1(cstr);  // Constructor
    string str2 = cstr;  // Assignment

    cout << "str1: " << str1 << endl;

    // WHY: Convert std::string to C-style
    string str3 = "C++ string";
    const char* cstr2 = str3.c_str();  // c_str() method

    cout << "C-style from string: " << cstr2 << endl;

    return 0;
}
```

---

## 5. Accessing String Characters

### 5.1 Using [] Operator

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello";

    // WHY: Access by index (0-based)
    cout << "First char: " << str[0] << endl;
    cout << "Last char: " << str[4] << endl;

    // WHY: Modify character
    str[0] = 'h';
    cout << "Modified: " << str << endl;

    // ⚠️ WARNING: No bounds checking!
    // str[10] = 'X';  // Undefined behavior!

    return 0;
}
```

### 5.2 Using at() Method

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "GeeksforGeeks";

    // WHY: at() provides bounds checking
    try {
        cout << "Char at 0: " << str.at(0) << endl;
        cout << "Char at 6: " << str.at(6) << endl;

        // WHY: Throws exception on out of bounds
        cout << str.at(100);  // Will throw!

    } catch (const out_of_range& e) {
        cout << "Exception: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Char at 0: G
Char at 6: f
Exception: basic_string::at: __n (which is 100) >= this->size() (which is 13)
```

### 5.3 Front and Back

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Welcome";

    // WHY: Convenient access to first/last
    cout << "First: " << str.front() << endl;  // 'W'
    cout << "Last: " << str.back() << endl;    // 'e'

    return 0;

```

---

## 6. String Length and Capacity

### 6.1 length() and size()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: length() and size() are identical
    cout << "Length: " << str.length() << endl;
    cout << "Size: " << str.size() << endl;

    // WHY: Check if empty
    if (!str.empty()) {
        cout << "String is not empty" << endl;
    }

    return 0;

```

### 6.2 capacity() and reserve()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello";

    // WHY: capacity() shows allocated memory
    cout << "Size: " << str.size() << endl;
    cout << "Capacity: " << str.capacity() << endl;

    // WHY: reserve() pre-allocates memory
    str.reserve(100);
    cout << "After reserve(100):" << endl;
    cout << "Size: " << str.size() << endl;
    cout << "Capacity: " << str.capacity() << endl;

    return 0;
}
```

**Output:**

```
Size: 5
Capacity: 15
After reserve(100):
Size: 5
Capacity: 100
```

### 6.3 resize() and clear()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    cout << "Original: " << str << " (size: " << str.size() << ")" << endl;

    // WHY: resize() changes actual length
    str.resize(5);
    cout << "After resize(5): " << str << " (size: " << str.size() << ")" << endl;

    str.resize(10, 'X');
    cout << "After resize(10, 'X'): " << str << " (size: " << str.size() << ")" << endl;

    // WHY: clear() empties the string
    str.clear();
    cout << "After clear(): " << str << " (size: " << str.size() << ")" << endl;

    return 0;
}
```

**Output:**

```
Original: Hello World (size: 11)
After resize(5): Hello (size: 5)
After resize(10, 'X'): HelloXXXXX (size: 10)
After clear():  (size: 0)
```

---

## 7. String Concatenation

### 7.1 Using + Operator

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str1 = "Hello";
    string str2 = "World";

    // WHY: + operator creates new string
    string str3 = str1 + " " + str2;
    cout << "Concatenated: " << str3 << endl;

    // WHY: Can mix string and C-style
    string str4 = str1 + " from C++";
    cout << "Mixed: " << str4 << endl;

    return 0;
}
```

### 7.2 Using append()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello";

    // WHY: append() modifies existing string
    str.append(" World");
    cout << "After append: " << str << endl;

    // WHY: Can append substring
    string str2 = "GeeksforGeeks";
    str.append(str2, 0, 5);  // Append "Geeks"
    cout << "After substring append: " << str << endl;

    // WHY: Can append multiple characters
    str.append(3, '!');
    cout << "After char append: " << str << endl;

    return 0;
}
```

**Output:**

```
After append: Hello World
After substring append: Hello WorldGeeks
After char append: Hello WorldGeeks!!
```

### 7.3 Using += Operator

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "C++";

    // WHY: += is shorthand for append
    str += " is";
    str += " awesome";

    cout << str << endl;

    return 0;
}
```

### 7.4 push_back() and pop_back()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello";

    // WHY: push_back() adds one character
    str.push_back('!');
    cout << "After push_back: " << str << endl;

    // WHY: pop_back() removes last character
    str.pop_back();
    cout << "After pop_back: " << str << endl;

    return 0;
}

```

---

## 8. String Comparison

### 8.1 Using Relational Operators

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str1 = "apple";
    string str2 = "banana";
    string str3 = "apple";

    // WHY: Lexicographical comparison
    if (str1 == str3) {
        cout << "str1 and str3 are equal" << endl;
    }

    if (str1 != str2) {
        cout << "str1 and str2 are different" << endl;
    }

    if (str1 < str2) {
        cout << "str1 comes before str2 alphabetically" << endl;
    }

    return 0;
}
```

### 8.2 Using compare()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str1 = "Hello";
    string str2 = "World";
    string str3 = "Hello";

    // WHY: compare() returns int
    // 0 if equal, <0 if less, >0 if greater
    int result1 = str1.compare(str3);
    cout << "str1.compare(str3): " << result1 << endl;

    int result2 = str1.compare(str2);
    cout << "str1.compare(str2): " << result2 << endl;

    // WHY: Can compare substring
    string str4 = "Hello World";
    int result3 = str4.compare(0, 5, "Hello");
    cout << "Substring compare: " << result3 << endl;

    return 0;
}
```

**Output:**

```
str1.compare(str3): 0
str1.compare(str2): -15
Substring compare: 0
```

---

## 9. String Modification Methods

### 9.1 insert()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "I have a cat";

    // WHY: insert() adds at specified position
    str.insert(9, "black ");
    cout << "After insert: " << str << endl;

    // WHY: Can insert multiple characters
    string str2 = "Hello";
    str2.insert(5, 3, '!');
    cout << "After char insert: " << str2 << endl;

    return 0;
}
```

**Output:**

```
After insert: I have a black cat
After char insert: Hello!!!
```

### 9.2 erase()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: erase() removes characters
    str.erase(5, 6);  // Remove " World"
    cout << "After erase: " << str << endl;

    // WHY: erase(pos) removes from pos to end
    string str2 = "GeeksforGeeks";
    str2.erase(5);
    cout << "After erase from pos: " << str2 << endl;

    return 0;
}
```

**Output:**

```
After erase: Hello
After erase from pos: Geeks
```

### 9.3 replace()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: replace(pos, len, new_str)
    str.replace(6, 5, "C++");
    cout << "After replace: " << str << endl;

    // WHY: Replace with repeated character
    string str2 = "abc123def";
    str2.replace(3, 3, 5, 'X');
    cout << "After char replace: " << str2 << endl;

    return 0;
}
```

**Output:**

```
After replace: Hello C++
After char replace: abcXXXXXdef
```

---

## 10. String Searching Methods

### 10.1 find()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World, World is beautiful";

    // WHY: find() returns position of first occurrence
    size_t pos = str.find("World");

    if (pos != string::npos) {
        cout << "'World' found at position: " << pos << endl;
    }

    // WHY: Find from specific position
    size_t pos2 = str.find("World", pos + 1);
    cout << "Second 'World' at: " << pos2 << endl;

    // WHY: Find character
    size_t pos3 = str.find('o');
    cout << "First 'o' at: " << pos3 << endl;

    return 0;
}
```

**Output:**

```
'World' found at position: 6
Second 'World' at: 13
First 'o' at: 4
```

### 10.2 rfind()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World, World is beautiful";

    // WHY: rfind() searches from right to left
    size_t pos = str.rfind("World");
    cout << "Last 'World' at: " << pos << endl;

    size_t pos2 = str.rfind('o');
    cout << "Last 'o' at: " << pos2 << endl;

    return 0;
}
```

**Output:**

```
Last 'World' at: 13
Last 'o' at: 18
```

### 10.3 find_first_of() and find_last_of()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: Find first occurrence of any character in set
    size_t pos1 = str.find_first_of("aeiou");
    cout << "First vowel at: " << pos1 << " (" << str[pos1] << ")" << endl;

    // WHY: Find last occurrence of any character in set
    size_t pos2 = str.find_last_of("aeiou");
    cout << "Last vowel at: " << pos2 << " (" << str[pos2] << ")" << endl;

    return 0;
}
```

**Output:**

```
First vowel at: 1 (e)
Last vowel at: 7 (o)
```

---

## 11. Substring Operations

### 11.1 substr()

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: substr(pos, len) extracts substring
    string sub1 = str.substr(0, 5);
    cout << "First 5 chars: " << sub1 << endl;

    // WHY: substr(pos) from pos to end
    string sub2 = str.substr(6);
    cout << "From position 6: " << sub2 << endl;

    // WHY: Extract using find
    size_t pos = str.find("World");
    if (pos != string::npos) {
        string sub3 = str.substr(pos, 5);
        cout << "Extracted: " << sub3 << endl;
    }

    return 0;
}
```

**Output:**

```
First 5 chars: Hello
From position 6: World
Extracted: World
```

### 11.2 Practical Example: Parsing

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string email = "user@example.com";

    // WHY: Extract username and domain
    size_t atPos = email.find('@');

    if (atPos != string::npos) {
        string username = email.substr(0, atPos);
        string domain = email.substr(atPos + 1);

        cout << "Username: " << username << endl;
        cout << "Domain: " << domain << endl;
    }

    return 0;
}
```

**Output:**

```
Username: user
Domain: example.com
```

---

## 12. String Conversion Methods

### 12.1 c_str() and data()

```cpp
#include <iostream>
#include <string>
#include <cstring>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: c_str() returns const char* (null-terminated)
    const char* cstr = str.c_str();
    cout << "C-string: " << cstr << endl;
    cout << "Length (strlen): " << strlen(cstr) << endl;

    // WHY: data() similar to c_str() (C++11+)
    const char* data_ptr = str.data();
    cout << "Data: " << data_ptr << endl;

    return 0;
}
```

### 12.2 Numeric Conversions

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Convert string to number
    string str1 = "12345";
    int num = stoi(str1);
    cout << "String to int: " << num << endl;

    string str2 = "3.14159";
    double pi = stod(str2);
    cout << "String to double: " << pi << endl;

    string str3 = "123456789012345";
    long long bigNum = stoll(str3);
    cout << "String to long long: " << bigNum << endl;

    // WHY: Convert number to string
    int value = 42;
    string str4 = to_string(value);
    cout << "Int to string: " << str4 << endl;

    double pi2 = 3.14159;
    string str5 = to_string(pi2);
    cout << "Double to string: " << str5 << endl;

    return 0;
}
```

**Output:**

```
String to int: 12345
String to double: 3.14159
String to long long: 123456789012345
Int to string: 42
Double to string: 3.141590
```

---

## 13. String Streams

### 13.1 stringstream

```cpp
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

int main() {
    // WHY: stringstream for string manipulation
    stringstream ss;

    ss << "Number: " << 42 << ", PI: " << 3.14;
    string result = ss.str();
    cout << result << endl;

    // WHY: Parse from stringstream
    stringstream ss2("100 3.14 Hello");
    int num;
    double pi;
    string word;

    ss2 >> num >> pi >> word;
    cout << "Parsed: " << num << ", " << pi << ", " << word << endl;

    return 0;
}
```

**Output:**

```
Number: 42, PI: 3.14
Parsed: 100, 3.14, Hello
```

### 13.2 istringstream (Input)

```cpp
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

int main() {
    string data = "Alice 25 3.8";
    istringstream iss(data);

    // WHY: Extract multiple types
    string name;
    int age;
    double gpa;

    iss >> name >> age >> gpa;

    cout << "Name: " << name << endl;
    cout << "Age: " << age << endl;
    cout << "GPA: " << gpa << endl;

    return 0;
}
```

### 13.3 ostringstream (Output)

```cpp
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

int main() {
    ostringstream oss;

    // WHY: Build formatted string
    oss << "Student: " << "Bob" << ", Score: " << 95;

    string result = oss.str();
    cout << result << endl;

    return 0;
}
```

### 13.4 Practical: Tokenization

```cpp
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

int main() {
    string text = "apple,banana,orange,grape";
    stringstream ss(text);
    string token;
    vector<string> tokens;

    // WHY: Split string by delimiter
    while (getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    cout << "Tokens:" << endl;
    for (const string& t : tokens) {
        cout << "- " << t << endl;
    }

    return 0;
}
```

**Output:**

```
Tokens:
- apple
- banana
- orange
- grape
```

---

## 14. Raw String Literals (C++11)

### 14.1 Basic Usage

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Avoid escaping special characters

    // Traditional string (need escaping)
    string path1 = "C:\\Users\\Name\\Documents";
    cout << "Traditional: " << path1 << endl;

    // WHY: Raw string literal (no escaping needed)
    string path2 = R"(C:\Users\Name\Documents)";
    cout << "Raw: " << path2 << endl;

    // WHY: Multi-line strings
    string multiline = R"(Line 1
Line 2
Line 3)";
    cout << "Multi-line:\n" << multiline << endl;

    return 0;
}
```

**Output:**

```
Traditional: C:\Users\Name\Documents
Raw: C:\Users\Name\Documents
Multi-line:
Line 1
Line 2
Line 3
```

### 14.2 Custom Delimiters

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // WHY: Handle strings containing )"
    string str = R"delim(String with )" inside)delim";
    cout << str << endl;

    return 0;
}
```

---

## 15. string_view (C++17)

### 15.1 What is string_view?

**string_view is a non-owning read-only view over a character sequence.** It's lightweight and doesn't copy data.

```cpp
#include <iostream>
#include <string>
#include <string_view>
using namespace std;

int main() {
    string str = "Hello World";

    // WHY: string_view doesn't copy
    string_view sv = str;

    cout << "string_view: " << sv << endl;
    cout << "Length: " << sv.length() << endl;

    // WHY: Can create from substring
    string_view sv2 = sv.substr(0, 5);
    cout << "Substring view: " << sv2 << endl;

    return 0;
}
```

### 15.2 Performance Benefits

```cpp
#include <iostream>
#include <string>
#include <string_view>
using namespace std;

// WHY: Avoid copy with string_view parameter
void printString(string_view sv) {
    cout << "View: " << sv << endl;
}

int main() {
    string str = "GeeksforGeeks";

    // WHY: No copy created
    printString(str);
    printString("Literal");

    return 0;
}
```

### 15.3 Limitations

```cpp
#include <iostream>
#include <string>
#include <string_view>
using namespace std;

int main() {
    string_view sv;

    {
        string temp = "Temporary";
        sv = temp;  // ⚠️ DANGER: sv points to temp
    }  // temp destroyed here!

    // cout << sv;  // ❌ Undefined behavior!

    return 0;
}
```

---

## 16. C-Style vs std::string Performance

### 16.1 Memory Overhead

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    char cstr[] = "Hello";
    string str = "Hello";

    cout << "C-style size: " << sizeof(cstr) << " bytes" << endl;
    cout << "std::string size: " << sizeof(str) << " bytes" << endl;

    // WHY: std::string has overhead (pointer, size, capacity)

    return 0;
}
```

**Output:**

```
C-style size: 6 bytes
std::string size: 32 bytes
```

### 16.2 When to Use Each

**Use C-Style Strings When:**

- Interfacing with C libraries
- Performance-critical (minimal overhead)
- Fixed-size, no modification needed
- Embedded systems (limited memory)

**Use std::string When:**

- Dynamic strings (unknown size)
- Need string operations (search, replace)
- Safety is priority
- Modern C++ code

### 16.3 Comparison Table

| Feature | C-Style (char[]) | std::string |
| --- | --- | --- |
| **Memory** | Stack (fixed) | Heap (dynamic) |
| **Overhead** | Minimal (1 byte '\0') | ~32 bytes object |
| **Speed** | Faster (no allocation) | Slower (allocations) |
| **Safety** | Manual bounds check | Automatic |
| **Resizing** | ❌ Cannot | ✅ Can |
| **Operations** | Limited (cstring) | Rich (methods) |
| **Best for** | Small, fixed strings | Dynamic strings |

---

## 17. Best Practices and Common Pitfalls

### 17.1 Common Mistakes

**Mistake 1: Buffer overflow with C-style**

```cpp
// ❌ WRONG
char str[5] = "Hi";
strcat(str, " World");  // OVERFLOW!

// ✅ CORRECT
string str = "Hi";
str += " World";  // Safe
```

**Mistake 2: Comparing C-strings with ==**

```cpp
// ❌ WRONG
char s1[] = "Hello";
char s2[] = "Hello";
if (s1 == s2) { }  // Compares pointers, not content!

// ✅ CORRECT
if (strcmp(s1, s2) == 0) { }  // C-style
// OR
string str1 = "Hello", str2 = "Hello";
if (str1 == str2) { }  // std::string

```

**Mistake 3: Forgetting null terminator**

```cpp
// ❌ WRONG
char str[5] = {'H', 'e', 'l', 'l', 'o'};  // No '\0'!

// ✅ CORRECT
char str[6] = {'H', 'e', 'l', 'l', 'o', '\0'};
// OR
char str[] = "Hello";  // Compiler adds '\0'
```

**Mistake 4: Using c_str() incorrectly**

```cpp
// ❌ WRONG
const char* ptr;
{
    string temp = "Hello";
    ptr = temp.c_str();
}  // temp destroyed, ptr now dangling!

// ✅ CORRECT
string str = "Hello";
const char* ptr = str.c_str();  // Valid as long as str exists
```

### 17.2 Best Practices

**✅ Prefer std::string**

```cpp
// WHY: Safer, easier, more maintainable
string str = "Hello";
str += " World";
```

**✅ Use string_view for read-only parameters**

```cpp
void process(string_view sv) {
    // WHY: No copy, works with string, char[], literals
}
```

**✅ Reserve capacity if size known**

```cpp
string str;
str.reserve(1000);  // WHY: Avoid reallocations
for (int i = 0; i < 1000; i++) {
    str += 'a';
}
```

**✅ Use empty() instead of size() == 0**

```cpp
if (str.empty()) {  // WHY: More readable, potentially faster
    // ...
}
```

**✅ Use const references for parameters**

```cpp
void print(const string& str) {  // WHY: Avoid copy
    cout << str << endl;
}
```

---

## Summary

### Key Takeaways

1. **Two string types** - C-style (char[]) for legacy/performance, std::string for safety/convenience
2. **Null terminator** - C-style strings MUST end with '\0', std::string handles automatically
3. **length() vs size()** - Identical for strings, both return number of characters (excluding '\0' for std::string)
4. **Automatic memory** - std::string manages memory automatically, grows/shrinks as needed
5. **Rich operations** - std::string provides append(), insert(), erase(), replace(), find(), substr()
6. **String streams** - stringstream, istringstream, ostringstream for parsing and formatting
7. **Raw literals (C++11)** - R"(text)" avoids escaping, supports multi-line strings
8. **string_view (C++17)** - Non-owning view, no copy overhead, great for read-only parameters
9. **c_str() method** - Converts std::string to const char* for C library compatibility
10. **Performance trade-off** - C-style faster but unsafe, std::string safer but overhead

### Interview Essential Points

**Q: What is the difference between C-style strings and std::string in C++?**

A: C-style strings are character arrays (char[]) terminated with '\0', requiring manual memory management and cstring functions (strcpy, strcat, strcmp). std::string is a class from STL with automatic memory management, dynamic sizing, and rich built-in methods. Key differences: (1) Memory - C-style is fixed size on stack, std::string is dynamic on heap. (2) Safety - C-style prone to buffer overflows, std::string bounds-checked. (3) Operations - C-style needs cstring functions, std::string has member methods and operator overloading. (4) Overhead - C-style minimal (just null terminator), std::string has ~32 byte object overhead. Use C-style for C compatibility or extreme performance needs; use std::string for modern safe C++ code with dynamic strings.

**Q: Explain the significance of the null terminator in C-style strings.**

A: The null terminator ('\0') is a zero byte marking the end of C-style strings. It's critical because: (1) C functions (strlen, strcpy, etc.) rely on it to find string end - they iterate until '\0'. (2) Size calculation - a string "Hello" needs 6 bytes (5 chars + '\0'). (3) Without it, functions read past the string into undefined memory causing crashes or security issues. The compiler automatically adds '\0' when initializing with string literals: `char s[] = "Hi"` becomes `{'H', 'i', '\0'}`. This is why sizeof() on a string is length+1. std::string doesn't require '\0' internally but c_str() method adds it for C compatibility.

**Q: What are the advantages of std::string over C-style strings?**

A: std::string offers significant advantages: (1) **Automatic memory management** - grows/shrinks dynamically, no manual allocation/deallocation. (2) **Safety** - bounds checking with at(), no buffer overflows from strcat/strcpy. (3) **Rich interface** - 60+ methods for find(), replace(), substr(), insert(), erase() vs limited cstring functions. (4) **Operator overloading** - `+` for concatenation, `==` for comparison vs strcmp(). (5) **Exception safety** - RAII guarantees cleanup, can throw on errors. (6) **Standard library integration** - works seamlessly with STL containers and algorithms. Trade-off: std::string has memory overhead (~32 bytes object + heap allocation) and is slower than C-style for fixed small strings, but the safety and convenience make it the default choice for modern C++.

**Q: How do length(), size(), and capacity() differ for std::string?**

A: All three are member functions but measure different things: (1) **length() and size()** are identical - both return the number of characters currently in the string, excluding null terminator. Example: `string s = "Hello"` has length()==5. (2) **capacity()** returns the number of characters that can be stored without reallocation, always >= size(). Example: after `reserve(100)`, capacity might be 100+ while size remains 5. Capacity grows automatically when needed (typically doubles) to minimize reallocations. Use **resize()** to change actual string size, **reserve()** to pre-allocate memory if final size is known, and **shrink_to_fit()** to reduce capacity to match size. Checking size()==0 vs empty() - prefer empty() as it's more semantic and potentially O(1) guaranteed.

**Q: Explain string_view and when you would use it.**

A: string_view (C++17) is a lightweight non-owning read-only reference to a character sequence. It contains just a pointer and length, no data copy. Benefits: (1) **Zero-copy** - viewing substring creates new view, no allocation: `sv.substr(0,5)` is instant. (2) **Polymorphic** - works with string, char[], literals without conversion. (3) **Performance** - passing string_view parameter avoids copy overhead of `const string&`. Use cases: (1) Read-only function parameters that accept any string type. (2) Temporary views into existing strings. (3) Parsing without creating substrings. **Critical limitation**: string_view doesn't own data, so original string must outlive the view. Assigning string_view from temporary causes dangling reference. Don't return string_view from functions unless source is static/global. Think of it as `const char*` + size, but type-safe.

**Q: What are raw string literals and why are they useful?**

A: Raw string literals (C++11) use R"(text)" syntax to avoid escaping special characters. Inside R"( )", backslashes and quotes are literal characters, not escape sequences. Example: traditional `"C:\\Users\\Name"` vs raw `R"(C:\Users\Name)"` - cleaner and less error-prone. Use cases: (1) **File paths** on Windows with backslashes. (2) **Regex patterns** which use many backslashes - `R"(\d+\.\d+)"` vs `"\\d+\\.\\d+"`. (3) **Multi-line strings** - newlines are preserved literally. (4) **JSON/XML** literals in code. Custom delimiters handle edge case of )" inside string: `R"delim(text with )" inside)delim"`. Raw strings make code more readable when content has many special characters, but regular strings suffice for simple cases.

**Q: How do you convert between std::string and numeric types?**

A: C++11 provides bidirectional conversion functions: **String to number**: stoi() for int, stol() for long, stoll() for long long, stof() for float, stod() for double. Example: `int n = stoi("123")`, `double d = stod("3.14")`. These throw invalid_argument if conversion fails and out_of_range if number too large. **Number to string**: to_string() works for all numeric types. Example: `string s = to_string(42)`, `string pi = to_string(3.14159)`. Pre-C++11 alternatives: istringstream for parsing (`iss >> num`), ostringstream for formatting (`oss << num; s = oss.str()`). C functions atoi/atof still exist but don't error check. For complex formatting (precision, width), use stringstream or newer std::format (C++20).

**Q: What is the purpose of c_str() and data() methods?**

A: Both return const char* pointer to string contents: **c_str()** guarantees null-terminated C-style string, enabling use with C library functions like printf(), strlen(), fopen(). Example: `string s = "file.txt"; FILE* f = fopen(s.c_str(), "r")`. **data()** before C++11 was similar but didn't guarantee null termination; since C++11 it's identical to c_str(). Modern code should prefer c_str() for clarity when C compatibility needed. **Critical warning**: returned pointer is only valid while the string object exists and isn't modified. Storing the pointer beyond string's lifetime causes undefined behavior. The pointer becomes invalid after string operations that might reallocate (insert, append, etc.). For permanent C-style string, copy the data with strcpy(). Generally avoid if possible - use std::string throughout and only convert at API boundaries.

---