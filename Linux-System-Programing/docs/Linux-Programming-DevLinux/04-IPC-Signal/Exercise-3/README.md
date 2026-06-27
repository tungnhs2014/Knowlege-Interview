## Exercise 3: Inter-Process Communication using Signals

## Description
Write a program to create two processes (parent and child) and use the **SIGUSR1** signal to communicate between them.

## Implement
1. Use `fork()` to create a child process from the parent process.
2. The parent process will send the **SIGUSR1** signal to the child process every 2 seconds.
3. Upon receiving the **SIGUSR1** signal, the child process will print the message **"Received signal from parent".**
4. Stop the program after sending the signal 5 times.

## Hint
- Use `kill()` to send the signal from the parent process to the child process.
- Use `signal()` in the child process to catch the **SIGUSR1** signal.

## Question
- Why do we need to use `kill()` to send the signal in this exercise?

**ANSWERS:**
- The `kill()` function is used to send a signal from one process to another.
- Using `kill()` because it explicitly sends the signal from the parent to the child, allowing communication between two independent processes. Without using kill(), there would be no way for the parent to trigger the child process's signal handler.

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
    └── main.c              # Source code implementing IPC with signals
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

### Example Run
```bash
Parent process. PID: 2145, Child PID: 2146
Child process started. PID: 2146
Sent signal 1 to child process
Received signal from parent process
Sent signal 2 to child process
Received signal from parent process
Sent signal 3 to child process
Received signal from parent process
Sent signal 4 to child process
Received signal from parent process
Sent signal 5 to child process
Received signal from parent process
Sent 5 signals. Terminating child process...
Child process terminated. Parent exiting
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```