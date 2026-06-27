# 9.2. File Handling - Interview Questions

---

## Table of Contents

1. Interview Question 1: File Streams & Modes
2. Interview Question 2: Text vs Binary Files
3. Interview Question 3: Error Handling & RAII

**Note:** This is Part 9.2 of File Handling. See Part 9.1 for File I/O basics, text/binary operations, and error handling fundamentals.

---

### Q1: Explain the difference between ifstream, ofstream, and fstream. When should you use each? What are file modes and how do you combine them?

**Answer:**

C++ provides three file stream classes in `<fstream>` header for file operations:

**1. ifstream (Input File Stream):**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: ifstream for reading files
    ifstream file("input.txt");

    if (!file.is_open()) {
        cerr << "Cannot open file!" << endl;
        return 1;
    }

    // Read operations
    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }

    // WHY: Default mode is ios::in
    // Equivalent to: ifstream file("input.txt", ios::in);

    file.close();
    return 0;
}
```

**Characteristics:**

- Used exclusively for reading
- Default mode: `ios::in`
- Cannot write to file
- File must exist (doesn't create new file)
- Inherits from `istream`

**2. ofstream (Output File Stream):**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: ofstream for writing files
    ofstream file("output.txt");

    if (!file.is_open()) {
        cerr << "Cannot create file!" << endl;
        return 1;
    }

    // Write operations
    file << "Hello World!" << endl;
    file << "Line 2" << endl;

    // WHY: Default mode is ios::out | ios::trunc
    // If file exists, content erased
    // If file doesn't exist, created

    file.close();
    return 0;
}
```

**Characteristics:**

- Used exclusively for writing
- Default mode: `ios::out` (with truncate)
- Cannot read from file
- Creates file if doesn't exist
- Overwrites existing content by default
- Inherits from `ostream`

**3. fstream (File Stream):**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // WHY: fstream for both reading and writing
    fstream file("data.txt", ios::in | ios::out);

    if (!file.is_open()) {
        cerr << "Cannot open file!" << endl;
        return 1;
    }

    // Write operation
    file << "Data written" << endl;

    // WHY: Move to beginning to read
    file.seekg(0, ios::beg);

    // Read operation
    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }

    file.close();
    return 0;
}
```

**Characteristics:**

- Used for both reading and writing
- No default mode (must specify)
- Most flexible
- Inherits from `iostream`

**Comparison Table:**

| Feature | ifstream | ofstream | fstream |
| --- | --- | --- | --- |
| **Read** | ✅ Yes | ❌ No | ✅ Yes |
| **Write** | ❌ No | ✅ Yes | ✅ Yes |
| **Default Mode** | ios::in | ios::out + trunc | None (must specify) |
| **Creates File** | ❌ No | ✅ Yes | Depends on mode |
| **Use Case** | Read config, logs | Write output, reports | Read/modify databases |

**File Modes:**

```cpp
#include <fstream>
using namespace std;

int main() {
    // WHY: Different file modes

    // 1. ios::in - Input (reading)
    ifstream f1("file.txt", ios::in);

    // 2. ios::out - Output (writing, overwrites)
    ofstream f2("file.txt", ios::out);

    // 3. ios::app - Append (add to end)
    ofstream f3("log.txt", ios::app);

    // 4. ios::ate - At End (move to end after open)
    fstream f4("file.txt", ios::ate);

    // 5. ios::trunc - Truncate (delete existing content)
    ofstream f5("file.txt", ios::out | ios::trunc);

    // 6. ios::binary - Binary mode
    ofstream f6("data.bin", ios::out | ios::binary);

    return 0;
}
```

**Mode Descriptions:**

| Mode | Description | Effect |
| --- | --- | --- |
| `ios::in` | Input | Open for reading |
| `ios::out` | Output | Open for writing (truncates by default) |
| `ios::app` | Append | All writes go to end |
| `ios::ate` | At End | Seek to end after opening |
| `ios::trunc` | Truncate | Delete existing content |
| `ios::binary` | Binary | No text translation |

**Combining Modes with | Operator:**

```cpp
#include <fstream>
using namespace std;

int main() {
    // WHY: Combine modes for specific behavior

    // 1. Read and write (doesn't truncate)
    fstream f1("data.txt", ios::in | ios::out);

    // 2. Write and append
    ofstream f2("log.txt", ios::out | ios::app);

    // 3. Binary write
    ofstream f3("data.bin", ios::out | ios::binary);

    // 4. Binary read
    ifstream f4("data.bin", ios::in | ios::binary);

    // 5. Read, write, and binary
    fstream f5("db.bin", ios::in | ios::out | ios::binary);

    // 6. Write, append, and binary
    ofstream f6("log.bin", ios::out | ios::app | ios::binary);

    // 7. Create or truncate
    ofstream f7("new.txt", ios::out | ios::trunc);

    return 0;
}
```

**When to Use Each:**

**Use ifstream when:**

- Reading configuration files
- Reading log files for analysis
- Importing data from text/CSV files
- No write operations needed

**Use ofstream when:**

- Creating new files
- Writing logs
- Generating reports
- Exporting data
- No read operations needed

**Use fstream when:**

- Need both read and write
- Updating existing files
- Database-like operations
- Random access files
- File modification needed

**Best Practices:**

1. Choose most restrictive stream (if only reading, use `ifstream`)
2. Always check `is_open()` after opening
3. Close files explicitly or use RAII
4. Use `ios::binary` for binary data
5. Combine modes carefully (understand interactions)

---

### Q2: Compare text files vs binary files. When should you use each? How do you read and write structures to binary files? What are the advantages and disadvantages?

**Answer:**

**Text Files vs Binary Files - Fundamental Difference:**

**Text Files:**

- Store data as human-readable characters
- Use ASCII/UTF-8 encoding
- Newlines platform-dependent (`\n` vs `\r\n`)
- Numbers stored as character sequences

**Binary Files:**

- Store data in raw binary format (0s and 1s)
- Direct memory representation
- No character encoding
- Exact byte-for-byte storage

**Example - Storing Integer 12345:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    int num = 12345;

    // WHY: Text file - stores as "12345" (5 bytes)
    ofstream textFile("num_text.txt");
    textFile << num;  // Converts to string "12345"
    textFile.close();

    // WHY: Binary file - stores as binary (4 bytes for int)
    ofstream binFile("num_bin.bin", ios::binary);
    binFile.write(reinterpret_cast<char*>(&num), sizeof(num));
    binFile.close();

    // Verify sizes
    ifstream tf("num_text.txt");
    tf.seekg(0, ios::end);
    cout << "Text file size: " << tf.tellg() << " bytes" << endl;

    ifstream bf("num_bin.bin", ios::binary);
    bf.seekg(0, ios::end);
    cout << "Binary file size: " << bf.tellg() << " bytes" << endl;

    return 0;
}
```

**Output:**

```
Text file size: 5 bytes
Binary file size: 4 bytes
```

**Comprehensive Comparison:**

| Aspect | Text Files | Binary Files |
| --- | --- | --- |
| **Readability** | Human-readable | Not human-readable |
| **Size** | Larger (encoding overhead) | Smaller (raw data) |
| **Speed** | Slower (conversion) | Faster (no conversion) |
| **Precision** | May lose precision (floats) | Exact representation |
| **Portability** | More portable | Platform-dependent (endianness) |
| **Editing** | Any text editor | Hex editor required |
| **Newlines** | Platform-dependent | Not applicable |
| **Whitespace** | Significant | Not applicable |

**When to Use Text Files:**

```cpp
// 1. Configuration files
ofstream config("config.txt");
config << "port=8080" << endl;
config << "host=localhost" << endl;
config << "debug=true" << endl;

// 2. Log files
ofstream log("app.log", ios::app);
log << "2024-01-01 10:30:00 - User logged in" << endl;

// 3. CSV files
ofstream csv("data.csv");
csv << "Name,Age,City" << endl;
csv << "Alice,25,NY" << endl;

// 4. Human-readable reports
ofstream report("report.txt");
report << "Sales Report" << endl;
report << "Total: $1234.56" << endl;
```

**When to Use Binary Files:**

```cpp
// 1. Images, audio, video
ofstream image("photo.jpg", ios::binary);
// Write binary image data

// 2. Databases
ofstream db("records.db", ios::binary);
// Write binary records

// 3. Serialized objects
ofstream save("game.sav", ios::binary);
// Write game state

// 4. Compressed data
ofstream archive("data.zip", ios::binary);
// Write compressed data
```

**Writing Structures to Binary Files:**

```cpp
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct Employee {
    int id;
    char name[50];
    double salary;
    bool active;
};

int main() {
    // WHY: Write structure to binary file
    ofstream outFile("employees.bin", ios::binary);

    if (!outFile.is_open()) {
        cerr << "Cannot create file!" << endl;
        return 1;
    }

    // Create employees
    Employee e1 = {101, "Alice Johnson", 75000.50, true};
    Employee e2 = {102, "Bob Smith", 82000.75, true};
    Employee e3 = {103, "Charlie Brown", 68000.00, false};

    // WHY: Write each structure
    outFile.write(reinterpret_cast<char*>(&e1), sizeof(Employee));
    outFile.write(reinterpret_cast<char*>(&e2), sizeof(Employee));
    outFile.write(reinterpret_cast<char*>(&e3), sizeof(Employee));

    outFile.close();
    cout << "Employees written to binary file" << endl;

    // WHY: Read structures from binary file
    ifstream inFile("employees.bin", ios::binary);

    if (!inFile.is_open()) {
        cerr << "Cannot open file!" << endl;
        return 1;
    }

    Employee emp;
    cout << "\nReading employees:" << endl;
    while (inFile.read(reinterpret_cast<char*>(&emp), sizeof(Employee))) {
        cout << "ID: " << emp.id
             << ", Name: " << emp.name
             << ", Salary: $" << emp.salary
             << ", Active: " << (emp.active ? "Yes" : "No") << endl;
    }

    inFile.close();
    return 0;
}
```

**Output:**

```
Employees written to binary file

Reading employees:
ID: 101, Name: Alice Johnson, Salary: $75000.5, Active: Yes
ID: 102, Name: Bob Smith, Salary: $82000.8, Active: Yes
ID: 103, Name: Charlie Brown, Salary: $68000, Active: No
```

**Important Considerations for Binary Structures:**

```cpp
// ❌ BAD: Structure with pointers
struct BadStruct {
    int id;
    char* name;  // Pointer - only address written!
    double value;
};

// ✅ GOOD: Structure with fixed-size arrays
struct GoodStruct {
    int id;
    char name[50];  // Fixed array - actual data written
    double value;
};

// ❌ BAD: Structure with std::string
struct BadStruct2 {
    int id;
    string name;  // Complex object - can't write directly
    double value;
};
```

**Random Access with Structures:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

struct Record {
    int id;
    char data[40];
};

void updateRecord(const string& filename, int index, const Record& newData) {
    // WHY: Random access to update specific record
    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open()) {
        cerr << "Cannot open file!" << endl;
        return;
    }

    // Move to specific record
    file.seekp(index * sizeof(Record), ios::beg);

    // Write new data
    file.write(reinterpret_cast<const char*>(&newData), sizeof(Record));

    file.close();
    cout << "Record " << index << " updated" << endl;
}

Record readRecord(const string& filename, int index) {
    // WHY: Read specific record
    ifstream file(filename, ios::binary);

    Record r;
    file.seekg(index * sizeof(Record), ios::beg);
    file.read(reinterpret_cast<char*>(&r), sizeof(Record));

    file.close();
    return r;
}
```

**Advantages and Disadvantages:**

**Text Files:**

✅ **Advantages:**

- Human-readable (easy debugging)
- Can edit with any text editor
- Platform-independent (mostly)
- Good for configuration
- Easy to version control
- Interoperable (many languages can read)

❌ **Disadvantages:**

- Larger file size
- Slower (conversion overhead)
- May lose precision (floats)
- Parsing required
- Whitespace issues

**Binary Files:**

✅ **Advantages:**

- Compact (smaller size)
- Fast (no conversion)
- Exact representation
- Efficient for large data
- Random access efficient
- No parsing overhead

❌ **Disadvantages:**

- Not human-readable
- Platform-dependent (endianness)
- Requires knowledge of structure
- Version compatibility issues
- Harder to debug
- Special tools needed to view

**Decision Matrix:**

| Use Case | Best Choice | Reason |
| --- | --- | --- |
| Configuration files | Text | Human-editable |
| Log files | Text | Human-readable |
| CSV data | Text | Standard format |
| Images/Audio | Binary | Raw data |
| Database records | Binary | Performance |
| Game saves | Binary | Compact |
| Serialization | Binary | Exact data |
| API responses | Text (JSON) | Interoperability |

---

### Q3: How do you handle file errors in C++? Explain stream state flags (good, eof, fail, bad). What is exception-based file handling? How does RAII help with file safety?

**Answer:**

File operations can fail for many reasons - file not found, permission denied, disk full, hardware errors. Proper error handling is essential for robust applications.

**Stream State Flags:**

Every file stream maintains four state flags:

| Flag | Method | Description | Common Causes |
| --- | --- | --- | --- |
| **goodbit** | `good()` | All OK, no errors | Normal state |
| **eofbit** | `eof()` | End of file reached | Read past end |
| **failbit** | `fail()` | Logical error | Wrong format, conversion failed |
| **badbit** | `bad()` | Serious error | Hardware, memory issues |

**Complete Example:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

void printStreamState(ios& stream) {
    cout << "Stream state:" << endl;
    cout << "  good(): " << stream.good() << " (all OK)" << endl;
    cout << "  eof():  " << stream.eof() << " (end of file)" << endl;
    cout << "  fail(): " << stream.fail() << " (logical error)" << endl;
    cout << "  bad():  " << stream.bad() << " (serious error)" << endl;
    cout << endl;
}

int main() {
    // Create test file
    ofstream out("test.txt");
    out << "123 456 abc 789";
    out.close();

    ifstream file("test.txt");

    // 1. Initial state - all OK
    cout << "=== Initial State ===" << endl;
    printStreamState(file);

    // 2. Read successfully
    int num1, num2;
    file >> num1 >> num2;
    cout << "=== After Reading 2 Integers ===" << endl;
    cout << "Read: " << num1 << ", " << num2 << endl;
    printStreamState(file);

    // 3. Try to read "abc" as integer - fail!
    int num3;
    file >> num3;
    cout << "=== After Reading Non-Integer ===" << endl;
    printStreamState(file);

    // 4. Clear error state
    file.clear();
    cout << "=== After clear() ===" << endl;
    printStreamState(file);

    // 5. Skip bad input and continue
    string dummy;
    file >> dummy;  // Read "abc"
    file >> num3;   // Read 789
    cout << "After skipping: num3 = " << num3 << endl;

    // 6. Read past end of file
    int num4;
    file >> num4;
    cout << "=== After Reading Past EOF ===" << endl;
    printStreamState(file);

    file.close();
    return 0;
}
```

**Output:**

```
=== Initial State ===
Stream state:
  good(): 1 (all OK)
  eof():  0 (end of file)
  fail(): 0 (logical error)
  bad():  0 (serious error)

=== After Reading 2 Integers ===
Read: 123, 456
Stream state:
  good(): 1 (all OK)
  eof():  0 (end of file)
  fail(): 0 (logical error)
  bad():  0 (serious error)

=== After Reading Non-Integer ===
Stream state:
  good(): 0 (all OK)
  eof():  0 (end of file)
  fail(): 1 (logical error)
  bad():  0 (serious error)

=== After clear() ===
Stream state:
  good(): 1 (all OK)
  eof():  0 (end of file)
  fail(): 0 (logical error)
  bad():  0 (serious error)

After skipping: num3 = 789
=== After Reading Past EOF ===
Stream state:
  good(): 0 (all OK)
  eof():  1 (end of file)
  fail(): 1 (logical error)
  bad():  0 (serious error)
```

**Error Checking Patterns:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    // Pattern 1: Check file open
    ifstream file("data.txt");
    if (!file.is_open()) {
        cerr << "Error: Cannot open file!" << endl;
        return 1;
    }

    // Pattern 2: Check read operation
    int num;
    if (!(file >> num)) {
        cerr << "Error: Failed to read integer!" << endl;

        // Determine specific error
        if (file.eof()) {
            cout << "Reason: End of file" << endl;
        } else if (file.fail()) {
            cout << "Reason: Format error" << endl;
        } else if (file.bad()) {
            cout << "Reason: Hardware error" << endl;
        }

        return 1;
    }

    // Pattern 3: Loop with error checking
    string line;
    while (getline(file, line)) {
        // Process line
        cout << line << endl;
    }

    // WHY: Check WHY loop ended
    if (file.bad()) {
        cerr << "Error: I/O error while reading" << endl;
    } else if (!file.eof()) {
        cerr << "Error: Format error" << endl;
    }
    // file.eof() == true means normal end

    file.close();
    return 0;
}
```

**Exception-Based File Handling:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ifstream file;

    // WHY: Enable exceptions for file stream
    file.exceptions(ifstream::failbit | ifstream::badbit);

    try {
        // Open file
        file.open("data.txt");

        // Read data
        int num;
        while (file >> num) {
            cout << num << " ";
        }
        cout << endl;

        file.close();

    } catch (const ifstream::failure& e) {
        // WHY: Catch file exceptions
        cerr << "File exception: " << e.what() << endl;
        cerr << "Error code: " << e.code() << endl;

        // Check which error occurred
        if (file.is_open()) {
            if (file.eof()) {
                cout << "Normal: End of file reached" << endl;
            } else if (file.fail()) {
                cerr << "Error: Read/write operation failed" << endl;
            } else if (file.bad()) {
                cerr << "Error: Serious I/O error" << endl;
            }

            file.close();
        } else {
            cerr << "Error: File not opened" << endl;
        }
    }

    return 0;
}
```

**RAII for File Safety:**

**Without RAII - Manual Cleanup:**

```cpp
// ❌ BAD: Manual cleanup in multiple paths
void processFile() {
    ifstream file("data.txt");

    if (!file.is_open()) {
        return;  // No cleanup needed yet
    }

    try {
        // Process file...
        if (error1) {
            file.close();  // Manual cleanup
            throw exception();
        }

        // More processing...
        if (error2) {
            file.close();  // Manual cleanup again!
            throw exception();
        }

        file.close();  // Manual cleanup

    } catch (...) {
        file.close();  // Manual cleanup in catch too!
        throw;
    }
}
```

**With RAII - Automatic Cleanup:**

```cpp
#include <iostream>
#include <fstream>
#include <stdexcept>
using namespace std;

// ✅ GOOD: RAII wrapper
class FileGuard {
private:
    fstream file;
    string filename;
    bool opened;

public:
    FileGuard(const string& name, ios::openmode mode)
        : filename(name), opened(false) {

        file.open(filename, mode);

        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + filename);
        }

        opened = true;
        cout << "[RAII] File opened: " << filename << endl;
    }

    ~FileGuard() {
        // WHY: Automatically called even if exception thrown
        if (opened && file.is_open()) {
            file.close();
            cout << "[RAII] File closed: " << filename << endl;
        }
    }

    // Prevent copying
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    fstream& getStream() {
        return file;
    }
};

void processFile() {
    // WHY: File automatically closed when guard goes out of scope
    FileGuard guard("data.txt", ios::out);

    guard.getStream() << "Line 1" << endl;
    guard.getStream() << "Line 2" << endl;

    // Simulate error
    throw runtime_error("Processing failed");

    // File automatically closed by destructor!
}

int main() {
    try {
        processFile();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    cout << "Program continues" << endl;
    return 0;
}
```

**Output:**

```
[RAII] File opened: data.txt
[RAII] File closed: data.txt
Error: Processing failed
Program continues
```

**Complete Error Handling Example:**

```cpp
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

class SafeFileReader {
public:
    static vector<string> readLines(const string& filename) {
        vector<string> lines;
        ifstream file;

        // Enable exceptions
        file.exceptions(ifstream::badbit);

        try {
            file.open(filename);

            // Check if opened
            if (!file.is_open()) {
                throw runtime_error("Cannot open file: " + filename);
            }

            string line;
            while (getline(file, line)) {
                lines.push_back(line);
            }

            // Check why loop ended
            if (file.bad()) {
                throw runtime_error("I/O error while reading");
            }
            // file.eof() is normal

            file.close();

        } catch (const exception& e) {
            if (file.is_open()) {
                file.close();
            }
            throw;  // Re-throw
        }

        return lines;
    }
};

int main() {
    try {
        vector<string> lines = SafeFileReader::readLines("data.txt");

        cout << "Read " << lines.size() << " lines:" << endl;
        for (const string& line : lines) {
            cout << line << endl;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
```

**Key Best Practices:**

1. **Always check file operations** - Never assume they succeed
2. **Use RAII** - Automatic cleanup prevents leaks
3. **Enable exceptions for critical operations** - Simplifies error handling
4. **Check specific error flags** - `eof()`, `fail()`, `bad()` for diagnosis
5. **Clear error state before retry** - Use `clear()` method
6. **Close files explicitly** - Even though destructor does it
7. **Provide meaningful error messages** - Include filename and operation

---