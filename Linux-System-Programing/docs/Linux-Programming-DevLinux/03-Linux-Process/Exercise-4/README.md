## Exercise 4: Process Status Recording

## Description
This project implements a program that creates a **child process**, and the parent process uses **wait()** to wait for the child process to terminate and record its exit status.

## Requirements
1. **Child Process Exit Status:**
   - In the child process, set the exit status by calling **exit()** with different values.

2. **Status Recording:**
   - Use **WIFEXITED()** and **WEXITSTATUS()** to check and record the exit code of the child process.

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
    └── main.c              # Source code implementing process status monitoring
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

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```

---

## 💡 Notes
- The **wait()** function is used to wait for the child process to finish.
- The **WIFEXITED()** macro checks if the child process exited normally.
- The **WEXITSTATUS()** macro retrieves the exit status of the child process if it exited normally.