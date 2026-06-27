# Exercise 2: Using Excel and Environment Variables

## Description
This project implements a C program where the child process replaces the running program with another program (e.g., `ls` or `date`) using the `execlp()` or `execvp()` function.

## Requirements
1. Pass environment variables to the program to perform different tasks based on the input:  
    - If the passed environment variable is **1**, the child process will execute the **`ls`** command.
    - If the passed environment variable is **2**, the child process will execute the **`date`** command.
  
2. Explanation: What happens to the original process after exec is called?
    **ANSWERS**: 
    - After successfully calling the `exec` function, the current process image is **replaced** by the new program. 
    - As a result:
    + No further code from the original process will be executed.
    + If the `exec` call fails, the original process will continue running.

---

## Project Structure
```
├── bin/         
│   └── exam                # Executable binary file
├── Makefile                # Build automation script
├── obj/         
│   └── main.o              # Compiled object file
└── src/        
    └── main.c              # Source code implementing exec functions
```

## Build and Run Instructions

### Build the program
```bash
$ make
```
### Run the program
```bash
./bin/exam 1    # To execute the 'ls' command
./bin/exam 2    # To execute the 'date' command
```
### Clean the project
```bash
$ make clean
```