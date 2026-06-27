## Exercise 5: Sharing Resources with Read-Write Lock

## Description
Write a program to simulate a scenario where multiple threads read and write shared data:

1. Create a shared integer variable `data`.
2. Create 5 reader threads and 2 writer threads:
   - **Reader threads** only read `data` without modifying it.
   - **Writer threads** increase the value of data by 1.
3. Use a read-write lock to ensure:
   - Multiple reader threads can read simultaneously.
   - Only one writer thread can modify `data` at a time.
4. Print the final value of data after all threads complete execution.


## Hint
- Use `pthread_rwlock_rdlock` for reading and `pthread_rwlock_wrlock` for writing to maintain proper synchronization.

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
    └── main.c              # Source code implementing read-write locks
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
Starting read-write lock demonstration
Creating 5 reader threads and 2 writer threads
Each thread will perform 5 iterations

Writer 1 update data to: 1
Writer 2 update data to: 2
Reader 1 reads data: 2
Reader 2 reads data: 2
Reader 3 reads data: 2
Reader 4 reads data: 2
Reader 5 reads data: 2
Reader 1 reads data: 2
Reader 2 reads data: 2
Reader 4 reads data: 2
Reader 3 reads data: 2
Reader 5 reads data: 2
Writer 1 update data to: 3
Writer 2 update data to: 4
Reader 1 reads data: 4
Reader 5 reads data: 4
Reader 2 reads data: 4
Reader 4 reads data: 4
Reader 3 reads data: 4
Reader 1 reads data: 4
Reader 2 reads data: 4
Reader 5 reads data: 4
Reader 4 reads data: 4
Reader 3 reads data: 4
Writer 1 update data to: 5
Writer 2 update data to: 6
Reader 1 reads data: 6
Reader 5 reads data: 6
Reader 2 reads data: 6
Reader 3 reads data: 6
Reader 4 reads data: 6
Writer 1 update data to: 7
Writer 2 update data to: 8
Writer 1 update data to: 9
Writer 2 update data to: 10

All threads completed execution
Final value of data: 10
Expected value: 10
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```