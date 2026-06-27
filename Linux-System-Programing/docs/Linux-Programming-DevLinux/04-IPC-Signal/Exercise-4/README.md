## Exercise 4: Ignoring SIGTSTP Signal

## Description
Write a program that can ignore the **SIGTSTP** signal (commonly triggered by pressing `Ctrl+Z`).

## Requirements
1. Use `signal()` or sigaction() to catch and ignore the SIGTSTP signal.
2. When the **SIGTSTP** signal is received, the program should not stop but continue running and print the message "**SIGTSTP ignored"**.

## Hint
- What happens if the SIGTSTP signal is not handled and the user presses `Ctrl+Z`?

## Question
- What happens if the SIGTSTP signal is not handled and the user presses Ctrl+Z?

**ANSWERS:**
- If the **SIGTSTP** signal is not handled, the default behavior is to suspend the process and move it to the background. The terminal will display a message similar to "[1]+ Stopped <program_name>". To resume the process, the user needs to run `fg`(foreground) or `bg`(background).
- **Example**
```c
#include <stdio.h>
#include <unistd.h>

int main() {
    int counter = 0;
    
    printf("Program started. Press Ctrl+Z to test default SIGTSTP behavior.\n");
    
    while (1) {
        printf("Program is running... Count: %d\n", counter++);
        sleep(1);
    }
    
    return 0;
}
```
- When is running program and press `Ctrl +Z`. Terminal will display a message like:
```bash
[1]+  Stopped                 ./program
```
+ fg: Bring the stopped program back to the foreground to continue running.
+ bg: Resume the stopped program in the background.
+ jobs: List all stopped or background jobs.
+ kill -9 %1: Completely terminate the program (replace %1 with the correct job ID obtained from the jobs command).

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
    └── main.c              # Source code implementing SIGTSTP handling
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
Program started. Press Ctrl + Z to test SIGTSTP handling
Programming is running count: 1
^Z
SIGTSTP ignored
Programming is running count: 3
^Z
SIGTSTP ignored
Programming is running count: 4
^Z
SIGTSTP ignored
Programming is running count: 6
^C
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```