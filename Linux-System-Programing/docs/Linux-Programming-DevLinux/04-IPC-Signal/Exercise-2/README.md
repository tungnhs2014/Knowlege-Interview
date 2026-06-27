## Exercise 2: Using SIGALRM to Create a Timer

## Description
This project implements a timer using the **SIGALRM** signal. The program sets an alarm to trigger every second and counts the elapsed time up to 10 seconds.

## Requirements
**1. Setting the Alarm:**
- Use the `alarm()` function to trigger **SIGALRM** every second.

**2. Signal Handling:**
- Use the `signal()` function to register the handler for **SIGALRM**.
- Increase the timer count and print the message:
``` bash
Timer: <seconds> seconds
```
**3. Stopping the Program:**
- Stop the timer and exit after counting 10 seconds.

## Hint
- Use the `signal()` function to register the signal handler for **SIGALRM**, and set `alarm(1)` to make the timer repeat every second.

## Question
- Question: What happens if alarm(1) is not called again inside the signal handler?

**ANSWERS:**
- The **SIGALRM** signal will be triggered only once (after the initial call in the main function).
- After handling the first **SIGALRM** signal, the handler will not be called again.
- The program will count up to 1 second and then stop counting (although it will continue running).
- This happens because the alarm() function sets up a one-shot timer, not an automatic repeating timer.

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
    └── main.c              # Source code implementing timer with SIGALRM
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
Timer program started. Will count to 10 seconds.

Time: 1 seconds

Time: 2 seconds

Time: 3 seconds

Time: 4 seconds

Time: 5 seconds

Time: 6 seconds

Time: 7 seconds

Time: 8 seconds

Time: 9 seconds

Time: 10 seconds
Timer reached 10 seconds. Exiting program...
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```