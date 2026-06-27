## Exercise 1: Using Pipe for Communication Between Parent and Child Process

## Description
Write a program that uses a **pipe** to enable communication between two processes.

## Requirements
1. Create a pipe using the `pipe()` system call.
2. Use `fork()` to create a child process.
3. The **parent process** sends a string message through the pipe.
4. The **child process** reads the message from the pipe and prints it to the screen.

## Hint
- Use the `pipe()` function to create the pipe.
- Use `fork()` to create a child process.
- Use `write()` in the parent process to send data.
- Use `read()` in the child process to receive and display the message.

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
    └── main.c              # Source code implementing pipe-based IPC
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
Parent process (PID: 5331) sending message: Hello from parent process
Child process (PID: 5332) received message: Hello from parent process
Child process terminated
Parent process: Child exited with status 0
Parent process terminated
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```