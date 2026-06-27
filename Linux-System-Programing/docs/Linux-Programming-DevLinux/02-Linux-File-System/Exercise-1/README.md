## Exercise 1

- Write a program that opens any file using the O_APPEND flag. Then, perform a seek operation to move to the beginning of the file and write some data to it. Where will the data appear in the file, and why does this happen?

### Answer
- The data will always be written at the end of the file, regardless of the `lseek(fd, 0, SEEK_SET)` call.

### Explanation
- The file is opened with the `O_APPEND` flag, which forces all write operations to append data at the end of the file.
- Even if the program explicitly calls `lseek(fd, 0, SEEK_SET)` to move the file pointer to the beginning, this does not affect the writing position.
- This behavior is part of the POSIX standard and is designed to ensure atomic appends in multi-process environments.

### Project Structure
The project follows this directory structure:
```
├── bin/                    
│   └── examaple_program   # Final executable binary
├── Makefile               # Build automation script
├── src
│   ├── main.c             # Source code demonstrating O_APPEND behavior
├── obj                    # Directory for compiled object files
└── README.md              # Project documentation (this file)
```

### How to Build and Run
#### Build the project:
```sh
make
```
This compiles the source code and generates the executable in `bin/example_program`.

#### Run the program:
```sh
./bin/exam
```
This will create a directory named "file", create a file "linux.txt" inside it, and demonstrate the O_APPEND behavior.

#### Clean the build files:
```sh
make clean
```
This removes all compiled files and directories (`obj/`, `bin/`).