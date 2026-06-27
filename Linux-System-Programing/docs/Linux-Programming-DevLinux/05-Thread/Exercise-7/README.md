## Exercise 7: Producer-Consumer Problem

## Description
This program implements the classical Producer-Consumer synchronization problem using multiple threads, mutex, and condition variables.

## Implementation
- **Producers**: 10 threads that generate random numbers and add them to a shared buffer.
- **Consumers**: 5 threads that remove numbers from the shared buffer.
- **Buffer**: A circular buffer with a fixed size of 5 elements.
- **Synchronization**:
  - A mutex to protect the shared buffer.
  - Two condition variables:
    - `not_empty` is signaled when an item is added to the buffer
    - `not_full` is signaled when an item is removed from the buffer

## Synchronization Rules
1. Multiple producers cannot add items simultaneously.
2. Multiple consumers cannot remove items simultaneously.
3. Producers must wait when the buffer is full.
4. Consumers must wait when the buffer is empty.

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
    └── main.c              # Source code implementing producer-consumer pattern
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
Starting Producer-Consumer demonstration
Creating 10 producers and 5 consumers with buffer size 5

Producer 0: Added item 83 at position 0
Producer 3: Added item 86 at position 1
  Consumer 2: Removed item 83 from position 0
Producer 5: Added item 77 at position 0
  Consumer 0: Removed item 86 from position 1
Producer 1: Added item 15 at position 1
Producer 7: Added item 93 at position 2
  Consumer 1: Removed item 77 from position 0
Producer 2: Added item 35 at position 0
  Consumer 4: Removed item 15 from position 1
Producer 6: Added item 86 at position 1
  Consumer 3: Removed item 93 from position 2
...

Producer 9: Finished producing items
Producer 8: Finished producing items
Producer 5: Finished producing items
Producer 2: Finished producing items
Consumer 4: Finished consuming items
Consumer 2: Finished consuming items
Consumer 0: Finished consuming items
Consumer 1: Finished consuming items
Consumer 3: Finished consuming items

All threads have completed
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```