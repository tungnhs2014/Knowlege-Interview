## Exercise 2: Using Multiple Pipes for Communication Between Processes

## Description
Extend the previous exercise by using **multiple pipes** to enable communication among three processes.

## Requirements
1. Create two pipes:
   - One for communication from the **parent process to child process 1**.
   - Another for communication from **child process 1 to child process 2**.
2. The **parent process** sends a message to **child process 1**.
3. **Child process 1** receives the message, modifies it (e.g., appends a string), and sends it to **child process 2**.
4. **Child process 2** receives the final message and prints it to the screen.

## Hint
- Use the `pipe()` function to create the pipe.
- Use `fork()` to create a child process.
- Use `read()` and `write()` for inter-process communication.
- Close unused pipe ends in each process to prevent blocking or resource leaks.

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
    └── main.c              # Source code implementing multi-pipe IPC
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
Parent process (PID: 2470) sending a message: Hello TungNHS
Parent sent message to Child 1
Child 1 process (PID: 2471) received message: Hello TungNHS
Child 1 modified message: Hello TungNHS - Nguyen Huu Son Tung
Child 1 sent message to Child 2
Child 1 process terminated
Child 2 process (PID: 2472) received message: Hello TungNHS - Nguyen Huu Son Tung
Child 2 process terminated
Parent: Child 1 exited with status 0
Parent: Child 2 exited with status 0
Parent process terminated
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```