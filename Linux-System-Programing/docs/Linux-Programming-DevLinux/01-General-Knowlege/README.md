# 1. GENERAL KNOWLEDGE

## 1.1. Introduction to Makefile

### 1.1.1. What is Makefile?

A Makefile is a powerful script used by the `make` build automation tool to compile and manage projects efficiently. It contains:

- **Project structure information**: Files and their dependencies
- **Build commands**: Instructions for compiling, linking, and cleaning up files
- **Rules**: Definitions for how to generate target files

The `make` program reads the Makefile, intelligently determines which files need to be updated based on modification times, and executes only the necessary commands to build the project.

**Naming Conventions:**
- `Makefile` (recommended, as `make` searches for this by default)
- `makefile` (works but less common)
- `*.mk` (used for modular or included Makefiles)

<p align="center">
  <img src="https://github.com/user-attachments/assets/a6448848-a377-4f52-8ab4-a7540e31de65" alt="Makefile Structure" width="70%">
</p>

### 1.1.2. How to run make?

The `make` command executes **shell commands** to update one or more **target files** (typically programs or executables):

```bash
# Basic usage
make

# Specify a different Makefile
make -f custom_makefile

# Build a specific target
make target_name
```

Key behaviors:
- If no `-f` option is provided, `make` automatically searches for a file named `Makefile` or `makefile` in the current directory
- It checks **dependencies** and executes commands **only if target files are outdated** (older than their dependencies)
- This dependency-based approach optimizes the build process by avoiding unnecessary recompilation

<p align="center">
  <img src="https://github.com/user-attachments/assets/68a634ea-35e1-4c89-a6cf-dcea405c3e70" alt="Make Command" width="70%">
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/d7f86721-6a11-495b-9564-ff9a3177135d" alt="Make Dependencies" width="70%">
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/2e931dbf-b370-476a-a428-7f65927efdc4" alt="Make Process" width="70%">
</p>

For more detailed information, refer to the manual: `man make`

### 1.1.3. Variables in Makefile

#### Why Use Variables?

- **Generalization**: Variables help avoid hardcoding specific file names
- **Maintainability**: Makes the Makefile easier to modify and maintain
- **Consistency**: Ensures consistent values throughout the Makefile

To access a variable's value, use the syntax: `$(VARIABLE_NAME)` or `${VARIABLE_NAME}`

**Example of defining and using variables:**
```makefile
# Define variables
CC = gcc
CFLAGS = -Wall -g
SOURCES = main.c helper.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = myprogram

# Use variables in rules
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Print variable values (useful for debugging)
print:
	@echo "Compiler: $(CC)"
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Object files: $(OBJECTS)"
```

#### Common Compiler & Linker Flags

```makefile
# Compiler flags (for gcc)
CFLAGS += -I./include  # Add include/ to header search path
CFLAGS += -DDEBUG      # Define preprocessor macro DEBUG
CFLAGS += -Wall        # Enable all important warnings
CFLAGS += -Werror      # Treat warnings as errors
CFLAGS += -g           # Generate debugging symbols for gdb
CFLAGS += -O2          # Optimize code (level 2)

# Linker flags
LDFLAGS += -L./lib     # Add lib/ to library search path
LDFLAGS += -lm         # Link with math library (libm.so)
LDFLAGS += -lpthread   # Link with POSIX threads library
```

#### Automatic Variables

Automatic variables are special predefined variables that reference parts of the rule dynamically:

| Variable | Description |
|----------|-------------|
| `$@` | The target of the current rule |
| `$<` | The first prerequisite (dependency) |
| `$^` | All prerequisites (space-separated) |
| `$*` | The stem with which the pattern matched |
| `$(@D)` | The directory part of the target |
| `$(@F)` | The file part of the target |

**Example using automatic variables:**
```makefile
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
```

### 1.1.4. Makefile Functions

Makefile functions are built-in utilities that process strings, manipulate files, and execute shell commands. They make Makefiles more dynamic and reusable by avoiding hardcoded values.

**Syntax**: `$(function arguments)`

**Most Common Makefile Functions:**

| Function | Description | Example |
|----------|-------------|---------|
| `$(wildcard pattern)` | Finds files matching a pattern | `SOURCES = $(wildcard src/*.c)` |
| `$(patsubst pattern,replacement,text)` | Replaces pattern with replacement | `OBJECTS = $(patsubst %.c,%.o,$(SOURCES))` |
| `$(shell command)` | Executes shell command and returns output | `KERNEL = $(shell uname -s)` |
| `$(foreach var,list,text)` | Iterates over list and expands text | `FILES = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))` |
| `$(if condition,then-part,else-part)` | Conditional evaluation | `COMP = $(if $(DEBUG),gcc -g,gcc -O2)` |
| `$(word n,text)` | Returns the nth word of text | `FIRST_SOURCE = $(word 1,$(SOURCES))` |
| `$(filter pattern,text)` | Keeps elements matching pattern | `C_FILES = $(filter %.c,$(FILES))` |
| `$(filter-out pattern,text)` | Removes elements matching pattern | `NON_C_FILES = $(filter-out %.c,$(FILES))` |


### 1.1.5. Phony Targets

"Phony" targets are targets that don't represent actual files but are used to execute commands. They're useful for operations like cleaning build files, running tests, or installing programs.

```makefile
.PHONY: clean all test install

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET) --test

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
```

By marking targets as `.PHONY`, `make` will always execute the commands regardless of whether a file with the target name exists or when it was last modified.

### 1.1.6. Complete Makefile Example

Here's a practical Makefile for a medium-sized C project with a clear directory structure:

```makefile
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lm

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/program

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Clean up build files
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run the program
.PHONY: run
run: all
	./$(TARGET)

# Generate documentation
.PHONY: docs
docs:
	doxygen Doxyfile
```

This example shows how to:
- Organize files in structured directories
- Automatically find source files using `wildcard`
- Transform file paths using `patsubst`
- Create build directories automatically
- Define multiple phony targets for different tasks
- Use automatic variables for concise rule definitions

--- 

## 1.2. The Process of Compiling a C Program

Compiling a C program involves four main stages: preprocessing, compilation, assembly, and linking. Understanding these stages is crucial for effective debugging and optimization.

### 1.2.1. Preprocessing

The preprocessor performs text manipulations on the source code before actual compilation:

- **Removes comments** from the source code
- **Expands macros** defined by `#define` directives
- **Processes conditional compilation** (`#ifdef`, `#ifndef`, `#endif`, etc.)
- **Includes header files** by replacing `#include` directives with the actual contents
- **Handles pragmas** that provide instructions to the compiler

**Command:**
```sh
gcc -E source.c -o source.i
```

**Example of preprocessed code:**
```c
// Original code
#include <stdio.h>
#define MAX 100
// This is a comment
int main() {
    printf("Max value: %d\n", MAX);
    return 0;
}

// After preprocessing (simplified)
// stdio.h contents would be here
int main() {
    printf("Max value: %d\n", 100);
    return 0;
}
```

### 1.2.2. Compilation

This stage translates the preprocessed C code into assembly language specific to the target processor architecture:

- Performs **lexical analysis** (tokenization)
- Conducts **syntax analysis** (parsing)
- Executes **semantic analysis** (type checking)
- Performs various **optimizations**
- Generates assembly code with processor-specific instructions

**Command:**
```sh
gcc -S source.i -o source.s
```

**Example of generated assembly (x86):**
```asm
    .file   "source.c"
    .section    .rodata
.LC0:
    .string "Max value: %d\n"
    .text
    .globl  main
    .type   main, @function
main:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    $100, %esi
    movl    $.LC0, %edi
    movl    $0, %eax
    call    printf
    movl    $0, %eax
    popq    %rbp
    ret
```

### 1.2.3. Assembly

The assembly stage converts the human-readable assembly code into machine code (object file):

- Translates assembly instructions into binary machine code
- Produces an object file containing machine instructions
- Creates a symbol table for functions and variables
- Object files contain **relocatable code** (final addresses not yet determined)

**Command:**
```sh
gcc -c source.s -o source.o
```

The output object file (`.o`) is binary and contains machine code, symbol information, and relocation entries.

### 1.2.4. Linking

The linking stage combines object files with libraries to create the final executable:

- Resolves **external references** between object files
- Incorporates code from libraries (static or shared)
- Assigns final memory addresses to functions and variables
- Creates the executable file format (ELF on Linux, PE on Windows)

**Command:**
```sh
gcc source.o -o program
```

**Types of linking:**
- **Static linking**: Library code is copied into the executable
- **Dynamic linking**: References to shared libraries are created, with actual linking at runtime

### 1.2.5. Practical Debugging with GCC Compilation Flags

Understanding the compilation process helps with effective debugging. Here's how to use GCC flags at each stage:

```sh
# Generate preprocessed output for debugging macros
gcc -E -dD source.c -o source.i

# Generate assembly with annotations for optimization review
gcc -S -fverbose-asm source.c -o source.s

# Generate object file with debug symbols
gcc -c -g source.c -o source.o

# Link with all warnings and debugging enabled
gcc -Wall -Wextra -g source.o -o program
```

When debugging complex build issues, it's helpful to examine the output of each stage:

1. Check preprocessor output to verify macro expansion and include path issues
2. Review assembly code to understand compiler optimizations
3. Use tools like `objdump` to examine object file symbols

---

## 1.3. Static and Shared Libraries in Linux

### 1.3.1. What is a Library?

A **library** is a collection of precompiled code that can be reused across multiple programs. Libraries promote:

- **Code reusability**: Write once, use many times
- **Modularity**: Organize code into logical components
- **Maintenance**: Update functionality in one place
- **Collaboration**: Share common functionality between developers

Libraries come in two main types:
- **Static Libraries** (`.a` on Linux, `.lib` on Windows)
- **Shared/Dynamic Libraries** (`.so` on Linux, `.dll` on Windows)

<p align="center">
  <img src="https://github.com/user-attachments/assets/11854185-9112-456a-b5a3-11c94d443265" alt="Libraries" width="70%">
</p>

### 1.3.2. Static Libraries

#### Definition
A **static library** is an archive of object files (`.o`) that are linked directly into an executable during the compilation phase.

#### How Static Libraries Work

1. Library code is compiled into object files (`.o`)
2. Object files are bundled into an archive (`.a`)
3. At **build time**, the linker extracts needed object files from the archive
4. The extracted code is directly incorporated into the final executable

#### Advantages of Static Libraries

✅ **Self-contained executables**: No external library dependencies at runtime  
✅ **Predictable execution**: No version conflicts with shared libraries  
✅ **Better performance**: No runtime linking overhead  
✅ **Easier deployment**: Single executable file to distribute  

#### Disadvantages of Static Libraries

❌ **Larger executables**: Each program contains copies of library code  
❌ **Memory inefficiency**: Multiple running programs duplicate code in memory  
❌ **Update challenges**: Requires recompilation of all applications when library changes  
❌ **Maintenance overhead**: Each application needs updating separately  

#### Creating and Using a Static Library

**Step 1: Create object files from source code**
```sh
gcc -c math_functions.c -o math_functions.o
gcc -c string_functions.c -o string_functions.o
```

**Step 2: Bundle object files into a static library**
```sh
ar rcs libutils.a math_functions.o string_functions.o
```
*Options explained: `r` (insert with replacement), `c` (create if not exist), `s` (create/update index)*

**Step 3: Link your program with the static library**
```sh
gcc main.c -o program -L. -lutils
```
*Options explained: `-L.` (look for libraries in current directory), `-lutils` (link with libutils.a)*

#### Example: Static Library Project

Here's a complete example of creating and using a static math library:

Directory structure:
```
libmath_project/
├── include/
│   └── math_utils.h
├── src/
│   ├── add.c
│   ├── subtract.c
│   ├── multiply.c
│   └── divide.c
├── examples/
│   └── calculator.c
└── Makefile
```

Makefile for the static library:
```makefile
# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -I./include
AR = ar
ARFLAGS = rcs

# Directories
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
INCLUDE_DIR = include

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB_NAME = libmathutils.a
LIB = $(LIB_DIR)/$(LIB_NAME)

# Default target
all: directories $(LIB)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Build example program
example: $(LIB) examples/calculator.c
	$(CC) $(CFLAGS) examples/calculator.c -o calculator -L$(LIB_DIR) -lmathutils

# Clean up build files
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) calculator
```

### 1.3.3. Shared Libraries

#### Definition
A **shared library** (also called dynamic library) contains code that is loaded into memory at runtime by multiple programs.

#### How Shared Libraries Work

1. Library code is compiled as Position-Independent Code (PIC)
2. At **build time**, the linker only includes references to the library
3. At **runtime**, the dynamic linker loads the library into memory
4. Multiple programs can share the same copy of the library in memory

#### Advantages of Shared Libraries

✅ **Smaller executables**: Only references to library functions are included  
✅ **Memory efficiency**: One copy of the library shared by multiple programs  
✅ **Dynamic updates**: Library can be updated without recompiling programs  
✅ **Plugin architecture**: Applications can load modules at runtime  

#### Disadvantages of Shared Libraries

❌ **Dependency requirements**: Libraries must be available on the target system  
❌ **Version compatibility issues**: Changes in libraries can break applications  
❌ **Slightly slower startup**: Time needed to load and link at runtime  
❌ **More complex deployment**: Need to ensure correct libraries are installed  

#### Creating and Using a Shared Library

**Step 1: Compile source files as Position-Independent Code (PIC)**
```sh
gcc -fPIC -c math_functions.c -o math_functions.o
gcc -fPIC -c string_functions.c -o string_functions.o
```

**Step 2: Create the shared library**
```sh
gcc -shared -o libutils.so math_functions.o string_functions.o
```

**Step 3: Link your program with the shared library**
```sh
gcc main.c -o program -L. -lutils
```

**Step 4: Ensure the library can be found at runtime**
```sh
# Option 1: Add library directory to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

# Option 2: Install in standard location
sudo cp libutils.so /usr/local/lib/
sudo ldconfig

# Option 3: Set RPATH during linking
gcc main.c -o program -L. -lutils -Wl,-rpath=.
```

#### Example: Shared Library Project

Here's a complete example of creating and using a shared math library:

Makefile for the shared library:
```makefile
# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I./include
LDFLAGS = -shared

# Directories
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
INCLUDE_DIR = include

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB_NAME = libmathutils.so
LIB = $(LIB_DIR)/$(LIB_NAME)

# Default target
all: directories $(LIB)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create shared library
$(LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Build example program with RPATH set
example: $(LIB) examples/calculator.c
	$(CC) $(CFLAGS) examples/calculator.c -o calculator -L$(LIB_DIR) -lmathutils -Wl,-rpath,./$(LIB_DIR)

# Clean up build files
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) calculator
```

### 1.3.4. Comparison Table

| **Feature** | **Static Library (.a)** | **Shared Library (.so)** |
|-------------|------------------------|--------------------------|
| **File Extension** | `.a` (Linux), `.lib` (Windows) | `.so` (Linux), `.dll` (Windows) |
| **Linking Time** | At compile time | At runtime |
| **Executable Size** | Larger (contains library code) | Smaller (contains references only) |
| **Memory Usage** | Higher (each program has its own copy) | Lower (shared among programs) |
| **Updates** | Requires recompilation of all programs | Programs use updated library automatically |
| **Load Time** | Faster (no dynamic linking) | Slightly slower (dynamic linking overhead) |
| **Dependencies** | Self-contained (no runtime dependencies) | Requires library to be available at runtime |
| **Common Use Cases** | Embedded systems, performance-critical applications | Desktop applications, plugins, system libraries |

### 3.5. When to Use Each Type of Library

| **Use Static Libraries When:** | **Use Shared Libraries When:** |
|-------------------------------|--------------------------------|
| Performance is critical | Memory efficiency is important |
| Distribution simplicity is needed | Multiple applications use the same code |
| Target system has limited library support | Frequent updates to libraries are expected |
| Creating standalone applications | Building plugin-based architectures |
| Working with embedded systems | Developing large software ecosystems |

### 1.3.6. Advanced Library Concepts

#### Library Versioning

Shared libraries support versioning to maintain compatibility:

```sh
# Create a versioned shared library
gcc -shared -Wl,-soname,libutils.so.1 -o libutils.so.1.0.0 *.o

# Create symbolic links for version handling
ln -s libutils.so.1.0.0 libutils.so.1
ln -s libutils.so.1 libutils.so
```

#### Using Symbol Visibility

Control which symbols (functions/variables) are exported from libraries:

```c
// Explicitly export a function
__attribute__((visibility("default")))
int exported_function() { ... }

// Hide a function (not accessible outside the library)
__attribute__((visibility("hidden")))
int internal_function() { ... }
```

#### Finding Library Information

Useful commands for inspecting libraries:

```sh
# List symbols in a library
nm libutils.so

# View library dependencies
ldd program

# Check library type and details
file libutils.so

# View shared library version information
objdump -p libutils.so | grep SONAME
```
---

## 1.4. Additional Resources

### 1.4.1. GNU Make Documentation
- [Official GNU Make Manual](https://www.gnu.org/software/make/manual/make.html)
- [Make Quick Reference](https://www.gnu.org/software/make/manual/html_node/Quick-Reference.html)

### 1.4.2. Library Creation and Management
- [Creating Libraries with GCC](https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/GCC/create-libraries/index)
- [Advanced Library Versioning](https://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html)
