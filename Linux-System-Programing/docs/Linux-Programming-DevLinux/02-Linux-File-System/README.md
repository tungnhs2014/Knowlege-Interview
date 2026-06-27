# 2. Linux File System

## 2.1. Introduction to the Linux File System

### 2.1.1. Everything is a File

In Linux, a core philosophy is: **"Everything is a file."** This fundamental concept simplifies how the system handles different resources:

- **Regular files**: Text documents, binary programs, images, videos
- **Directories**: Special files that contain lists of other files
- **Hardware devices**: Represented as files in `/dev` directory
- **Network interfaces**: Accessible through files in `/sys` directory
- **Processes**: Visible as files in `/proc` directory
- **Sockets and pipes**: Used for inter-process communication

This unified approach allows users and programs to interact with system resources using consistent file operations (open, read, write, close).

To view detailed information about files and directories, use:
```sh
ls -l
```

Example output:
```
drwxr-xr-x  2 user user 4096 Mar 09 12:00 Documents
-rw-r--r--  1 user user 1234 Mar 09 12:34 file.txt
lrwxrwxrwx  1 user user   11 Mar 09 12:40 shortcut -> /home/user/file
crw-rw-rw-  1 root root 1, 3 Mar 09 12:50 /dev/null
brw-rw----  1 root disk 8, 1 Mar 09 13:00 /dev/sda1
```

The first character of each line indicates the file type, followed by permissions, link count, owner, group, size, modification time, and name.

### 2.1.2. Types of Files in Linux

Linux categorizes files into seven distinct types, each serving a specific purpose:

| **File Type**         | **Symbol** | **Description** | **Example** |
|----------------------|------------|----------------|-------------|
| **Regular File**     | `-`        | Normal data files containing text, programs, images, etc. | `/home/user/document.txt` |
| **Directory**        | `d`        | Special files that contain lists of other files | `/home/user/Documents` |
| **Character Device** | `c`        | Provides unbuffered, direct access to hardware | `/dev/tty` (terminal) |
| **Block Device**     | `b`        | Provides buffered access to hardware devices | `/dev/sda` (hard disk) |
| **Symbolic Link**    | `l`        | Points to another file or directory | `/bin/sh -> bash` |
| **Socket**          | `s`        | Enables communication between processes | `/run/systemd/private` |
| **Named Pipe (FIFO)**| `p`        | Allows data transfer between processes | `/tmp/my_pipe` |

To determine a file's type, you can use these commands:
```sh
# View file type with ls
ls -l filename

# Get detailed file information
file filename
```

<p align="center">
  <img src="https://github.com/user-attachments/assets/ef4f69fa-8d19-48e1-8e0b-cf54d7ecc0f0" alt="Linux File Types" width="70%">
</p>

### 2.1.3. Linux File System Hierarchy

The Linux file system is organized in a hierarchical tree structure, starting from the root directory (`/`). Each directory serves a specific purpose in the system:

| **Directory** | **Purpose** | **Contains** |
|-------------|-------------|-------------|
| `/` | **Root directory** | The top-level directory containing all other files and directories |
| `/bin` | **Essential binaries** | Core command-line utilities required for system boot and repair |
| `/boot` | **Boot files** | Kernel images and boot loader files needed to start the system |
| `/dev` | **Device files** | Files representing hardware devices and peripherals |
| `/etc` | **System configuration** | System-wide configuration files and startup scripts |
| `/home` | **User home directories** | Personal directories for regular users |
| `/lib` | **Shared libraries** | Common code used by programs and essential for the system |
| `/media` | **Removable media** | Mount points for removable devices like USB drives and DVDs |
| `/mnt` | **Temporary mounts** | Temporarily mounted filesystems |
| `/opt` | **Optional software** | Third-party applications installed separately from the system |
| `/proc` | **Process information** | Virtual filesystem providing information about running processes |
| `/root` | **Root user home** | Home directory for the system administrator (root user) |
| `/run` | **Runtime data** | Information about the running system since last boot |
| `/sbin` | **System binaries** | Administrative commands for system maintenance |
| `/srv` | **Service data** | Data for services provided by the system |
| `/sys` | **System information** | Virtual filesystem exposing kernel parameters and hardware information |
| `/tmp` | **Temporary files** | Files that are deleted on system reboot |
| `/usr` | **User programs** | Applications, libraries, and documentation installed by the package manager |
| `/var` | **Variable data** | Files that change during normal operation (logs, databases, mail) |

To explore the directory structure:
```sh
# List directories under root
ls /

# Display directory tree (requires 'tree' package)
tree -L 1 /

# Show disk usage of directories
du -h --max-depth=1 /
```

### 2.1.4. File Permissions and Ownership

Linux enforces a robust permission system that controls who can access files and what actions they can perform. Each file has three permission sets:

- **Owner (u)**: The user who created or owns the file
- **Group (g)**: A collection of users with shared access rights
- **Others (o)**: All other users on the system

For each set, three types of permissions can be granted:

| **Permission** | **Symbol** | **Numeric Value** | **For Files** | **For Directories** |
|---------------|-----------|-----------------|--------------|---------------------|
| **Read**      | `r`       | 4               | View content | List contents       |
| **Write**     | `w`       | 2               | Modify content | Create/delete files |
| **Execute**   | `x`       | 1               | Run as program | Enter directory    |
| **No access** | `-`       | 0               | Access denied | Access denied      |

#### Reading Permission Strings

In `ls -l` output, permissions appear as a 10-character string:
```
-rwxr-xr--
```

- First character: File type (`-` for regular file, `d` for directory)
- Characters 2-4: Owner permissions (`rwx` = read, write, execute)
- Characters 5-7: Group permissions (`r-x` = read, execute only)
- Characters 8-10: Others permissions (`r--` = read only)

#### Changing Permissions

The `chmod` command changes file permissions using either symbolic or numeric notation:

**Symbolic notation**:
```sh
# Add write permission for owner
chmod u+w file.txt

# Remove read permission from group
chmod g-r file.txt

# Give execute permission to everyone
chmod a+x script.sh
```

**Numeric notation**:
```sh
# Set rwxr-xr-- permissions (764)
chmod 764 file.txt

# Set rwx------ permissions (700)
chmod 700 private_directory
```

#### Changing Ownership

The `chown` command changes file ownership:
```sh
# Change file owner
chown username file.txt

# Change both owner and group
chown username:groupname file.txt

# Change ownership recursively for directories
chown -R username:groupname directory/
```

Special permission bits also exist for advanced use cases:

| **Special Permission** | **Symbol** | **Numeric** | **Effect** |
|----------------------|-----------|------------|-----------|
| **Set User ID (SUID)** | `s` in owner execute | 4000 | Executes with owner's permissions |
| **Set Group ID (SGID)** | `s` in group execute | 2000 | Executes with group's permissions |
| **Sticky Bit** | `t` in others execute | 1000 | Only owner can delete files (common for /tmp) |

### 2.1.5. Disk Management and Partitioning

Linux provides powerful tools for managing storage devices, partitions, and filesystems.

#### Viewing Disk Information

```sh
# List block devices with details
lsblk

# Show disk partitions
fdisk -l

# Display disk usage
df -h

# Show directory space usage
du -sh /path/to/directory
```

#### Partitioning Disks

Partitioning divides a physical disk into logical sections:

```sh
# Interactive partition tool
fdisk /dev/sdb

# Modern partition manager
parted /dev/sdb

# Create a partition table and partitions
gdisk /dev/sdb  # For GPT partitions
```

Basic fdisk workflow:
1. Start `fdisk` on the target device
2. Press `p` to print the current partition table
3. Press `n` to create a new partition
4. Specify partition details (primary/extended, size)
5. Press `w` to write changes and exit

#### Creating Filesystems

After partitioning, you must format with a filesystem:

```sh
# Create ext4 filesystem
mkfs.ext4 /dev/sdb1

# Create XFS filesystem
mkfs.xfs /dev/sdb2

# Create FAT32 filesystem (for compatibility)
mkfs.vfat -F 32 /dev/sdb3
```

#### Mounting Filesystems

Mount filesystems to access their contents:

```sh
# Create mount point
mkdir /mnt/data

# Mount temporarily
mount /dev/sdb1 /mnt/data

# Unmount when finished
umount /mnt/data
```

For permanent mounts, add an entry to `/etc/fstab`:
```
/dev/sdb1  /mnt/data  ext4  defaults  0  2
```

#### Checking and Repairing Filesystems

```sh
# Check and repair ext4 filesystem
fsck.ext4 -f /dev/sdb1

# Check XFS filesystem
xfs_repair /dev/sdb2
```

### 2.1.6. Linux Filesystem Types

Linux supports various filesystem types, each with different features and use cases:

| **Filesystem** | **Features** | **Best For** | **Limitations** |
|---------------|-------------|-------------|----------------|
| **ext4** | Journaling, large file support, backward compatibility | General purpose, default in many distributions | Less advanced than newer filesystems |
| **XFS** | High performance, scalability, online resizing | Large files, databases, media storage | Cannot shrink partitions |
| **Btrfs** | Copy-on-write, snapshots, checksums, compression | Data integrity, snapshots, SSD optimization | Still maturing, complex |
| **F2FS** | Optimized for flash storage | SSDs, flash drives, mobile devices | Limited tools compared to ext4 |
| **NTFS** | Windows compatibility | Dual-boot systems, Windows data exchange | Limited Linux performance |
| **FAT32/exFAT** | Wide compatibility | External drives, cross-platform sharing | Limited permissions, file size limits |
| **ZFS** | Advanced data protection, compression, snapshots | Enterprise storage, data servers | High memory requirements |
| **tmpfs** | Memory-based, no persistence | Temporary files, performance-critical cache | Data lost on power loss |

To check the filesystem type of mounted partitions:
```sh
df -T
```

To check filesystem type of a specific device:
```sh
blkid /dev/sdb1
```

### 2.1.7. Virtual Filesystems

Linux uses special virtual filesystems that don't correspond to physical storage but provide interfaces to system information:

#### /proc Filesystem

The `/proc` filesystem provides a window into the kernel's view of the system:

```sh
# CPU information
cat /proc/cpuinfo

# Memory statistics
cat /proc/meminfo

# Kernel version
cat /proc/version

# Currently mounted filesystems
cat /proc/mounts

# Process-specific information
ls /proc/PID/
```

The `/proc` directory contains numerical subdirectories for each running process, allowing inspection of process memory maps, file descriptors, and status.

#### /sys Filesystem

The `/sys` filesystem exposes kernel objects, device drivers, and hardware information:

```sh
# List all network interfaces
ls /sys/class/net/

# Get CPU information
ls /sys/devices/system/cpu/

# View block device information
ls /sys/block/

# Check USB devices
ls /sys/bus/usb/devices/
```

#### /dev Filesystem

The `/dev` filesystem contains device files for hardware interaction:

```sh
# Storage devices
ls /dev/sd*

# Terminal devices
ls /dev/tty*

# Special device files
ls -la /dev/null /dev/zero /dev/random
```

Important special files:
- `/dev/null`: Discards all data written to it
- `/dev/zero`: Provides unlimited stream of zero bytes
- `/dev/random` and `/dev/urandom`: Provide random data

--- 

## 2.2. System Calls for File Operations

### 2.2.1. Introduction to System Calls

System calls provide the interface between user applications and the Linux kernel. They allow programs to request services from the kernel, such as file operations, process management, and network communication.

Key characteristics of system calls:
- Execute with kernel privileges
- Provide a controlled interface to system resources
- Enable secure access to hardware and data
- Form the foundation of all file and process operations

The typical workflow for a system call:
1. User program prepares arguments
2. CPU switches from user mode to kernel mode
3. Kernel performs the requested operation
4. CPU switches back to user mode
5. Program continues execution with the result

### 2.2.2. File Management System Calls

These system calls handle file operations:

| **System Call** | **Purpose** | **Common Usage** |
|----------------|------------|-----------------|
| `open()` | Opens or creates a file | `int fd = open("file.txt", O_RDWR);` |
| `read()` | Reads data from a file descriptor | `read(fd, buffer, size);` |
| `write()` | Writes data to a file descriptor | `write(fd, buffer, size);` |
| `lseek()` | Repositions the file offset | `lseek(fd, offset, SEEK_SET);` |
| `close()` | Closes a file descriptor | `close(fd);` |
| `unlink()` | Removes a file | `unlink("file.txt");` |
| `rename()` | Renames a file | `rename("old.txt", "new.txt");` |
| `stat()` | Gets file status | `stat("file.txt", &stat_buf);` |
| `chmod()` | Changes file permissions | `chmod("file.txt", 0644);` |
| `mkdir()` | Creates a directory | `mkdir("new_dir", 0755);` |
| `rmdir()` | Removes a directory | `rmdir("empty_dir");` |

#### Example: File Copy Implementation

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s source destination\n", argv[0]);
        return 1;
    }

    // Open source file for reading
    int source_fd = open(argv[1], O_RDONLY);
    if (source_fd == -1) {
        perror("Failed to open source file");
        return 1;
    }

    // Create destination file for writing
    int dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("Failed to create destination file");
        close(source_fd);
        return 1;
    }

    // Copy data
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    
    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Write error");
            close(source_fd);
            close(dest_fd);
            return 1;
        }
    }

    if (bytes_read == -1) {
        perror("Read error");
        close(source_fd);
        close(dest_fd);
        return 1;
    }

    // Close files
    close(source_fd);
    close(dest_fd);
    printf("File copied successfully.\n");
    
    return 0;
}
```

### 2.2.3. Process Management System Calls

These system calls manage processes and execution:

| **System Call** | **Purpose** | **Common Usage** |
|----------------|------------|-----------------|
| `fork()` | Creates a new process by duplicating the calling process | `pid_t pid = fork();` |
| `exec()` family | Replaces current process image with a new one | `execl("/bin/ls", "ls", "-l", NULL);` |
| `wait()` family | Waits for a child process to terminate | `wait(&status);` |
| `exit()` | Terminates the calling process | `exit(EXIT_SUCCESS);` |
| `getpid()` | Gets process ID | `pid_t pid = getpid();` |
| `getppid()` | Gets parent process ID | `pid_t parent = getppid();` |
| `kill()` | Sends a signal to a process | `kill(pid, SIGTERM);` |

#### Example: Process Creation and Command Execution

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d)\n", getpid());
        execl("/bin/ls", "ls", "-l", NULL);
        
        // execl only returns on error
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        printf("Parent process (PID: %d), child PID: %d\n", getpid(), pid);
        
        int status;
        wait(&status);
        
        if (WIFEXITED(status)) {
            printf("Child exited with status: %d\n", WEXITSTATUS(status));
        }
    }
    
    return 0;
}
```

### 2.2.4. Memory Management System Calls

These system calls handle memory allocation and management:

| **System Call** | **Purpose** | **Common Usage** |
|----------------|------------|-----------------|
| `brk()` / `sbrk()` | Changes the program break (heap end) | `sbrk(increment);` |
| `mmap()` | Maps files or devices into memory | `ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);` |
| `munmap()` | Unmaps memory regions | `munmap(ptr, size);` |
| `mprotect()` | Changes protection on memory region | `mprotect(ptr, size, PROT_READ);` |
| `msync()` | Synchronizes a mapped memory region | `msync(ptr, size, MS_SYNC);` |

#### Example: Memory-Mapped File Access

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    // Map file into memory
    char *mapped = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // File descriptor can be closed now, mapping remains valid
    close(fd);

    // Print the file contents
    printf("File contents: %.*s\n", (int)sb.st_size, mapped);

    // Modify the mapped memory (this will change the file)
    if (sb.st_size >= 10) {
        memcpy(mapped, "MODIFIED: ", 10);
        
        // Ensure changes are written back to the file
        if (msync(mapped, sb.st_size, MS_SYNC) == -1) {
            perror("msync");
        }
    }

    // Unmap when done
    if (munmap(mapped, sb.st_size) == -1) {
        perror("munmap");
        return 1;
    }

    printf("File modified successfully.\n");
    return 0;
}
```

### 2.2.5. Inter-Process Communication (IPC) System Calls

These system calls facilitate communication between processes:

| **System Call** | **Purpose** | **Common Usage** |
|----------------|------------|-----------------|
| `pipe()` | Creates an unnamed pipe | `pipe(fd);` |
| `mkfifo()` | Creates a named pipe (FIFO) | `mkfifo("myfifo", 0666);` |
| `socket()` | Creates a network socket | `sockfd = socket(AF_INET, SOCK_STREAM, 0);` |
| `shmget()` | Allocates shared memory segment | `shmid = shmget(KEY, size, flags);` |
| `shmat()` | Attaches shared memory segment | `ptr = shmat(shmid, NULL, 0);` |
| `shmdt()` | Detaches shared memory segment | `shmdt(ptr);` |
| `msgget()` | Creates/accesses a message queue | `msgid = msgget(KEY, flags);` |
| `msgsnd()` | Sends a message to a queue | `msgsnd(msgid, &msg, size, flags);` |
| `msgrcv()` | Receives a message from a queue | `msgrcv(msgid, &msg, size, type, flags);` |

#### Example: Inter-Process Communication Using Pipes

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    int pipe_fd[2];
    pid_t pid;
    char buffer[100];
    
    // Create pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Create child process
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        // Parent process - writer
        close(pipe_fd[0]);  // Close unused read end
        
        const char *message = "Hello from parent process!";
        write(pipe_fd[1], message, strlen(message) + 1);
        printf("Parent wrote: %s\n", message);
        
        close(pipe_fd[1]);  // Close write end
        wait(NULL);  // Wait for child
    } else {
        // Child process - reader
        close(pipe_fd[1]);  // Close unused write end
        
        read(pipe_fd[0], buffer, sizeof(buffer));
        printf("Child read: %s\n", buffer);
        
        close(pipe_fd[0]);  // Close read end
        exit(EXIT_SUCCESS);
    }
    
    return 0;
}
```
---

## 2.3. File Management Internals

### 2.3.1. Page Cache

The Linux page cache is a mechanism that improves file I/O performance by storing recently accessed data in unused portions of system memory (RAM). This reduces disk access frequency and improves system responsiveness.

<p align="center">
  <img src="https://github.com/user-attachments/assets/ee94175f-6695-42aa-9bbc-a9fd93bac091" alt="Page Cache" width="70%">
</p>

#### How Page Cache Works

1. **During Read Operations**:
   - When data is requested, the kernel first checks if it's already in the page cache
   - If found (cache hit), data is returned directly from RAM without disk access
   - If not found (cache miss), data is read from disk, stored in the cache, then returned

2. **During Write Operations**:
   - Data is first written to the page cache, not immediately to disk
   - Modified pages are marked as "dirty"
   - Dirty pages are written to disk periodically or when explicitly requested

#### Page Cache Benefits

- **Improved read performance**: Frequently accessed data is available in memory
- **Write buffering**: Allows grouping of small writes into larger, more efficient operations
- **Read-ahead**: The kernel can prefetch additional data that might be needed soon
- **Reduced I/O operations**: Minimizes physical disk access, extending SSD lifespan

#### Cache Synchronization

The page cache uses delayed writes to improve performance, but this creates a risk of data loss during system crashes. Linux provides several ways to ensure data is written to disk:

```c
// In C programs
#include <unistd.h>

// Sync all dirty pages to disk
sync();

// Sync only a specific file's data
fsync(fd);

// Sync file data but not metadata
fdatasync(fd);
```

From the command line:
```sh
# Sync all dirty pages to disk
sync

# Drop clean caches to free memory
echo 1 > /proc/sys/vm/drop_caches

# Write dirty pages older than 30 seconds
sysctl -w vm.dirty_expire_centisecs=3000
```

#### Page Cache Monitoring

```sh
# View current cache statistics
cat /proc/meminfo | grep -i cache

# Show cached pages per file system
cat /proc/sys/vm/nr_unstable

# View I/O statistics including cache hits/misses
iostat -x 1
```

### 2.3.2. File Management Tables

Linux uses a sophisticated system of tables and data structures to manage files. Understanding these components helps explain how files are tracked, accessed, and modified.

#### I-node Table

An i-node (index node) contains all the metadata about a file except its name:

| **I-node Field** | **Description** |
|-----------------|----------------|
| File mode | Type and permissions |
| Owner IDs | User and group owners |
| Size | File size in bytes |
| Timestamps | Access, modify, change times |
| Link count | Number of hard links to this i-node |
| Data block pointers | Direct, indirect, and double indirect pointers |
| Extended attributes | Additional metadata |

I-nodes have these key properties:
- Each filesystem has its own i-node table
- Each file has exactly one i-node
- Multiple filenames (hard links) can point to the same i-node
- I-node numbers are unique within a filesystem

<p align="center">
  <img src="https://github.com/user-attachments/assets/5d0bf71a-afd6-4567-926a-0eb4e6dfdfb0" alt="I-node Structure" width="70%">
</p>

#### Open File Table

The Open File Table is a kernel-wide data structure that tracks all files currently opened by any process:

| **Field** | **Description** |
|----------|----------------|
| File offset | Current position in the file |
| File status flags | O_RDONLY, O_WRONLY, O_RDWR, etc. |
| I-node pointer | Link to the file's i-node |
| Access mode | Read, write, or both |
| Reference count | Number of file descriptors referencing this entry |

When a file is opened:
1. Kernel checks permissions and locates the i-node
2. Creates a new entry in the Open File Table
3. Sets file offset to 0 (or file end if O_APPEND is used)
4. Returns a file descriptor to the process

<p align="center">
  <img src="https://github.com/user-attachments/assets/7226d6da-7d56-46e2-a9f4-8e7c0f1c3d48" alt="Open File Table" width="70%">
</p>

#### File Descriptor Table

Each process has its own File Descriptor Table:

| **Field** | **Description** |
|----------|----------------|
| File descriptor number | Small non-negative integer (starting from 0) |
| File descriptor flags | Close-on-exec flag (FD_CLOEXEC) |
| Pointer to Open File Table entry | Reference to corresponding open file |

Standard file descriptors:
- 0: Standard input (stdin)
- 1: Standard output (stdout)
- 2: Standard error (stderr)

<p align="center">
  <img src="https://github.com/user-attachments/assets/c8c186b7-e0e0-4dcb-b42c-9215260128ad" alt="File Descriptor Table" width="70%">
</p>

#### Complete File Opening Process

When a process calls `open()`:

1. **System Call Entry**:
   - Process invokes `open()` system call with filename and flags
   - Control transfers to kernel mode

2. **Path Resolution**:
   - Kernel parses the file path component by component
   - Checks permissions at each directory level
   - Locates the file's i-node

3. **Permission Checking**:
   - Verifies the process has permissions for requested access
   - Handles special cases (e.g., O_CREAT flag)

4. **Open File Table Entry**:
   - Creates new entry in Open File Table
   - Initializes file position pointer
   - Sets access mode and status flags
   - Creates link to i-node

5. **File Descriptor Allocation**:
   - Finds lowest unused file descriptor in process's table
   - Creates entry pointing to Open File Table entry

6. **System Call Return**:
   - Returns file descriptor number to process
   - Process now uses this descriptor for subsequent operations

<p align="center">
  <img src="https://github.com/user-attachments/assets/ccd92654-3fd3-4ec2-ab34-757622a7ccd1" alt="File Opening Process" width="70%">
</p>

### 2.3.3. File Access and I/O Models

Linux supports different I/O models to handle various performance and programming needs:

#### Blocking I/O

- **Default behavior**: Process waits until operation completes
- **Advantages**: Simple programming model, efficient for most cases
- **Disadvantages**: Process cannot do other work while waiting
- **Example**: `read()` call that waits until data is available

```c
// Blocking read example
ssize_t bytes = read(fd, buffer, sizeof(buffer));
// Execution continues only after data is read
```

#### Non-blocking I/O

- **Optional behavior**: Operations return immediately, even if incomplete
- **Advantages**: Process can do other work, no waiting
- **Disadvantages**: Requires frequent checking for completion
- **Example**: Using O_NONBLOCK flag with open() and read()

```c
// Open file in non-blocking mode
int fd = open("file.txt", O_RDONLY | O_NONBLOCK);

// Non-blocking read
ssize_t bytes = read(fd, buffer, sizeof(buffer));
if (bytes == -1 && errno == EAGAIN) {
    // No data available right now, do something else
    printf("No data available yet\n");
}
```

#### Memory-mapped I/O

- **Alternative approach**: Maps file contents directly to memory
- **Advantages**: Simpler programming model, potentially better performance
- **Disadvantages**: More complex for large files, requires careful synchronization
- **Example**: Using mmap() to access file data as memory

```c
// Memory-mapped file access
void *addr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, 
                  MAP_SHARED, fd, 0);
if (addr != MAP_FAILED) {
    // Access file as if it were memory
    memcpy(addr, "New data", 8);
    
    // Ensure changes are written to disk
    msync(addr, file_size, MS_SYNC);
    
    // Unmap when done
    munmap(addr, file_size);
}
```

#### Asynchronous I/O

- **Advanced approach**: Operations start in background, notify when complete
- **Advantages**: True parallelism, efficient for many concurrent operations
- **Disadvantages**: Complex programming model, less portable
- **Example**: Using Linux AIO interface

```c
// Asynchronous I/O example (simplified)
struct aiocb cb;
memset(&cb, 0, sizeof(cb));
cb.aio_fildes = fd;
cb.aio_buf = buffer;
cb.aio_nbytes = sizeof(buffer);
cb.aio_offset = 0;

// Start asynchronous read
if (aio_read(&cb) == 0) {
    // Do other work while I/O happens in background
    
    // Check if operation is complete
    while (aio_error(&cb) == EINPROGRESS) {
        // Still in progress, do more work
    }
    
    // Get result when complete
    ssize_t bytes = aio_return(&cb);
}
```
---

## 2.4. File Locking in Linux

File locking provides mechanisms to coordinate access to files by multiple processes, preventing data corruption and race conditions.

### 2.4.1. Overview and Purpose

File locking serves several critical purposes:

- **Data integrity**: Prevents simultaneous writes that could corrupt data
- **Race condition prevention**: Ensures operations happen in the correct order
- **Controlled access**: Allows multiple readers but exclusive writers
- **Process synchronization**: Coordinates activities between unrelated processes

Linux implements two primary types of locks:

1. **Advisory locks**: Processes must voluntarily check and respect locks
2. **Mandatory locks**: Enforced by the kernel, though rarely used in Linux

### 2.4.2. How File Locking Works

When a process requests a file lock:

1. **Lock state registration**:
   - The kernel records lock information in the file's i-node
   - Information includes process ID, lock type, and affected byte range

2. **Lock compatibility checking**:
   - For new lock requests, the kernel checks compatibility with existing locks
   - Multiple read locks can coexist (shared locks)
   - Write locks require exclusive access (exclusive locks)

3. **Lock enforcement**:
   - For advisory locks, processes must check lock status before operating
   - For mandatory locks, the kernel actively blocks incompatible operations

4. **Lock release**:
   - Locks are released explicitly via system calls
   - Locks are automatically released when a process closes the file or terminates

### 2.4.3. Comparison: flock() vs. fcntl()

Linux provides two main mechanisms for file locking: `flock()` and `fcntl()`:

| **Feature**     | **flock()**                                           | **fcntl()**                                                                 |
|----------------|---------------------------------------------------------|------------------------------------------------------------------------------|
| **Lock scope** | Entire file only | Entire file or specific byte ranges |
| **Lock types** | Shared (read) or Exclusive (write) | Shared (read), Exclusive (write), or Unlock |
| **Programming complexity** | Simple API, easy to use | More complex but highly flexible |
| **Persistence** | Released when all file descriptors close | Can persist across fork() and dup() |
| **Network filesystem support** | Inconsistent across implementations | Better support on network filesystems |
| **Blocking behavior** | Blocking by default, with non-blocking option | Both blocking and non-blocking modes |

The right choice depends on your specific requirements:
- **Use flock()** when you need simple whole-file locking
- **Use fcntl()** when you need fine-grained control or byte-range locking

### 2.4.4. Using flock() for File Locking

The `flock()` system call provides simple whole-file locking:

#### Syntax and Parameters

```c
#include <sys/file.h>

int flock(int fd, int operation);
```

- **fd**: File descriptor of the file to lock
- **operation**: Bit mask specifying the lock operation

#### Operation Flags

| **Flag** | **Description** |
|---------|----------------|
| `LOCK_SH` | Place a shared (read) lock |
| `LOCK_EX` | Place an exclusive (write) lock |
| `LOCK_UN` | Remove an existing lock |
| `LOCK_NB` | Non-blocking mode (returns immediately if lock cannot be acquired) |

#### Example: Basic File Locking with flock()

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    // Open the file
    int fd = open(argv[1], O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    
    printf("Attempting to lock file...\n");
    
    // Try to acquire an exclusive lock
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            printf("File is locked by another process. Waiting...\n");
            // Switch to blocking mode and wait for the lock
            if (flock(fd, LOCK_EX) == -1) {
                perror("flock");
                close(fd);
                return 1;
            }
        } else {
            perror("flock");
            close(fd);
            return 1;
        }
    }
    
    printf("File locked successfully. Writing data...\n");
    
    // Simulating file operation
    const char *message = "Data written while locked\n";
    if (write(fd, message, strlen(message)) == -1) {
        perror("write");
    }
    
    printf("Press Enter to unlock the file...");
    getchar();
    
    // Release the lock
    if (flock(fd, LOCK_UN) == -1) {
        perror("flock unlock");
    } else {
        printf("File unlocked.\n");
    }
    
    close(fd);
    return 0;
}
```

### 2.4.5. Using fcntl() for File Locking

The `fcntl()` system call provides more flexible file locking with byte-range control:

#### Syntax and Parameters

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, struct flock *lock);
```

- **fd**: File descriptor of the file to lock
- **cmd**: Command specifying the lock operation
- **lock**: Pointer to a struct flock containing lock details

#### struct flock Fields

```c
struct flock {
    short l_type;    /* Type of lock: F_RDLCK, F_WRLCK, F_UNLCK */
    short l_whence;  /* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
    off_t l_start;   /* Starting offset for lock */
    off_t l_len;     /* Number of bytes to lock (0 means until EOF) */
    pid_t l_pid;     /* PID of process holding the lock (set by F_GETLK) */
};
```

#### Command Flags

| **Command** | **Description** |
|------------|----------------|
| `F_SETLK` | Set or clear a lock (non-blocking) |
| `F_SETLKW` | Set or clear a lock, wait if blocked (blocking) |
| `F_GETLK` | Get information about an existing lock |

#### Lock Types

| **Type** | **Description** |
|---------|----------------|
| `F_RDLCK` | Shared (read) lock |
| `F_WRLCK` | Exclusive (write) lock |
| `F_UNLCK` | Unlock |

#### Example: Record Locking with fcntl()

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Lock a specific region of a file
int lock_region(int fd, int lock_type, off_t offset, off_t len) {
    struct flock lock;
    
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = offset;
    lock.l_len = len;
    
    return fcntl(fd, F_SETLK, &lock);
}

// Test if a region is locked
int is_locked(int fd, off_t offset, off_t len) {
    struct flock lock;
    
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = offset;
    lock.l_len = len;
    
    if (fcntl(fd, F_GETLK, &lock) == -1) {
        return -1; // Error occurred
    }
    
    if (lock.l_type == F_UNLCK) {
        return 0; // Not locked
    }
    
    return 1; // Locked by process lock.l_pid
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    // Open the file
    int fd = open(argv[1], O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    
    // Set up file with sample data if empty
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == 0) {
        const char *data = "Record 1\nRecord 2\nRecord 3\nRecord 4\nRecord 5\n";
        lseek(fd, 0, SEEK_SET);
        write(fd, data, strlen(data));
        file_size = strlen(data);
    }
    
    // Define record size and position
    off_t record_size = 10; // Approximate size of each record
    off_t record_num = 2;   // Lock the third record (0-based index)
    off_t offset = record_num * record_size;
    
    // Check if record is already locked
    if (is_locked(fd, offset, record_size) > 0) {
        printf("Record %ld is already locked by another process\n", record_num);
        close(fd);
        return 1;
    }
    
    // Lock the record
    if (lock_region(fd, F_WRLCK, offset, record_size) == -1) {
        if (errno == EAGAIN || errno == EACCES) {
            printf("Record %ld is locked by another process\n", record_num);
        } else {
            perror("fcntl lock");
        }
        close(fd);
        return 1;
    }
    
    printf("Record %ld locked successfully\n", record_num);
    
    // Simulate modifying the record
    lseek(fd, offset, SEEK_SET);
    const char *new_data = "UPDATED!\n";
    write(fd, new_data, strlen(new_data));
    
    printf("Press Enter to unlock the record...");
    getchar();
    
    // Unlock the record
    if (lock_region(fd, F_UNLCK, offset, record_size) == -1) {
        perror("fcntl unlock");
    } else {
        printf("Record %ld unlocked\n", record_num);
    }
    
    close(fd);
    return 0;
}
```

### 2.4.6. Best Practices for File Locking

When implementing file locking in your applications:

1. **Always check return values**
   - Lock operations can fail for various reasons
   - Properly handle error conditions

2. **Avoid holding locks for too long**
   - Long-held locks reduce concurrency
   - Minimize the critical section

3. **Consider a lock timeout mechanism**
   - Prevent infinite waits for blocking locks
   - Implement retry logic with exponential backoff

4. **Release locks properly**
   - Always release locks when done
   - Use cleanup handlers to release locks on errors or signals

5. **Be aware of network filesystem limitations**
   - NFS and other network filesystems may handle locks differently
   - Test locking behavior specifically on your target filesystem

6. **Document your locking strategy**
   - Make it clear which locks protect which resources
   - Establish a consistent order for acquiring multiple locks to prevent deadlocks

--- 

## 2.5. Advanced File System Concepts

### 2.5.1. Extended Attributes

Extended attributes (xattr) provide a way to associate metadata with files beyond the standard attributes stored in the i-node:

```sh
# Set an extended attribute
setfattr -n user.description -v "Important document" file.txt

# Get an extended attribute
getfattr -n user.description file.txt

# List all extended attributes
getfattr -d file.txt
```

In C programs:
```c
#include <sys/xattr.h>

// Set extended attribute
setxattr("file.txt", "user.description", 
         "Important document", strlen("Important document"), 0);

// Get extended attribute
char value[1024];
getxattr("file.txt", "user.description", value, sizeof(value));
```

Extended attributes are categorized into namespaces:
- **user**: User-defined attributes (most common)
- **system**: System-defined attributes
- **security**: Security-related attributes (SELinux uses this)
- **trusted**: Restricted to privileged processes

### 2.5.2. Access Control Lists (ACLs)

ACLs extend the traditional Unix permission model to provide finer-grained control:

```sh
# View ACLs for a file
getfacl file.txt

# Add read permission for a specific user
setfacl -m u:username:r file.txt

# Add read/write permission for a specific group
setfacl -m g:groupname:rw file.txt

# Remove ACL entry for a user
setfacl -x u:username file.txt
```

ACLs allow permissions for:
- Specific users beyond owner
- Specific groups beyond the primary group
- Default permissions for new files in directories

### 2.5.3. Filesystem in Userspace (FUSE)

FUSE allows implementing filesystems in user space instead of kernel space:

- Enables non-privileged users to create filesystems
- Simplifies filesystem development
- Allows mounting unusual data sources as filesystems

Examples of FUSE filesystems:
- **sshfs**: Mount remote directories over SSH
- **encfs**: Encrypted filesystem
- **ntfs-3g**: Full-featured NTFS driver
- **gphoto2fs**: Mount digital cameras

### 2.5.4. Copy-on-Write Filesystems

Modern filesystems like Btrfs and ZFS use copy-on-write techniques:

- Changes write to new locations instead of overwriting
- Original data remains intact until transaction completes
- Enables features like snapshots and crash resistance

Key benefits:
- **Atomic updates**: Prevents partial writes during crashes
- **Snapshots**: Efficient point-in-time copies
- **Clones**: Space-efficient file copies
- **Data integrity**: Checksums verify data hasn't been corrupted

### 2.5.5. Journaling

Journaling filesystems (ext3, ext4, XFS) maintain a journal (transaction log) to prevent filesystem corruption:

1. **Before changing data**: Write intentions to journal
2. **Make changes**: Update actual filesystem
3. **Mark complete**: Note in journal that changes are done

If a crash occurs during step 2, the system can:
- Check the journal during recovery
- Either complete interrupted operations
- Or roll back to the pre-operation state

Journaling modes in ext4:
- **data=writeback**: Journal metadata only (fastest, least safe)
- **data=ordered**: Journal metadata, ensure data written first (default)
- **data=journal**: Journal both metadata and data (safest, slowest)

### 2.5.6. Linux Buffer and Cache Management

Linux uses sophisticated memory management for I/O performance:

```sh
# View buffer and cache statistics
free -m

# Current kernel memory parameters
sysctl -a | grep vm.
```

Memory management components:
- **Buffer cache**: Stores filesystem metadata
- **Page cache**: Stores file contents
- **Dirty page writeback**: Controls when modified pages are written to disk

Tuning parameters:
- **vm.dirty_ratio**: Maximum memory percentage for dirty pages
- **vm.dirty_background_ratio**: Threshold for background writeback
- **vm.swappiness**: Preference for swapping vs. dropping cache