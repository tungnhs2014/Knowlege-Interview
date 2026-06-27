## Exercise 2: Count Characters Using POSIX Message Queue

## Description
Extend the previous exercise to let the child process count the number of characters in the received message.

## Requirements
1. The **parent process** sends a string message through a POSIX message queue.
2. The **child process** receives the message, counts the number of characters, and sends the result back through **another message queue**.
3. The **parent process** receives the result and prints the number of characters counted.

## Hint
- Create two message queues:
    1. One for sending the string from **parent → child**.
    2. One for sending the result from **child → parent**.
- Use `strlen()` to calculate the number of characters in the received string.
- Remember to `close` and `unlink` both message queues after use.

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
    └── main.c              # Source code implementing character counting with message queues
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
[Parent] Process ID: 6262
[Parent] Sending message to child: Hello, I am TungNHS
[Child] Process ID: 6263
[Child] Message received: Hello, I am TungNHS
[Child] Character count: 19
[Parent] Character count received from child: 19
[Parent] Child exited with status 0
[Parent] Cleaning up message queues...
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```