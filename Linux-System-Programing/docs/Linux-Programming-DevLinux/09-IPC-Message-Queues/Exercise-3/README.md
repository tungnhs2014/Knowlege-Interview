## Exercise 3: Communication Between Multiple Processes Using POSIX Message Queues

## Description
Write a program that uses **POSIX message queues** to communicate between **three processes**.

## Requirements
1. Create a **message queue** for communication.
2. The parent process sends a string message into the message queue.
3. **Child process 1** receives the string, **converts it to uppercase**, and sends it back through the **same message queue**.
4. **Child process 2** receives the updated string and prints it to the screen.

## Hint
- Use `mq_send()` and `mq_receive()` for message communication.
- Make sure the processes do not block each other during reading/writing.
- Use `mq_close()` and `mq_unlink()` to close and delete the message queue after completion.

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
    └── main.c              # Source code implementing multi-process IPC
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
[Parent] Process ID: 3747
[Parent] Sending message from PID: 3747
[Child 1] Process ID: 3748
[Child 2] Process ID: 3749
Child 2 received message: HELLO TUNGNHS NGUYEN HUU SON TUNG!
[Parent] Child 1 exited with status 0
[Parent] Child 2 exited with status 0
[Parent] Done.
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```