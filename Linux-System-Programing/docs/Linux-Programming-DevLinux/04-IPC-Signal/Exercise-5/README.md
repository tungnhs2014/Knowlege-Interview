## Exercise 5: Signal Handling with User Input

## Description
Write a C program that can simultaneously handle signals and accept user input from the keyboard.

## Instructions
1. Use `select()` or `poll()` to allow the program to handle signals without interrupting keyboard input.
2. When receiving the **SIGINT** signal, print the message:
```bash
SIGINT received.
```
3. When receiving the **SIGTERM** signal, terminate the program.

## Hint
- Combine signal handling and input reading to make the program more flexible and responsive.

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
    └── main.c              # Source code implementing signal handling with select()
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
$ ./bin/exam
Program started. Press Ctrl + C to send SIGINT, or send SIGTERM to terminate.
hello world
User input: hello world
^C
SIGINT received
^C
SIGINT received
```

#### Sending SIGTERM from Another Terminal:
1. Find the process ID (PID) of the running program:
```bash
ps aux | grep -iE exam
```
2. Send the SIGTERM signal:
```bash
kill -15 PID
```
Output in the running program:
```bash
SIGTERM received. Exiting program...
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```