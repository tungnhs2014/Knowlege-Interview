## Exercise 4

This program creates a file, writes some data into it, and retrieves information about the file using `struct stat` in C.

## Features
- Creates a file named "test.txt" in the "file" directory
- Writes a sample text into the file
- Retrieves file details including:
  - File type
  - File name
  - Last modified time
  - File size
  - I-node number
  - Access and modification timestamps
- Displays the retrieved information on the screen

## Project Structure
```
├── bin
│   └── exam                # Executable binary file
├── file
│   └── test.txt            # Generated text file for demonstration
├── Makefile                # Build automation script
├── obj
│   └── main.o              # Compiled object file
└── src
    └── main.c              # Source code implementing file operations
```

## Compilation and Execution
To compile the program, run:
```sh
make
```
This will generate the executable file `bin/exam`.

To execute the program, run:
```sh
./bin/exam
```

## Cleaning Up
To remove compiled files and clean the project, run:
```sh
make clean
```

## Example Output
```sh
File Name: file/test.txt
File Size: 20 bytes
I-node number: 2006454
Last status change:     Thu Mar  6 00:20:50 2025
Last file access:       Thu Mar  6 00:20:50 2025
Last file modification: Thu Mar  6 00:20:50 2025
File Type: Regular file
```

## Technical Details
The program demonstrates several key file-related system calls:
- `open()`: Creates and opens a file with specified permissions
- `write()`: Writes data to the file
- `stat()`: Retrieves file metadata including timestamps and type
- `close()`: Properly closes the file descriptor