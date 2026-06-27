## Exercise 2: Thread Synchronization with Mutex

## Description
Write a C program using **pthread** and mutex to perform the following tasks:

## Instructions
1. Create a global variable `counter` initialized to `0`.
2. Create three threads, each of which increments the value of `counter` by `1`, `1,000,000 times`.
3. Use `mutex` to ensure that the increment operation is performed safely.
4. Print the final value of `counter` after all threads complete.
5. Why is a mutex necessary in this exercise? What would happen if we do not use a mutex?
**ANSWERS:**
- **Why is a mutex necessary in this exercise?**
  + **Race condition:** 
    + When multiple threads access and modify a shared variable (like counter) simultaneously, a race condition can occur.
    + Without a mutex, threads can interleave these steps, leading to data inconsistency.

  + **Synchronization:**
    + A mutex ensures that only **one thread at a time** can perform the increment operation.
    + This prevents other threads from interfering during the update, **ensuring correct and consistent results.**
    
- **What would happen if we do not use a mutex?**
    + **Data Loss:** Threads may simultaneously read the same value of counter, increment it independently, and then write it back. This can cause some increments to be lost.
    + **Incorrect Results:** Instead of the expected value of 3,000,000 (3 threads × 1,000,000 increments), the final counter value may be significantly lower.

## Hint
- Use `pthread_mutex_lock()` and `pthread_mutex_unlock()` to lock and unlock the mutex when accessing the counter.

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
    └── main.c              # Source code implementing thread synchronization
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
Starting thread synchronization example...
Creating 3 threads, each incrementing counter 1000000 times
Final counter value: 3000000
Expected value: 3000000
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```