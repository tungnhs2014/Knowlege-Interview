## Exercise 1: Handling SIGINT Signal

## Description
Write a C program to catch the **SIGINT** signal (sent when pressing `Ctrl+C`) and print a message when the signal is received.


## Requirements
1. Use the `signal()` function to register a handler function for the SIGINT signal.
2. In the signal handler function, print the message **"SIGINT received"** every time the signal is caught.
3. The program should continue running until the **SIGINT** signal is received for the third time, after which the program will terminate.

## Hint
- Use a global counter variable to count the number of times the SIGINT signal is received and check this variable in the signal handler.

## Question
- What will happen if the SIGINT signal is ignored when pressing Ctrl+C?

**ANSWERS:**
- If no handler is registered, the default behavior of the operating system when receiving a **SIGINT** signal is to **terminate the program immediately**.
- If the handler is set to SIG_IGN, the program will completely ignore the SIGINT signal and continue running normally when `Ctrl+C` is pressed, making it impossible to stop the program using the usual shortcut.

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
During Execution
- The program will run continuously, waiting for the **SIGINT** signal (triggered by pressing `Ctrl+C`).
- Every time you press Ctrl+C, the program will print:
```bash
Program started. Press Ctrl+C to send SIGINT signal.
The program will exit after receiving SIGINT 3 times.
```
After receiving the SIGINT signal three times, the program will print a final message and terminate.

### Example Run
```bash
Program started. Press Ctrl+C to send SIGINT signal.
The program will exit after receiving SIGINT 3 times.
^C
SIGINT received: 1
^C
SIGINT received: 2
^C
SIGINT received: 3
Received SIGINT 3 times. Exiting program...
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```