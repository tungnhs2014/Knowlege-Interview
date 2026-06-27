# 9.1. File Handling - Basics to Advanced Operations

---

## Table of Contents

1. Introduction to File Handling
2. File Streams Basics
3. Text File Operations
4. Binary File Operations
5. File Error Handling
6. Advanced File Operations
7. Summary
8. Interview Preparation

---

## 1. Introduction to File Handling

### 1.1 What is File Handling?

**File handling** means reading from and writing to files using C++ standard library classes.

**Why File Handling?**

- Programs run in RAM (volatile memory)
- Data in RAM is lost when program terminates
- Files allow **permanent storage** in secondary memory (HDD/SSD)
- Data persists after program ends

```cpp
// WHY: Without file handling - data lost
int main() {
    int score = 100;
    // Program ends - score lost forever
    return 0;
}

// WHY: With file handling - data preserved
int main() {
    int score = 100;
    ofstream file("score.txt");
    file << score;  // Saved to disk
    file.close();
    // Program ends - score preserved in file
    return 0;
}
```

### 1.2 File Types

| Type | Content | Use Case |
| --- | --- | --- |
| **Text Files** | Human-readable characters | Config files, logs, CSV |
| **Binary Files** | Raw binary data (0s and 1s) | Images, databases, executables |

**Text File Example:**

```
Hello World
123
3.14
```

**Binary File Example (hexadecimal view):**

```
48 65 6C 6C 6F 00 7B 00 ...
```

### 1.3 The `<fstream>` Header

```cpp
#include <fstream>

// WHY: Three main classes for file operations
// ifstream - Input File Stream (reading)
// ofstream - Output File Stream (writing)
// fstream  - File Stream (both reading and writing)
```

---

## 2. File Streams Basics

### 2.1 The Three Stream Classes

**Class Hierarchy:**

```
ios (base class)
├── istream
│   └── ifstream (input file stream)
├── ostream
│   └── ofstream (output file stream)
└── iostream
    └── fstream (input/output file stream)
```

**ifstream - Input File Stream:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: ifstream for reading files
    ifstream inputFile("data.txt");

    if (!inputFile.is_open()) {
        cerr << "Error: Cannot open file!" << endl;
        return 1;
    }

    string line;
    while (getline(inputFile, line)) {
        cout << line << endl;
    }

    inputFile.close();
    return 0;
}
```

**ofstream - Output File Stream:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: ofstream for writing files
    ofstream outputFile("output.txt");

    if (!outputFile.is_open()) {
        cerr << "Error: Cannot create file!" << endl;
        return 1;
    }

    outputFile << "Hello World!" << endl;
    outputFile << "C++ File Handling" << endl;

    outputFile.close();
    cout << "Data written to file" << endl;

    return 0;
}
```

**Expected Output:**

```
Data written to file
```

**File content (output.txt):**

```
Hello World!
C++ File Handling
```

**fstream - Both Input and Output:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: fstream for both reading and writing
    fstream file;

    // Write to file
    file.open("data.txt", ios::out);
    file << "First line" << endl;
    file << "Second line" << endl;
    file.close();

    // Read from file
    file.open("data.txt", ios::in);
    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }
    file.close();

    return 0;
}
```

**Output:**

```
First line
Second line
```

### 2.2 Opening Files

**Method 1: Constructor:**

```cpp
// WHY: Open file in constructor
ifstream file("input.txt");      // Read mode
ofstream file("output.txt");     // Write mode
fstream file("data.txt", ios::in | ios::out);  // Both modes
```

**Method 2: open() Function:**

```cpp
#include <fstream>
using namespace std;

int main() {
    ifstream file;

    // WHY: Open file explicitly
    file.open("data.txt");

    if (file.is_open()) {
        // File opened successfully
        file.close();
    }

    return 0;
}
```

### 2.3 File Open Modes

| Mode | Flag | Description |
| --- | --- | --- |
| **Input** | `ios::in` | Open for reading (default for ifstream) |
| **Output** | `ios::out` | Open for writing (default for ofstream) |
| **Append** | `ios::app` | Append to end of file |
| **At End** | `ios::ate` | Move to end after opening |
| **Truncate** | `ios::trunc` | Delete existing content |
| **Binary** | `ios::binary` | Binary mode (not text) |

**Combining Modes:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: Multiple modes with | operator

    // 1. Read and write
    fstream file1("data.txt", ios::in | ios::out);

    // 2. Write and append
    ofstream file2("log.txt", ios::out | ios::app);

    // 3. Binary write
    ofstream file3("data.bin", ios::out | ios::binary);

    // 4. Read binary
    ifstream file4("data.bin", ios::in | ios::binary);

    // 5. Truncate and write
    ofstream file5("old.txt", ios::out | ios::trunc);

    return 0;
}
```

**Mode Examples:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: ios::out - overwrites existing content
    ofstream file1("test.txt", ios::out);
    file1 << "First write" << endl;
    file1.close();

    ofstream file2("test.txt", ios::out);
    file2 << "Second write" << endl;  // Overwrites "First write"
    file2.close();

    // WHY: ios::app - appends to existing content
    ofstream file3("test.txt", ios::app);
    file3 << "Appended line" << endl;  // Adds after "Second write"
    file3.close();

    // Read final content
    ifstream file4("test.txt");
    string line;
    while (getline(file4, line)) {
        cout << line << endl;
    }
    file4.close();

    return 0;
}
```

**Output:**

```
Second write
Appended line
```

### 2.4 Closing Files

```cpp
#include <fstream>
using namespace std;

int main() {
    ofstream file("data.txt");

    // Write to file
    file << "Some data" << endl;

    // WHY: Always close files
    // 1. Flushes buffer to disk
    // 2. Releases file handle
    // 3. Allows other programs to access file
    file.close();

    // WHY: File automatically closed in destructor
    // But explicit close is better practice

    return 0;
}
```

**RAII Pattern for Files:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

void processFile() {
    // WHY: File opened in constructor
    ifstream file("data.txt");

    if (!file.is_open()) {
        throw runtime_error("Cannot open file");
    }

    // Process file...

    // WHY: File automatically closed when 'file' goes out of scope
    // Even if exception thrown - RAII ensures cleanup!
}

int main() {
    try {
        processFile();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
```

### 2.5 Checking File Status

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ifstream file("data.txt");

    // WHY: Check if file opened successfully
    if (!file.is_open()) {
        cerr << "Error: Cannot open file!" << endl;
        return 1;
    }

    // Alternative check
    if (!file) {
        cerr << "File not opened!" << endl;
        return 1;
    }

    // Another alternative
    if (file.fail()) {
        cerr << "File operation failed!" << endl;
        return 1;
    }

    file.close();
    return 0;
}
```

---

## 3. Text File Operations

### 3.1 Writing Text Files

**Using << Operator:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: Create output file stream
    ofstream file("student.txt");

    if (!file.is_open()) {
        cerr << "Error creating file!" << endl;
        return 1;
    }

    // WHY: Write data like cout
    file << "Name: John Doe" << endl;
    file << "Age: 20" << endl;
    file << "Grade: A" << endl;
    file << "GPA: 3.85" << endl;

    file.close();
    cout << "Student data written successfully!" << endl;

    return 0;
}
```

**Output:**

```
Student data written successfully!
```

**File content (student.txt):**

```
Name: John Doe
Age: 20
Grade: A
GPA: 3.85
```

**Using put() Method:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ofstream file("chars.txt");

    // WHY: put() writes single character
    file.put('H');
    file.put('e');
    file.put('l');
    file.put('l');
    file.put('o');
    file.put('\n');

    // Write string character by character
    string text = "World";
    for (char ch : text) {
        file.put(ch);
    }

    file.close();
    return 0;
}
```

**File content (chars.txt):**

```
Hello
World
```

**Formatted Output:**

```cpp
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;

int main() {
    ofstream file("formatted.txt");

    // WHY: Formatted output with manipulators
    file << left << setw(15) << "Name"
         << left << setw(10) << "Age"
         << left << setw(10) << "Salary" << endl;

    file << left << setw(15) << "Alice"
         << left << setw(10) << 25
         << left << setw(10) << fixed << setprecision(2) << 50000.50 << endl;

    file << left << setw(15) << "Bob"
         << left << setw(10) << 30
         << left << setw(10) << fixed << setprecision(2) << 60000.75 << endl;

    file.close();
    return 0;
}
```

**File content (formatted.txt):**

```
Name           Age       Salary
Alice          25        50000.50
Bob            30        60000.75
```

### 3.2 Reading Text Files

**Using >> Operator:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // First, write data
    ofstream outFile("numbers.txt");
    outFile << 10 << " " << 20 << " " << 30 << endl;
    outFile << 40 << " " << 50 << endl;
    outFile.close();

    // WHY: Read data word by word
    ifstream inFile("numbers.txt");

    int num;
    cout << "Numbers from file: ";
    while (inFile >> num) {
        cout << num << " ";
    }
    cout << endl;

    inFile.close();
    return 0;
}
```

**Output:**

```
Numbers from file: 10 20 30 40 50
```

**Using getline():**

```cpp
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    // Create file
    ofstream outFile("poem.txt");
    outFile << "Roses are red" << endl;
    outFile << "Violets are blue" << endl;
    outFile << "C++ is awesome" << endl;
    outFile << "And so are you!" << endl;
    outFile.close();

    // WHY: Read line by line with getline()
    ifstream inFile("poem.txt");

    string line;
    int lineNum = 1;
    while (getline(inFile, line)) {
        cout << lineNum++ << ": " << line << endl;
    }

    inFile.close();
    return 0;
}
```

**Output:**

```
1: Roses are red
2: Violets are blue
3: C++ is awesome
4: And so are you!
```

**Using get():**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // Create file
    ofstream outFile("text.txt");
    outFile << "Hello C++!";
    outFile.close();

    // WHY: Read character by character
    ifstream inFile("text.txt");

    char ch;
    cout << "Characters: ";
    while (inFile.get(ch)) {
        cout << ch;
    }
    cout << endl;

    inFile.close();
    return 0;
}
```

**Output:**

```
Characters: Hello C++!
```

**Reading Structured Data:**

```cpp
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

struct Student {
    string name;
    int age;
    double gpa;
};

int main() {
    // Write structured data
    ofstream outFile("students.txt");
    outFile << "Alice 20 3.8" << endl;
    outFile << "Bob 22 3.5" << endl;
    outFile << "Charlie 21 3.9" << endl;
    outFile.close();

    // WHY: Read structured data
    ifstream inFile("students.txt");

    Student student;
    cout << "Students:" << endl;
    while (inFile >> student.name >> student.age >> student.gpa) {
        cout << "Name: " << student.name
             << ", Age: " << student.age
             << ", GPA: " << student.gpa << endl;
    }

    inFile.close();
    return 0;
}
```

**Output:**

```
Students:
Name: Alice, Age: 20, GPA: 3.8
Name: Bob, Age: 22, GPA: 3.5
Name: Charlie, Age: 21, GPA: 3.9
```

### 3.3 CSV File Handling

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

// WHY: Read CSV file
void readCSV(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Cannot open file!" << endl;
        return;
    }

    string line;
    int row = 0;

    while (getline(file, line)) {
        // WHY: Use stringstream to parse CSV line
        stringstream ss(line);
        string cell;
        vector<string> rowData;

        while (getline(ss, cell, ',')) {
            rowData.push_back(cell);
        }

        cout << "Row " << row++ << ": ";
        for (const string& data : rowData) {
            cout << "[" << data << "] ";
        }
        cout << endl;
    }

    file.close();
}

int main() {
    // Create CSV file
    ofstream outFile("data.csv");
    outFile << "Name,Age,City" << endl;
    outFile << "Alice,25,New York" << endl;
    outFile << "Bob,30,Los Angeles" << endl;
    outFile << "Charlie,28,Chicago" << endl;
    outFile.close();

    // Read CSV file
    readCSV("data.csv");

    return 0;
}
```

**Output:**

```
Row 0: [Name] [Age] [City]
Row 1: [Alice] [25] [New York]
Row 2: [Bob] [30] [Los Angeles]
Row 3: [Charlie] [28] [Chicago]
```

### 3.4 File Positioning

**tellg() and tellp():**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // Create file
    ofstream outFile("position.txt");
    outFile << "0123456789";
    outFile.close();

    // WHY: tellg() - get current read position
    ifstream inFile("position.txt");

    cout << "Initial position: " << inFile.tellg() << endl;

    char ch;
    inFile.get(ch);
    cout << "After reading 1 char: " << inFile.tellg() << endl;

    inFile.get(ch);
    inFile.get(ch);
    cout << "After reading 3 chars: " << inFile.tellg() << endl;

    inFile.close();

    // WHY: tellp() - get current write position
    ofstream outFile2("position2.txt");

    cout << "\nWrite position: " << outFile2.tellp() << endl;
    outFile2 << "Hello";
    cout << "After writing 'Hello': " << outFile2.tellp() << endl;

    outFile2.close();

    return 0;
}
```

**Output:**

```
Initial position: 0
After reading 1 char: 1
After reading 3 chars: 3

Write position: 0
After writing 'Hello': 5
```

**seekg() and seekp():**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // Create file
    ofstream outFile("seek.txt");
    outFile << "ABCDEFGHIJ";
    outFile.close();

    // WHY: seekg() - move read position
    ifstream inFile("seek.txt");

    // Read from beginning
    inFile.seekg(0, ios::beg);
    char ch;
    inFile.get(ch);
    cout << "Position 0: " << ch << endl;

    // Read from position 5
    inFile.seekg(5, ios::beg);
    inFile.get(ch);
    cout << "Position 5: " << ch << endl;

    // Move 2 positions from current
    inFile.seekg(2, ios::cur);
    inFile.get(ch);
    cout << "Position 7 (5+2): " << ch << endl;

    // Read from end (-1 = last character)
    inFile.seekg(-1, ios::end);
    inFile.get(ch);
    cout << "Last character: " << ch << endl;

    inFile.close();

    return 0;
}
```

**Output:**

```
Position 0: A
Position 5: F
Position 7 (5+2): H
Last character: J
```

**Position Flags:**

| Flag | Description | Example |
| --- | --- | --- |
| `ios::beg` | Beginning of file | `seekg(5, ios::beg)` - 5 bytes from start |
| `ios::cur` | Current position | `seekg(2, ios::cur)` - 2 bytes forward |
| `ios::end` | End of file | `seekg(-3, ios::end)` - 3 bytes before end |

---

## 4. Binary File Operations

### 4.1 Binary vs Text Files

| Aspect | Text File | Binary File |
| --- | --- | --- |
| **Content** | Human-readable characters | Raw binary data (0s and 1s) |
| **Size** | Larger (ASCII encoding) | Smaller (direct binary) |
| **Readability** | Can read with text editor | Requires special viewer |
| **Speed** | Slower (conversion) | Faster (no conversion) |
| **Precision** | May lose precision | Exact representation |
| **Use Case** | Config files, logs, CSV | Images, databases, executables |

**Example - Integer Storage:**

```cpp
// Text file: "12345" (5 bytes)
// Binary file: 0x00003039 (4 bytes for int)
```

### 4.2 Writing Binary Files

**Using write():**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: Open file in binary mode
    ofstream file("data.bin", ios::out | ios::binary);

    if (!file.is_open()) {
        cerr << "Cannot create binary file!" << endl;
        return 1;
    }

    // Write integer
    int num = 12345;
    file.write(reinterpret_cast<char*>(&num), sizeof(num));

    // Write double
    double pi = 3.14159;
    file.write(reinterpret_cast<char*>(&pi), sizeof(pi));

    // Write array
    int arr[] = {10, 20, 30, 40, 50};
    file.write(reinterpret_cast<char*>(arr), sizeof(arr));

    file.close();
    cout << "Binary data written successfully!" << endl;

    return 0;
}
```

**Output:**

```
Binary data written successfully!
```

### 4.3 Reading Binary Files

**Using read():**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // First write data
    ofstream outFile("numbers.bin", ios::binary);
    int writeNum = 12345;
    double writePi = 3.14159;
    outFile.write(reinterpret_cast<char*>(&writeNum), sizeof(writeNum));
    outFile.write(reinterpret_cast<char*>(&writePi), sizeof(writePi));
    outFile.close();

    // WHY: Read binary data
    ifstream inFile("numbers.bin", ios::binary);

    if (!inFile.is_open()) {
        cerr << "Cannot open binary file!" << endl;
        return 1;
    }

    // Read integer
    int readNum;
    inFile.read(reinterpret_cast<char*>(&readNum), sizeof(readNum));

    // Read double
    double readPi;
    inFile.read(reinterpret_cast<char*>(&readPi), sizeof(readPi));

    cout << "Integer: " << readNum << endl;
    cout << "Double: " << readPi << endl;

    inFile.close();
    return 0;
}
```

**Output:**

```
Integer: 12345
Double: 3.14159
```

### 4.4 Structures in Binary Files

```cpp
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct Student {
    int id;
    char name[50];
    double gpa;
};

int main() {
    // WHY: Write structure to binary file
    ofstream outFile("students.bin", ios::binary);

    Student s1 = {101, "Alice Johnson", 3.85};
    Student s2 = {102, "Bob Smith", 3.67};
    Student s3 = {103, "Charlie Brown", 3.92};

    outFile.write(reinterpret_cast<char*>(&s1), sizeof(Student));
    outFile.write(reinterpret_cast<char*>(&s2), sizeof(Student));
    outFile.write(reinterpret_cast<char*>(&s3), sizeof(Student));

    outFile.close();
    cout << "Students written to binary file" << endl;

    // WHY: Read structures from binary file
    ifstream inFile("students.bin", ios::binary);

    Student s;
    cout << "\nReading students from file:" << endl;
    while (inFile.read(reinterpret_cast<char*>(&s), sizeof(Student))) {
        cout << "ID: " << s.id
             << ", Name: " << s.name
             << ", GPA: " << s.gpa << endl;
    }

    inFile.close();
    return 0;
}
```

**Output:**

```
Students written to binary file

Reading students from file:
ID: 101, Name: Alice Johnson, GPA: 3.85
ID: 102, Name: Bob Smith, GPA: 3.67
ID: 103, Name: Charlie Brown, GPA: 3.92
```

### 4.5 Random Access in Binary Files

```cpp
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct Record {
    int id;
    char data[40];
};

int main() {
    // Write records
    ofstream outFile("records.bin", ios::binary);

    for (int i = 0; i < 5; ++i) {
        Record r;
        r.id = i + 1;
        sprintf(r.data, "Record %d", i + 1);
        outFile.write(reinterpret_cast<char*>(&r), sizeof(Record));
    }
    outFile.close();

    // WHY: Random access - read specific record
    fstream file("records.bin", ios::in | ios::out | ios::binary);

    // Read record #3 (index 2)
    int recordNum = 2;
    file.seekg(recordNum * sizeof(Record), ios::beg);

    Record r;
    file.read(reinterpret_cast<char*>(&r), sizeof(Record));
    cout << "Record #3: ID=" << r.id << ", Data=" << r.data << endl;

    // Modify record #3
    strcpy(r.data, "Modified Record 3");
    file.seekp(recordNum * sizeof(Record), ios::beg);
    file.write(reinterpret_cast<char*>(&r), sizeof(Record));

    // Verify modification
    file.seekg(recordNum * sizeof(Record), ios::beg);
    file.read(reinterpret_cast<char*>(&r), sizeof(Record));
    cout << "After modification: ID=" << r.id << ", Data=" << r.data << endl;

    file.close();
    return 0;
}
```

**Output:**

```
Record #3: ID=3, Data=Record 3
After modification: ID=3, Data=Modified Record 3
```

---

## 5. File Error Handling

### 5.1 Stream State Flags

```cpp
#include <iostream>
#include <fstream>
using namespace std;

void checkStreamState(ios& stream, const string& streamName) {
    cout << streamName << " state:" << endl;

    // WHY: good() - all operations successful
    cout << "  good(): " << (stream.good() ? "true" : "false") << endl;

    // WHY: eof() - end of file reached
    cout << "  eof():  " << (stream.eof() ? "true" : "false") << endl;

    // WHY: fail() - logical error (format, conversion)
    cout << "  fail(): " << (stream.fail() ? "true" : "false") << endl;

    // WHY: bad() - serious error (hardware, memory)
    cout << "  bad():  " << (stream.bad() ? "true" : "false") << endl;

    cout << endl;
}

int main() {
    // Create test file
    ofstream outFile("test.txt");
    outFile << "123 456 abc";
    outFile.close();

    ifstream file("test.txt");

    // Initial state
    checkStreamState(file, "Initial");

    // Read integers successfully
    int num1, num2;
    file >> num1 >> num2;
    checkStreamState(file, "After reading 2 ints");

    // Try to read third integer (will fail - "abc" is not int)
    int num3;
    file >> num3;
    checkStreamState(file, "After reading non-int");

    // Clear error flags
    file.clear();
    checkStreamState(file, "After clear()");

    file.close();
    return 0;
}
```

**Output:**

```
Initial state:
  good(): true
  eof():  false
  fail(): false
  bad():  false

After reading 2 ints state:
  good(): true
  eof():  false
  fail(): false
  bad():  false

After reading non-int state:
  good(): false
  eof():  false
  fail(): true
  bad():  false

After clear() state:
  good(): true
  eof():  false
  fail(): false
  bad():  false
```

### 5.2 Checking File Operations

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: Check file open
    ifstream file("nonexistent.txt");

    if (!file.is_open()) {
        cerr << "Error: Cannot open file!" << endl;
        return 1;
    }

    // Alternative check
    if (!file) {
        cerr << "Error: File stream failed!" << endl;
        return 1;
    }

    // WHY: Check read operation
    int num;
    if (!(file >> num)) {
        cerr << "Error: Failed to read integer!" << endl;

        if (file.eof()) {
            cout << "Reason: End of file" << endl;
        } else if (file.fail()) {
            cout << "Reason: Invalid format" << endl;
        } else if (file.bad()) {
            cout << "Reason: Serious error" << endl;
        }

        return 1;
    }

    file.close();
    return 0;
}
```

**Output:**

```
Error: Cannot open file!
```

### 5.3 Exception-Based Error Handling

```cpp
#include <iostream>
#include <fstream>
#include <stdexcept>
using namespace std;

// WHY: Enable exceptions for file streams
void safeFileRead(const string& filename) {
    ifstream file;

    // Enable exceptions for failbit and badbit
    file.exceptions(ifstream::failbit | ifstream::badbit);

    try {
        file.open(filename);

        string line;
        while (getline(file, line)) {
            cout << line << endl;
        }

        file.close();

    } catch (const ifstream::failure& e) {
        cerr << "File exception: " << e.what() << endl;
        cerr << "Error code: " << e.code() << endl;

        if (file.is_open()) {
            file.close();
        }
    }
}

int main() {
    safeFileRead("test.txt");
    return 0;
}
```

### 5.4 RAII File Wrapper

```cpp
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

// WHY: RAII wrapper for automatic file closing
class FileGuard {
private:
    fstream file;
    string filename;

public:
    FileGuard(const string& name, ios::openmode mode)
        : filename(name) {
        file.open(filename, mode);

        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + filename);
        }

        cout << "File opened: " << filename << endl;
    }

    ~FileGuard() {
        if (file.is_open()) {
            file.close();
            cout << "File closed: " << filename << endl;
        }
    }

    // Prevent copying
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    fstream& getStream() {
        return file;
    }
};

int main() {
    try {
        // WHY: File automatically closed even if exception thrown
        FileGuard guard("data.txt", ios::out);

        guard.getStream() << "Line 1" << endl;
        guard.getStream() << "Line 2" << endl;

        // Simulate error
        throw runtime_error("Processing error");

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    // File already closed by destructor
    cout << "Program continues" << endl;

    return 0;
}
```

**Output:**

```
File opened: data.txt
File closed: data.txt
Error: Processing error
Program continues
```

---

## 6. Advanced File Operations

### 6.1 File Existence Check

```cpp
#include <iostream>
#include <fstream>
using namespace std;

bool fileExists(const string& filename) {
    // WHY: Try to open file for reading
    ifstream file(filename);
    return file.good();
}

int main() {
    string filename;
    cout << "Enter filename to check: ";
    cin >> filename;

    if (fileExists(filename)) {
        cout << "File exists!" << endl;
    } else {
        cout << "File does not exist!" << endl;
    }

    return 0;
}
```

### 6.2 File Copy Operation

```cpp
#include <iostream>
#include <fstream>
using namespace std;

bool copyFile(const string& source, const string& dest) {
    // WHY: Open source for reading
    ifstream src(source, ios::binary);
    if (!src.is_open()) {
        cerr << "Cannot open source file!" << endl;
        return false;
    }

    // WHY: Open destination for writing
    ofstream dst(dest, ios::binary);
    if (!dst.is_open()) {
        cerr << "Cannot create destination file!" << endl;
        src.close();
        return false;
    }

    // WHY: Copy byte by byte
    char byte;
    while (src.get(byte)) {
        dst.put(byte);
    }

    src.close();
    dst.close();

    return true;
}

int main() {
    if (copyFile("source.txt", "backup.txt")) {
        cout << "File copied successfully!" << endl;
    } else {
        cout << "File copy failed!" << endl;
    }

    return 0;
}
```

### 6.3 Counting Lines, Words, Characters

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

void analyzeFile(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Cannot open file!" << endl;
        return;
    }

    int lines = 0, words = 0, chars = 0;
    string line;

    while (getline(file, line)) {
        ++lines;
        chars += line.length() + 1; // +1 for newline

        // WHY: Count words using stringstream
        stringstream ss(line);
        string word;
        while (ss >> word) {
            ++words;
        }
    }

    file.close();

    cout << "Lines: " << lines << endl;
    cout << "Words: " << words << endl;
    cout << "Characters: " << chars << endl;
}

int main() {
    // Create test file
    ofstream out("test.txt");
    out << "Hello World" << endl;
    out << "C++ File Handling" << endl;
    out << "Is Awesome!" << endl;
    out.close();

    analyzeFile("test.txt");

    return 0;
}
```

**Output:**

```
Lines: 3
Words: 6
Characters: 37
```

### 6.4 Appending to File

```cpp
#include <iostream>
#include <fstream>
using namespace std;

void appendToFile(const string& filename, const string& data) {
    // WHY: Open in append mode
    ofstream file(filename, ios::app);

    if (!file.is_open()) {
        cerr << "Cannot open file for appending!" << endl;
        return;
    }

    file << data << endl;
    file.close();

    cout << "Data appended successfully!" << endl;
}

int main() {
    // Create initial file
    ofstream file("log.txt");
    file << "Log entry 1" << endl;
    file.close();

    // Append entries
    appendToFile("log.txt", "Log entry 2");
    appendToFile("log.txt", "Log entry 3");

    // Display file content
    ifstream inFile("log.txt");
    string line;
    cout << "\nLog file content:" << endl;
    while (getline(inFile, line)) {
        cout << line << endl;
    }
    inFile.close();

    return 0;
}
```

**Output:**

```
Data appended successfully!
Data appended successfully!

Log file content:
Log entry 1
Log entry 2
Log entry 3
```

---

## Summary

### Key Takeaways

1. **File Handling Preserves Data Permanently** - Unlike RAM data that's lost when program terminates, files store data in secondary memory (HDD/SSD) for persistent storage. Essential for applications that need data between runs.
2. **Three Main File Stream Classes** - `ifstream` for reading, `ofstream` for writing, `fstream` for both. Each class provides appropriate operators and methods. All defined in `<fstream>` header.
3. **File Modes Control Behavior** - `ios::in` (read), `ios::out` (write), `ios::app` (append), `ios::binary` (binary mode), `ios::trunc` (truncate). Combine modes with `|` operator for complex operations.
4. **Always Check File Operations** - Use `is_open()`, `good()`, `fail()`, `bad()`, `eof()` to verify operations. Never assume file operations succeed. Handle errors gracefully with checks or exceptions.
5. **RAII Ensures File Closure** - File streams automatically close in destructor. Even if exception thrown, file properly closed. Create custom RAII wrappers for additional safety and logging.
6. **Text vs Binary Files Trade-offs** - Text files: human-readable, larger size, platform-dependent newlines. Binary files: compact, exact representation, faster, platform-independent. Choose based on use case.
7. **File Positioning for Random Access** - `tellg()`/`tellp()` get position, `seekg()`/`seekp()` set position. Use with `ios::beg`, `ios::cur`, `ios::end`. Essential for database-like operations and large files.
8. **Binary Files Use reinterpret_cast** - `write()` and `read()` require `reinterpret_cast<char*>()` for type conversion. Write structures directly to binary files. Perfect for serialization and deserialization.
9. **Stream State Flags for Error Detection** - `good()` (all OK), `eof()` (end reached), `fail()` (format error), `bad()` (hardware error). Clear flags with `clear()` before retrying operations.
10. **Real-world Patterns Essential** - File existence checks, copy operations, CSV parsing, log appending, line counting. Master these patterns for professional development. Always handle edge cases and errors.

---