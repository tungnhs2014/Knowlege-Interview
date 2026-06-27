## Exercise 1: Basic Thread Creation and Execution

## Description
Write a C program using the **pthread.h** library to perform the following tasks:

## Instructions
1. Create two threads. Each thread should print a message along with its ID
```bash
Thread 1: Hello from thread.
```
2. Use the `pthread_join()` function to wait for both threads to complete before the program exits.
3. Understand how to use `pthread_create()`and `pthread_join()`, and when a thread terminates.


## Hint
- Use the `pthread_create()` function to create threads and the `pthread_join()`function to wait for threads to finish.

---

## Project Structure
```
project/
├── bin/         
│   └── exam                # Executable binary file
├── Makefile                # Build automation script
├── obj/         
│   └── main.o              # Compiled object file
└── src/         
    └── main.c              # Source code implementing thread creation
```
---

## Build and Run Instructions

### Build the program
To build the program, run:
```bash
$ make
```

### Run the program
To execute the program, use:
```bash
$ ./bin/exam
```

### Example output
```bash
Main: Created thread 1
Thread 1: Hello from thread!
Main: Created thread 2
Thread 2: Hello from thread!
Main: All threads have completed execution
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```