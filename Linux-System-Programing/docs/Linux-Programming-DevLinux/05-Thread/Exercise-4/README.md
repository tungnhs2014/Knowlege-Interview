## Exercise 4: Counting Even and Odd Numbers with Threads

## Description
Write a program to perform the following tasks:
1. Generate a random array of **100 integers** with values ranging from **1 to 100**.
2. Create two threads:
   - One thread counts the **even numbers** in the array.
   - The other thread counts the **odd numbers** in the array.
3. Once both threads complete counting, print the total number of **even numbers** and **odd numbers**.
4. Use **pthread_join** to ensure the program only terminates when both threads finish their tasks.

## Hint
- Use **pthread_join** to ensure the program only terminates when both threads finish their tasks.

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
    └── main.c              # Source code implementing threaded counting
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
Generating 100 random numbers and counting even/odd numbers using threads
Generated numbers: 42 68 35 1 79 95 27 62 3 84 ...
Created two threads to count even and odd numbers
Both threads have completed
Total even numbers: 53
Total odd numbers: 47
Total numbers: 100
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```