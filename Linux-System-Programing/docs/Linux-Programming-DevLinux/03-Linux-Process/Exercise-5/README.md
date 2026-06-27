## Exercise 5: Creating Zombie and Orphan Processes

## Description
This project demonstrates how to create **zombie** and **orphan** processes in Linux to understand these special process states.

---

## Requirements
1. **Creating a Zombie Process:**
   - Make the parent process terminate early while the child process is still running.
   - Use the **ps** command to observe the **zombie** state of the process.

2. **Creating an Orphan Process:**
   - Make the child process sleep for a longer period than the parent process.
   - Observe how the child process becomes an **orphan** and is adopted by the **init** process.

3. **Explanation:**
   - Clearly explain the zombie and orphan process states and why they appear in Linux.

   **ANSWERS:**
   - **Zombie Process:**
      + A zombie process is a child process that has terminated, but its exit status has not been collected by the parent process using wait() or waitpid().
      + It still exists in the process table to hold the exit status.
      + The status of a zombie process in the STAT column is Z or Z+.
   - **Orphan Process:**
      + An orphan process is a child process whose parent process has terminated while the child is still running.
      + In this case, the init process (PID = 1) adopts the orphan process.
      + An orphan process will have PPID = 1.

   **Why They Appear:**
     - **Zombie:** Occurs because the parent process fails to collect the exit status of the terminated child, causing the    child to remain as a zombie.
     - **Orphan:** Occurs when the parent process terminates before the child, making the child an orphan adopted by the init process.

---

## Project Structure
```
├── bin
│   ├── orphan              # Executable demonstrating orphan process
│   └── zombie              # Executable demonstrating zombie process
├── Exercise-5.md
├── Makefile                # Build automation script
├── obj
│   ├── orphan.o            # Compiled object file for orphan
│   └── zombie.o            # Compiled object file for zombie
└── src
    ├── orphan.c            # Source code creating an orphan process
    └── zombie.c            # Source code creating a zombie process
```

---

## Build and Run Instructions

### Build the program
To build both the Zombie and Orphan executables, run:
```bash
$ make
```

### Run the Zombie Program
To execute the Zombie program, use:
```bash
$ ./bin/zombie
```
While the program is running, open another terminal and run:
```bash
$ ps aux | grep -iE zombie
```
to observe the zombie process (it will show a status "Z").

### Run the Orphan Program
To execute the Orphan program:
```bash
$ ./bin/orphan
```
After running, use:
```bash
$ ps -ef | grep -iE orphan
```
to verify that the child process's parent (PPID) is now 1 (adopted by init).

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```