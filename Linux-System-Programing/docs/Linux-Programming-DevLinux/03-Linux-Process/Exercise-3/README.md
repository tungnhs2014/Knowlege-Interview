# Exercise 3: Process Signal

## Description
This project implements a program that creates a **parent-child process** pair. The parent process sends the **SIGUSR1** signal to the child process after a specific delay.

## Requirements
**Signal Handling:**
   - Use the `signal()` function to define the behavior of the child process when receiving the **SIGUSR1** signal.
   - Print a message when the child process receives the signal

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
    └── main.c              # Source code implementing signal handling
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
- The `SIGUSR1` signal is used for user-defined purposes and can be caught and handled by the process.
- The `signal()` function is used to define the handler for the signal.