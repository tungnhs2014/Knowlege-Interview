## Exercise 3

This program takes exactly 3 command-line arguments to either read from or write to a specified file.

## Usage
```bash
$ ./example_program filename num-bytes [r/w] "Hello"
```

### Parameters:
1. `example_program`: The name of the executable file.
2. `filename`: The name of the file.
3. `num-bytes`: The number of bytes to read/write.
4. `[r/w]`:  
   - `r`: Reads from `filename` and prints the content to the screen.
   - `w`: Writes to `filename`.
5. `"Hello"`: Any content to be read/written to the file.


## Project Structure
```
├── bin/         
│   └── example_program
├── file/        
│   └── linux.txt
├── Makefile     
├── obj/         
│   └── main.o
└── src/        
    └── main.c
```

## Build and Run Instructions

### Build the project
```bash
$ make
```

### Example Usage

#### Writing to a file
```bash
$ ./bin/example_program linux.txt 11 w "Hello Linux"
```
Expected output:
```
Written 11 to file: Hello Linux
```

#### Reading from a file
```bash
$ ./bin/example_program linux.txt 11 r
```
#### Expected output:
```
Read from file: Hello Linux
```

### Clean the project
```bash
$ make clean
```


