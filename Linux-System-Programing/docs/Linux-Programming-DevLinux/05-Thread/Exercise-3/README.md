## Exercise 3: Using Condition Variables

## Description
Write a program with two threads: producer and consumer, as follows:

## Instructions
1. **Producer:** Generates a random number between 1 and 10 and stores it in a global variable called `data`.
2. **Consumer:** Waits for the producer to provide data, then reads and prints the data.
3. Use `pthread_cond_wait` and `pthread_cond_signal` to synchronize between producer and consumer, ensuring the consumer reads the data only after the producer has generated it.
4. Repeat the above process **10 times** and print all the values read by the consumer.

## Hint
- Use `pthread_cond_wait` to make the consumer wait until it receives a signal from the producer indicating that the data is ready.

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
    └── main.c              # Source code implementing condition variables
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

### Example output
```bash
Starting producer-consumer demonstration with condition variables
Will perform 10 iterations of produce-consume cycle
Producer: Generated data = 7
Consumer: Consumed data = 7
Producer: Generated data = 3
Consumer: Consumed data = 3
Producer: Generated data = 9
Consumer: Consumed data = 9
Producer: Generated data = 2
Consumer: Consumed data = 2
Producer: Generated data = 5
Consumer: Consumed data = 5
Producer: Generated data = 10
Consumer: Consumed data = 10
Producer: Generated data = 1
Consumer: Consumed data = 1
Producer: Generated data = 6
Consumer: Consumed data = 6
Producer: Generated data = 4
Consumer: Consumed data = 4
Producer: Generated data = 8
Consumer: Consumed data = 8

All values read:
Value: 7        Value: 3        Value: 9        Value: 2        Value: 5        Value: 10       Value: 1        Value: 6        Value: 4        Value: 8

All iterations completed successfully
```

### Clean the project
To remove all compiled files and binaries, run:
```bash
$ make clean
```