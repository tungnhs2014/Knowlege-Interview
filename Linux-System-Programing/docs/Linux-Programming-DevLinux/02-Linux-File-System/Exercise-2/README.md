## Exercise 2

After each `write` command below, determine which file the content will be written to, what the content will be, and explain why.

```c
fd1 = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
fd3 = open(file, O_RDWR);
write(fd1, "Hello,", 6);
write(fd2, "world", 6);
lseek(fd2, 0, SEEK_SET);
write(fd1, "HELLO,", 6);
write(fd3, "Gidday", 6);
```

## Answer

### File Descriptor Creation
```c
fd1 = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
```
- Opens or creates a file with read and write permissions for the user only
- If the file exists, its contents are truncated (set to zero length)
- File position for fd1 is set to offset 0

```c
fd3 = open(file, O_RDWR);
```
- Opens the same file again with read/write permissions
- Creates a new file descriptor (fd3) pointing to the same file
- File position for fd3 is set to offset 0 (independent of fd1)

### Writing Operations

```c
write(fd1, "Hello,", 6);
```
- Writes 6 bytes ("Hello,") at the beginning of the file using fd1
- File content after this operation: `Hello,`
- File position for fd1 advances to offset 6, fd3 remains at offset 0

```c
write(fd2, "world", 6);
```
- **Error**: fd2 was never initialized with open()
- The write operation fails, nothing is written to the file
- File content remains: `Hello,`
- File positions remain unchanged (fd1 at offset 6, fd3 at offset 0)

```c
lseek(fd2, 0, SEEK_SET);
```
- **Error**: fd2 is invalid, so lseek() fails
- No change to any file position
- File content remains: `Hello,`

```c
write(fd1, "HELLO,", 6);
```
- Writes 6 bytes ("HELLO,") at offset 6 using fd1
- This appends after the existing content
- File content after this operation: `Hello,HELLO,`
- File position for fd1 advances to offset 12, fd3 still at offset 0

```c
write(fd3, "Gidday", 6);
```
- Writes 6 bytes ("Gidday") at the beginning of the file using fd3
- This overwrites the first 6 bytes of the file
- Final file content: `GiddayHELLO,`