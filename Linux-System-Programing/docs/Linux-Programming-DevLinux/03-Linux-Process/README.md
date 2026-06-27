# 3. Linux Process

## 3.1. Fundamentals of Programs and Processes

### 3.1.1. Programs

A program is a set of instructions designed to perform specific tasks on a computer system:

- **Definition**: An executable file containing machine code instructions stored on disk
- **Characteristics**:
  - **Static entity**: Exists as files on storage devices (hard drive, SSD)
  - **Passive**: Does not consume CPU or memory resources until executed
  - **Persistent**: Remains on the storage device even after the computer shuts down
  - **Shareable**: Multiple users can execute the same program simultaneously

Programs are created through a compilation process that transforms human-readable source code into machine-executable instructions. In Linux, executable programs are typically stored in directories like `/bin`, `/usr/bin`, or `/usr/local/bin`.

### 3.1.2. Processes

A process is an instance of a program that is being executed:

- **Definition**: A running program that occupies system resources
- **Characteristics**:
  - **Dynamic entity**: Active in memory and using computing resources
  - **Resource allocation**: Has allocated CPU time, memory space, file handles
  - **Unique identifier**: Each process has a unique Process ID (PID)
  - **Isolation**: Runs in its own virtual memory space
  - **Lifecycle**: Created, executed, and eventually terminated

Every process in Linux has:
- A unique numeric identifier (PID)
- An owner (user) and group
- A parent process that created it
- Resource usage statistics
- State information (running, sleeping, stopped, etc.)

### 3.1.3. Relationship Between Programs and Processes

Understanding the relationship between programs and processes is essential for effective system management:

| **Program** | **Process** |
|------------|-------------|
| Static executable file on disk | Running instance in memory |
| One program file | Can have multiple process instances |
| Requires no system resources when inactive | Actively consumes CPU, memory, and I/O resources |
| Exists even when not running | Exists only during execution |
| Modified through file operations | Modified through system calls |

**Key points about the relationship**:

1. **One-to-many relationship**: A single program can spawn multiple processes:
   - Multiple users can run the same program simultaneously
   - One user can run multiple instances of the same program
   - Example: The `bash` program can run as multiple different shell processes

2. **Resource independence**: Each process gets its own:
   - Memory space (stack, heap, data segments)
   - File descriptors and handles
   - Process ID and associated metadata

3. **Creation sequence**:
   - Program file is loaded from disk into memory
   - Operating system allocates resources
   - Process is created and begins execution
   - Process terminates when the task completes or through explicit termination

---

## 3.2. Command-line Arguments

### 3.2.1. Introduction to Command-line Arguments

Command-line arguments provide a way to pass information to a program when it starts:

- They allow flexible program behavior without modifying code
- They enable automated scripts to interact with programs
- They follow consistent conventions across Unix/Linux systems

In Linux, command-line arguments are specified after the program name:

```bash
$ program_name argument1 argument2 argument3
```

### 3.2.2. Accessing Command-line Arguments in C Programs

In C programs, command-line arguments are accessed through parameters in the `main()` function:

```c
int main(int argc, char *argv[])
```

These parameters provide access to all arguments passed to the program:

#### The `argc` Parameter

- **Name**: Argument Count
- **Type**: Integer (`int`)
- **Purpose**: Contains the number of command-line arguments
- **Value**: Always at least 1 (the program name itself is counted as the first argument)

#### The `argv` Parameter

- **Name**: Argument Vector
- **Type**: Array of strings (`char *argv[]` or `char **argv`)
- **Purpose**: Contains the actual arguments as strings
- **Index values**:
  - `argv[0]`: Program name (or path to the program)
  - `argv[1]`: First actual argument
  - `argv[2]`: Second actual argument
  - ...
  - `argv[argc-1]`: Last argument

### 3.2.3. Example: Processing Command-line Arguments

Here's a simple C program that demonstrates how to access and use command-line arguments:

```c
#include <stdio.h>

int main(int argc, char *argv[])
{
    // Print the number of arguments
    printf("Received %d command-line arguments\n", argc);
    
    // Print each argument with its position
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    
    // Check if at least one actual argument was provided
    if (argc > 1) {
        printf("\nThe first actual argument is: %s\n", argv[1]);
    } else {
        printf("\nNo additional arguments were provided.\n");
    }
    
    return 0;
}
```

When you compile and run this program with arguments:

```bash
$ gcc -o args args.c
$ ./args hello world 123
```

The output will be:

```
Received 4 command-line arguments
argv[0]: ./args
argv[1]: hello
argv[2]: world
argv[3]: 123

The first actual argument is: hello
```

### 3.2.4. Practical Uses for Command-line Arguments

Command-line arguments are used for many purposes:

1. **Configuration options**:
   ```bash
   $ ls -l -a  # '-l' and '-a' are arguments for long format and show all files
   ```

2. **Input/output file specifications**:
   ```bash
   $ cp source.txt destination.txt  # Source and destination files as arguments
   ```

3. **Operation modes**:
   ```bash
   $ grep -i "search term" file.txt  # '-i' for case-insensitive search
   ```

4. **Numeric parameters**:
   ```bash
   $ head -n 20 file.txt  # '20' specifies to show the first 20 lines
   ```

### 3.2.5. Argument Processing Patterns

When processing command-line arguments, several common patterns are used:

1. **Simple positional arguments**:
   ```c
   if (argc < 3) {
       printf("Usage: %s source destination\n", argv[0]);
       return 1;
   }
   char *source = argv[1];
   char *destination = argv[2];
   ```

2. **Flag arguments** (options that don't take values):
   ```c
   int verbose = 0;
   for (int i = 1; i < argc; i++) {
       if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
           verbose = 1;
       }
   }
   ```

3. **Option arguments** (options that take values):
   ```c
   int count = 10;  // Default value
   for (int i = 1; i < argc; i++) {
       if (strcmp(argv[i], "-n") == 0 && i+1 < argc) {
           count = atoi(argv[i+1]);
           i++;  // Skip the next argument since we used it
       }
   }
   ```
---

## 3.3. Process Memory Layout

### 3.3.1. Memory Segments Overview

Each process in Linux has its own virtual address space, divided into multiple segments that serve different purposes:

#### Text/Code Segment

- **Purpose**: Contains the executable code instructions
- **Location**: Lowest memory addresses
- **Characteristics**:
  - Read-only for security (prevents code modification)
  - Shared among processes running the same program
  - Fixed size determined at compile time

```c
// Code that gets placed in the text segment
int add(int a, int b) {
    return a + b;
}
```

#### Initialized Data Segment (.data)

- **Purpose**: Stores global and static variables initialized with non-zero values
- **Location**: Above the text segment
- **Characteristics**:
  - Read-write access
  - Loaded directly from the executable file
  - Fixed size determined at compile time

```c
// Variables that go into the data segment
int global_var = 100;
static int static_var = 200;
const int read_only_var = 300;  // May be placed in read-only section
```

#### Uninitialized Data Segment (BSS)

- **Purpose**: Contains global and static variables initialized to zero or not explicitly initialized
- **Location**: Above the initialized data segment
- **Characteristics**:
  - Read-write access
  - Zeroed by the kernel before program start
  - Does not occupy space in the executable file

```c
// Variables that go into the BSS segment
int global_uninitialized;
static int static_uninitialized;
static char buffer[10000];  // Large zero-initialized array
```

#### Heap

- **Purpose**: Area for dynamic memory allocation during runtime
- **Location**: Starts after the BSS segment and grows upward
- **Characteristics**:
  - Read-write access
  - Managed by `malloc()`, `calloc()`, `realloc()`, and `free()`
  - Size can change during program execution
  - Not automatically freed when variables go out of scope

```c
// Heap allocation example
int *dynamic_array = malloc(10 * sizeof(int));
// Use the memory
free(dynamic_array);  // Must be explicitly freed
```

#### Memory Mapping Segment

- **Purpose**: Region where files and shared libraries are mapped
- **Location**: Between heap and stack
- **Characteristics**:
  - Can have various permissions (read, write, execute)
  - Used for loading dynamic libraries (e.g., libc.so)
  - Used for memory-mapped files
  - Used for shared memory between processes

```c
// Memory mapping example
void *mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
// Use the mapped memory
munmap(mapped, size);  // Unmap when done
```

#### Stack

- **Purpose**: Stores function call information and local variables
- **Location**: High memory addresses, grows downward
- **Characteristics**:
  - Read-write access
  - Automatically managed (grows/shrinks with function calls/returns)
  - LIFO (Last In, First Out) data structure
  - Each function call creates a "stack frame"
  - Limited in size (typically 8MB by default)

```c
// Variables that go on the stack
void function() {
    int local_var = 10;
    char buffer[1024];
    // These variables are automatically created on the stack
    // and freed when the function returns
}
```

#### Kernel Space

- **Purpose**: Reserved for the operating system kernel
- **Location**: Highest memory addresses
- **Characteristics**:
  - Not directly accessible from user mode programs
  - Protected by CPU privilege levels
  - Accessed indirectly through system calls
  - In 32-bit systems, typically occupies the top 1GB of the 4GB address space

### 3.3.2. Memory Address Layout Visualization

#### 32-bit Linux System Memory Layout

```
0xFFFFFFFF  +---------------------+
            |                     |
            |    Kernel Space     |  (~1GB)
            |                     |
0xC0000000  +---------------------+
            |        Stack        |  (grows downward ↓)
            |        ...          |
            +---------------------+
            |                     |
            |   Memory Mapping    |
            |   Segment           |
            |                     |
            +---------------------+
            |                     |
            |    Unused Space     |
            |                     |
            +---------------------+
            |        Heap         |  (grows upward ↑)
            +---------------------+
            |         BSS         |
            +---------------------+
            |     Data (.data)    |
            +---------------------+
            |  Text/Code (.text)  |
            +---------------------+
0x08048000  |                     |
            |  Reserved/Unmapped  |
            |                     |
0x00000000  +---------------------+
```

#### 64-bit Linux System Memory Layout (Simplified)

```
0xFFFFFFFFFFFFFFFF  +---------------------+
                    |                     |
                    |    Kernel Space     |
                    |                     |
0xFFFF800000000000  +---------------------+
                    |                     |
                    |   Unused Address    |
                    |        Space        |
                    |                     |
0x00007FFFFFFFFFFF  +---------------------+
                    |        Stack        |  (grows downward ↓)
                    +---------------------+
                    |   Memory Mapping    |
                    |      Segment        |
                    +---------------------+
                    |        Heap         |  (grows upward ↑)
                    +---------------------+
                    |   BSS, Data, Text   |
                    +---------------------+
0x00000000         +---------------------+
```

### 3.3.3. Memory Security Mechanisms

Modern Linux systems implement several security mechanisms to protect process memory:

1. **ASLR (Address Space Layout Randomization)**:
   - Randomizes the base addresses of memory segments
   - Makes it harder for attackers to predict memory addresses
   - Helps prevent buffer overflow and return-to-libc attacks

2. **DEP/NX (Data Execution Prevention/No-Execute)**:
   - Marks data pages as non-executable
   - Prevents code execution in data areas like stack and heap
   - Blocks most shellcode-injection attacks

3. **Stack Canaries**:
   - Special values placed before the return address on the stack
   - Checked before function returns
   - Helps detect stack buffer overflows

4. **PIE (Position Independent Executable)**:
   - Executable code that can run at any memory location
   - Works with ASLR to randomize all segments, including text segment
   - Further increases security against memory attacks

### 3.3.4. Monitoring Process Memory

Linux provides several tools to examine process memory usage:

```bash
# View memory mapping of a process
cat /proc/PID/maps

# Display memory map in a more readable format
pmap PID

# Display the memory usage statistics for a process
cat /proc/PID/status

# Monitor memory usage in real-time
top
htop

# Analyze memory usage in depth
valgrind --tool=memcheck ./program
```

Example of `/proc/PID/maps` output:
```
address           perms offset  dev   inode      pathname
00400000-00452000 r-xp 00000000 08:02 173521      /usr/bin/dbus-daemon
00651000-00652000 r--p 00051000 08:02 173521      /usr/bin/dbus-daemon
00652000-00655000 rw-p 00052000 08:02 173521      /usr/bin/dbus-daemon
00e03000-00e24000 rw-p 00000000 00:00 0           [heap]
00e24000-011f7000 rw-p 00000000 00:00 0           [heap]
...
7dfea3000000-7dfea3021000 rw-p 00000000 00:00 0
7dfea3021000-7dfea4000000 ---p 00000000 00:00 0
7ffdc35ff000-7ffdc3620000 rw-p 00000000 00:00 0           [stack]
7ffdc36fe000-7ffdc3700000 r-xp 00000000 00:00 0           [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0   [vsyscall]
```
---

## 3.4. Process Creation and Execution

### 3.4.1. The fork() System Call

The `fork()` system call is fundamental to process creation in Linux. It creates a new process by duplicating the calling process:

<p align="center">
  <img src="https://github.com/user-attachments/assets/b4810c85-6346-46d4-9a7d-de96e5e66cba" alt="Fork System Call" width="70%">
</p>

#### How fork() Works

1. When a process calls `fork()`:
   - The kernel creates a new process (child)
   - The child receives a copy of the parent's memory segments
   - Both processes continue execution from the instruction following `fork()`
   - The child and parent run concurrently

2. Key characteristics:
   - The child process gets a new PID
   - The child's parent PID (PPID) is set to the parent's PID
   - Resources like file descriptors are duplicated
   - Memory is copied using copy-on-write for efficiency

#### Return Values

The `fork()` system call has different return values depending on the process:

```c
pid_t pid = fork();

if (pid < 0) {
    // Fork failed - error handling
}
else if (pid == 0) {
    // Child process - pid is 0
}
else {
    // Parent process - pid is the child's PID
}
```

| **Return Value** | **Process** | **Meaning** |
|------------------|-------------|-------------|
| `-1` | Parent | Fork failed, no child was created |
| `0` | Child | You are in the child process |
| `> 0` | Parent | Successful fork, value is the child's PID |

#### Memory Behavior After fork()

After a fork operation:
- Each process has its own address space
- Initially, both processes share the same content
- When either process modifies memory, a copy is created (copy-on-write)
- Changes to variables in one process do not affect the other

#### Complete fork() Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int x = 100;
    printf("Before fork: x = %d (PID: %d)\n", x, getpid());
    
    pid_t pid = fork();
    
    if (pid < 0) {
        // Error handling
        fprintf(stderr, "Fork failed\n");
        return 1;
    }
    else if (pid == 0) {
        // Child process
        x += 50;
        printf("Child process: x = %d (PID: %d, Parent PID: %d)\n", 
               x, getpid(), getppid());
    }
    else {
        // Parent process
        x -= 50;
        printf("Parent process: x = %d (PID: %d, Child PID: %d)\n", 
               x, getpid(), pid);
    }
    
    printf("Process %d exiting with x = %d\n", getpid(), x);
    return 0;
}
```

Output:
```
Before fork: x = 100 (PID: 12345)
Parent process: x = 50 (PID: 12345, Child PID: 12346)
Process 12345 exiting with x = 50
Child process: x = 150 (PID: 12346, Parent PID: 12345)
Process 12346 exiting with x = 150
```

### 3.4.2. The exec Family - Process Replacement

While `fork()` creates a new process that is a copy of the parent, the exec functions replace the current process with a new program:

#### How exec Works

1. The exec system call:
   - Replaces the current process image with a new one
   - Loads a new program into the current process
   - Keeps the same process ID
   - Discards the old program entirely
   - Resets signals to default behavior

2. Key characteristics:
   - The process ID remains the same
   - Most process attributes are preserved
   - Memory contents are replaced with the new program
   - On success, exec does not return to the calling program

#### The exec Family of Functions

There are several variants of exec, each with different features:

| **Function** | **Path Search** | **Argument Format** | **Environment** |
|--------------|----------------|---------------------|-----------------|
| `execl()` | No (full path) | List | Inherit |
| `execlp()` | Yes (uses PATH) | List | Inherit |
| `execle()` | No (full path) | List | Explicit |
| `execv()` | No (full path) | Vector | Inherit |
| `execvp()` | Yes (uses PATH) | Vector | Inherit |
| `execvpe()` | Yes (uses PATH) | Vector | Explicit |

The naming convention explains each function's behavior:
- `l`: Arguments passed as a list (varargs)
- `v`: Arguments passed as a vector (array)
- `p`: Program name search using PATH environment variable
- `e`: Custom environment variables can be provided

#### Example of exec Functions

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Original program running (PID: %d)\n", getpid());
    
    // Using execl (full path, list arguments)
    execl("/bin/ls", "ls", "-l", "-a", NULL);
    
    // If execl is successful, this code will never run
    fprintf(stderr, "execl failed!\n");
    return 1;
}
```

Alternative using execvp:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Original program running (PID: %d)\n", getpid());
    
    // Array of arguments
    char *args[] = {"ls", "-l", "-a", NULL};
    
    // Using execvp (search PATH, vector arguments)
    execvp("ls", args);
    
    // If execvp is successful, this code will never run
    fprintf(stderr, "execvp failed!\n");
    return 1;
}
```

### 3.4.3. The Fork-Exec Pattern

The combination of `fork()` and `exec()` is a fundamental pattern in Unix/Linux programming:

1. Parent process calls `fork()` to create a child process
2. Child process calls an `exec` function to run a new program
3. Parent process can either wait for the child or continue execution

This pattern is the foundation of how shells and many other Linux utilities work.

#### Example: Complete Fork-Exec Pattern

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Parent process (PID: %d) starting\n", getpid());
    
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return 1;
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d) will execute 'ls -l'\n", getpid());
        
        // Replace child process with 'ls -l' command
        execl("/bin/ls", "ls", "-l", NULL);
        
        // If execl returns, it failed
        fprintf(stderr, "Exec failed!\n");
        exit(1);
    }
    else {
        // Parent process
        printf("Parent process (PID: %d) waiting for child (PID: %d)\n", 
               getpid(), pid);
        
        int status;
        wait(&status);
        
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        } else {
            printf("Child process did not exit normally\n");
        }
    }
    
    return 0;
}
```

Output:
```
Parent process (PID: 12345) starting
Parent process (PID: 12345) waiting for child (PID: 12346)
Child process (PID: 12346) will execute 'ls -l'
total 40
-rwxr-xr-x 1 user user 16384 Apr 12 12:30 a.out
-rw-r--r-- 1 user user  1234 Apr 12 12:29 main.c
...
Child process exited with status: 0
```
----

## 3.5. Process Management and Synchronization

Process management in Linux involves creating, monitoring, and coordinating processes. Understanding how processes interact is essential for effective system programming.

<p align="center">
  <img src="https://github.com/user-attachments/assets/368ca4a6-2058-4e9a-b0c9-c1cbb0588ac4" alt="Process Management" width="70%">
</p>

### 3.5.1. Process Termination with exit()

The `exit()` function terminates the calling process and returns a status code to the parent process:

```c
#include <stdlib.h>

void exit(int status);
```

- `status`: Exit code that indicates success (0) or failure (non-zero)
- When a process calls `exit()`:
  - All open file descriptors are closed
  - Child processes are adopted by init (PID 1)
  - Exit status is sent to the parent process
  - Resources are released back to the system

Example:
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Do some work
    printf("Process (PID: %d) is running\n", getpid());
    
    // Check for error conditions
    FILE *file = fopen("non_existent_file.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);  // Standard error exit code (non-zero)
    }
    
    // Normal termination
    printf("Process completed successfully\n");
    exit(EXIT_SUCCESS);  // Standard success exit code (0)
}
```

### 3.5.2. Waiting for Child Processes

Parent processes need to wait for their children to avoid leaving zombie processes in the system.

#### The wait() System Call

The `wait()` system call suspends the parent process until one of its children terminates:

```c
#include <sys/wait.h>

pid_t wait(int *status);
```

- Blocks until any child process terminates
- Returns the PID of the terminated child
- Stores the exit status in the provided integer pointer
- Returns -1 on error (no children to wait for)

Example:
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d) running\n", getpid());
        sleep(2);  // Simulate work
        exit(42);  // Exit with a specific code
    }
    else {
        // Parent process
        printf("Parent (PID: %d) waiting for child (PID: %d)\n", getpid(), pid);
        
        int status;
        pid_t terminated_pid = wait(&status);
        
        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child (PID: %d) exited with status: %d\n", 
                       terminated_pid, WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status)) {
                printf("Child (PID: %d) killed by signal: %d\n", 
                       terminated_pid, WTERMSIG(status));
            }
        }
    }
    
    return 0;
}
```

#### The waitpid() System Call

The `waitpid()` system call offers more control by allowing you to wait for a specific child:

```c
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int options);
```

- `pid`: Which child to wait for
  - `-1`: Any child (like `wait()`)
  - `> 0`: Specific child PID
  - `0`: Any child in the same process group
  - `< -1`: Any child whose process group ID equals the absolute value of pid
- `status`: Exit status storage
- `options`: Additional control flags
  - `WNOHANG`: Don't block if no child has exited
  - `WUNTRACED`: Report status of stopped children
  - `WCONTINUED`: Report status of continued children

Example with non-blocking wait:
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        printf("Child (PID: %d) running for 5 seconds\n", getpid());
        sleep(5);
        exit(0);
    }
    else {
        // Parent process
        printf("Parent (PID: %d) will check child status every second\n", getpid());
        
        int status;
        pid_t result;
        
        do {
            // Try to reap child without blocking (WNOHANG)
            result = waitpid(pid, &status, WNOHANG);
            
            if (result == 0) {
                // Child still running
                printf("Child (PID: %d) still running...\n", pid);
                sleep(1);
            }
            else if (result > 0) {
                // Child terminated
                printf("Child (PID: %d) has terminated\n", result);
                if (WIFEXITED(status)) {
                    printf("Exit status: %d\n", WEXITSTATUS(status));
                }
            }
            else {
                // Error occurred
                perror("waitpid failed");
                exit(1);
            }
        } while (result == 0);
    }
    
    return 0;
}
```

### 3.5.3. Interpreting Exit Status

The exit status from `wait()` and `waitpid()` contains information about how the child process terminated:

```c
// Check if child terminated normally (by exit or return)
if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);  // Get the exit code (0-255)
}

// Check if child was terminated by a signal (e.g., SIGKILL)
if (WIFSIGNALED(status)) {
    int signal_num = WTERMSIG(status);  // Get the terminating signal number
}

// Check if child is currently stopped (e.g., SIGSTOP)
if (WIFSTOPPED(status)) {
    int stop_signal = WSTOPSIG(status);  // Get the stop signal number
}

// Check if child continued from a stop (SIGCONT)
if (WIFCONTINUED(status)) {
    // Child was resumed by SIGCONT
}
```
--- 

## 3.6. Orphan and Zombie Processes

Understanding orphan and zombie processes is crucial for proper process management in Linux systems.

### 3.6.1. Orphan Processes

An orphan process occurs when a parent process terminates before its child processes:

- **Definition**: A process whose parent has terminated
- **Behavior**: The orphaned process is "adopted" by the init process (PID 1)
- **Impact**: Minimal; init automatically reaps terminated orphans

#### Example: Creating an Orphan Process

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d, Parent PID: %d) running\n", 
               getpid(), getppid());
               
        // Sleep to give parent time to exit
        sleep(2);
        
        // After parent exits, we're adopted by init
        printf("Child process (PID: %d, New Parent PID: %d) is now an orphan\n", 
               getpid(), getppid());
               
        exit(0);
    }
    else {
        // Parent process
        printf("Parent process (PID: %d) created child (PID: %d)\n", 
               getpid(), pid);
               
        printf("Parent process exiting immediately\n");
        exit(0);  // Parent exits, orphaning the child
    }
    
    return 0;
}
```

In this example, the parent process exits immediately after creating a child. The child process becomes an orphan and is adopted by the init process, which is confirmed by the change in the parent PID.

#### Creating Intentional Orphans (Daemons)

Orphaning is sometimes done intentionally to create daemon processes that run in the background:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    // First fork to create a background process
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("First fork failed");
        exit(1);
    }
    else if (pid > 0) {
        // Parent exits, making child an orphan
        exit(0);
    }
    
    // Child process continues
    
    // Create new session and process group
    if (setsid() < 0) {
        perror("setsid failed");
        exit(1);
    }
    
    // Second fork to prevent reacquiring a terminal
    pid = fork();
    
    if (pid < 0) {
        perror("Second fork failed");
        exit(1);
    }
    else if (pid > 0) {
        // First child exits, second child becomes a daemon
        exit(0);
    }
    
    // Second child continues as the daemon
    
    // Change working directory to root
    chdir("/");
    
    // Close all open file descriptors
    for (int i = 0; i < 1024; i++) {
        close(i);
    }
    
    // Redirect standard I/O to /dev/null
    open("/dev/null", O_RDWR); // stdin
    dup(0); // stdout
    dup(0); // stderr
    
    // Daemon code starts here
    while (1) {
        // Do daemon work
        sleep(60);
    }
    
    return 0;
}
```

This is the standard procedure for creating a daemon process in Linux.

### 3.6.2. Zombie Processes

A zombie process (also called a defunct process) is a process that has completed execution but still has an entry in the process table:

- **Definition**: A terminated process whose exit status has not yet been collected by its parent
- **Behavior**: Appears as `<defunct>` in process listings with state 'Z'
- **Impact**: Consumes process table entries; excessive zombies can prevent creation of new processes

#### Example: Creating a Zombie Process

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d) is running\n", getpid());
        printf("Child process will exit now\n");
        exit(0);  // Child terminates
    }
    else {
        // Parent process
        printf("Parent (PID: %d) created child (PID: %d)\n", getpid(), pid);
        printf("Parent will sleep for 30 seconds without waiting for child\n");
        printf("You can check for zombie using: ps -ef | grep Z\n");
        
        // Sleep without calling wait() - child becomes a zombie
        sleep(30);
        
        printf("Parent exiting\n");
    }
    
    return 0;
}
```

When you run this program, the child process will terminate but remain as a zombie because the parent doesn't call `wait()` to collect its exit status.

### 3.6.3. Preventing Zombie Processes

There are several methods to prevent zombie processes:

#### Method 1: Using wait() or waitpid()

The most straightforward approach is for the parent to collect the exit status of its children:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d) is running\n", getpid());
        sleep(2);  // Simulate work
        printf("Child process exiting\n");
        exit(0);
    }
    else {
        // Parent process
        printf("Parent (PID: %d) waiting for child (PID: %d)\n", getpid(), pid);
        
        // Wait for the child to prevent it from becoming a zombie
        int status;
        waitpid(pid, &status, 0);
        
        printf("Parent collected child's exit status\n");
    }
    
    return 0;
}
```

#### Method 2: Using Signal Handlers (SIGCHLD)

For parents that need to continue execution without blocking, a SIGCHLD handler can be used:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
    // Reap all terminated children
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        printf("Reaped a child process\n");
    }
}

int main() {
    // Set up the SIGCHLD handler
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // Create multiple child processes
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }
        else if (pid == 0) {
            // Child process
            printf("Child %d (PID: %d) is running\n", i, getpid());
            sleep(i + 1);  // Each child runs for a different time
            printf("Child %d exiting\n", i);
            exit(0);
        }
        else {
            // Parent process
            printf("Parent created child %d (PID: %d)\n", i, pid);
        }
    }
    
    // Parent continues with other work
    printf("Parent doing other work while children run\n");
    for (int i = 0; i < 10; i++) {
        printf("Parent working... (%d/10)\n", i + 1);
        sleep(1);
    }
    
    printf("Parent exiting\n");
    return 0;
}
```

In this example, the parent sets up a SIGCHLD handler that automatically reaps any terminated child process, preventing zombies.

#### Method 3: Double Fork Technique

For processes that need to run independently (like daemons), the double fork technique can be used:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("First fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // First child
        pid_t second_pid = fork();
        
        if (second_pid < 0) {
            perror("Second fork failed");
            exit(1);
        }
        else if (second_pid == 0) {
            // Second child (grandchild of original parent)
            printf("Grandchild process (PID: %d) running\n", getpid());
            sleep(5);  // Do work
            printf("Grandchild process completed\n");
            exit(0);
        }
        else {
            // First child immediately exits, orphaning the grandchild
            // The grandchild is adopted by init, which will reap it
            printf("First child exiting, grandchild (PID: %d) will be adopted by init\n", 
                   second_pid);
            exit(0);
        }
    }
    else {
        // Original parent waits for first child
        waitpid(pid, NULL, 0);
        printf("Original parent reaped first child, grandchild now owned by init\n");
    }
    
    return 0;
}
```

This technique creates a "grandchild" process that is immediately orphaned and adopted by init, which will reap it when it terminates.

### 3.6.4. Detecting Zombie Processes

To check for zombie processes in a system:

```bash
# List all zombie processes
ps aux | grep 'Z'

# More detailed view with process hierarchy
ps -el | grep 'Z'

# Find zombies with their parent information
ps -xal | grep 'Z'
```

A zombie process will show as `<defunct>` in the command column and have a status of 'Z'.
