## Exercise 1: Process Creation and Monitoring

## Description
Write a C program to create a child process using fork(). After creating the process, the child process should print its own PID, and the parent process should print the PID of the child process.

---

## Project Structure
```
├── bin/         
│   └── exam                # Executable binary file
├── Makefile                # Build automation script
├── obj/         
│   └── main.o              # Compiled object file
└── src/        
    └── main.c              # Source code implementing fork()
```

## Build and Run Instructions

### Build the program
```bash
$ make
```
### Run the program
```bash
./bin/exam
```
### Clean the project
```bash
$ make clean
```
### Example output
```
Parent process:
PID of parent process: 12345
PID of child process: 12346

Child process:
PID of child process: 12346
PID of parent process: 12345

Child process has completed
```