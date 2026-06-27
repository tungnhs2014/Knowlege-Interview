## Exercise 3: Count Characters Using Pipe Between Parent and Child Process

## Description
Write a program that uses a **pipe to count** the number of characters in a string.

## Requirements
1. Create a `pipe` and `fork` a child process.
2. The parent process will send a string into the pipe.
3. The child process will read the string from the pipe, count the number of characters, and print the result.

## Hint
- Use the `pipe()` function to create the pipe.
- Use `fork()` to create a child process.
- Use `read()` and `write()` to send and receive data through the pipe.
- Use the `strlen()` function to count the number of characters in the received string.

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
    └── main.c              # Source code implementing character counting via pipe
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
Parent process (PID: 3620) sending a message: Welcome Nguyen Huu Son Tung
Child process (PID: 3621) received a message: Welcome Nguyen Huu Son Tung
Child process counted 27 characters in the received string
Child process terminated
Parent: Child exited with status 0
Parent process terminated
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```