# Chapter 5 - Memory

> Topics: 5.1 Memory Allocation | 5.2 Memory Mappings | 5.3 Virtual Memory Operations
> Main sources: TLPI Ch07, Ch49, Ch50 | DevLinux Module 03
> Production context: heap ownership, `mmap()` for files/shared memory, memory faults, daemon leaks, embedded/realtime memory pressure

---

## Coverage Notes

This file covers all Chapter 5 learning-map rows:

- 5.1 Memory Allocation: `malloc/free`, heap, `brk/sbrk`, allocator metadata,
  leaks, fragmentation, ownership, and allocation debugging.
- 5.2 Memory Mappings: `mmap/munmap`, file-backed and anonymous mappings,
  `MAP_PRIVATE`, `MAP_SHARED`, device-backed mappings, `msync()`, COW,
  driver ABI constraints, cache/coherency rules, `fork()` inheritance,
  `exec()` cleanup, `SIGSEGV`, `SIGBUS`, overcommit, and OOM behavior.
- 5.3 Virtual Memory Operations: `mprotect()`, `mlock()/mlockall()`,
  `madvise()`, page residency, and the security/performance tradeoffs around
  these APIs.

DevLinux Module 03 is used here only for process memory layout and practical
process context. Its process exercises are not memory-specific, so they are out
of scope for this knowledge file.

---

## Problem It Solves

C programs often do not know every data size at compile time. A backend daemon
needs request buffers, an embedded process needs predictable memory ownership,
a database/search service may need random access to large files, and related
processes sometimes need fast shared memory.

Linux solves these problems with virtual memory:

```text
process sees its own virtual address space
    |
    v
kernel maps virtual pages to RAM, file page cache, swap, or nothing yet
    |
    v
APIs such as malloc(), mmap(), mprotect(), and mlock() control those regions
```

This chapter helps answer five practical questions:

```text
Is this memory on the stack, heap, or in a mapping?
Who owns this memory and who releases it?
Is this mapping private or shared?
Is this failure a SIGSEGV, SIGBUS, leak, or OOM problem?
Does this advanced VM API actually fit the current workload?
```

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | memory layout, stack vs heap vs mapping, `malloc/free`, `mmap/munmap`, `MAP_PRIVATE` vs `MAP_SHARED`, `SIGSEGV` vs `SIGBUS` | read C systems code, debug crashes/leaks, answer core interview questions |
| Work useful | safe `realloc`, ownership cleanup, `calloc`, `posix_memalign`, `msync`, `/proc/<PID>/maps`, `pmap`, `strace`, Valgrind/ASan | write and debug daemon/backend/embedded C or C++ code |
| Recognize | `brk/sbrk`, `alloca`, overcommit/OOM, `mincore`, `mremap`, `MAP_FIXED`, huge pages | know the names and risks; deep dive only when a real workload needs them |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Virtual address space | Private address range seen by a process | Two processes can have the same virtual address pointing to different physical pages |
| Page | Kernel/MMU memory management unit | Commonly 4 KiB; query with `sysconf(_SC_PAGESIZE)` |
| Page table | Translation table from virtual page to physical frame | Also stores permission bits such as read/write/execute |
| VMA | Virtual Memory Area: contiguous address range with the same attributes | Each `/proc/<PID>/maps` line usually represents one VMA |
| Page fault | Trap raised when a page is absent or access violates permissions | Demand paging relies on valid page faults |
| Demand paging | Kernel allocates/loads RAM only when a process touches a page | A successful `malloc()` pointer does not mean every page already has RAM |
| RSS | Resident Set Size: memory currently resident in RAM | Different from virtual size/VmSize |
| Text segment | Machine-code region of the program | Usually read/execute |
| Data segment | Nonzero initialized global/static data | Example: `static int x = 10;` |
| BSS | Zero-initialized or uninitialized global/static data | Kernel provides zero-filled contents at load time |
| Stack | Call frames and automatic local variables | Lifetime ends when the function returns |
| Heap | Dynamic allocation area managed by the allocator | `malloc()`, `calloc()`, `realloc()`, `free()` |
| Program break | Traditional top boundary of the heap | `brk/sbrk` manipulate this boundary |
| Ownership | Rule for who is responsible for releasing memory | One allocation should have one clear owner |
| Memory leak | Memory is no longer needed but not released | Long-running daemons can grow RSS until OOM |
| Use-after-free | Code uses a pointer after `free()` | Can crash later or become a security bug |
| Double-free | Same allocation is passed to `free()` twice | Can corrupt allocator metadata |
| Memory mapping | Virtual memory region created by `mmap()` | Can be file-backed or anonymous |
| File-backed mapping | Mapping initialized from a file region | Used for mapped files, executables, shared libraries |
| Anonymous mapping | Mapping without a real file, initially zero-filled | Used for private memory or parent/child sharing |
| `MAP_PRIVATE` | Writes are not visible to other processes and do not update the file | Usually implemented with copy-on-write |
| `MAP_SHARED` | Writes are visible to mappings of the same object | File-backed dirty pages can be written back |
| Copy-on-write | Pages are shared until a writer appears, then the kernel copies the page | Used by `fork()` and `MAP_PRIVATE` |
| `msync()` | Synchronizes dirty pages from a shared file mapping | Use when durability or portability matters |
| `SIGSEGV` | Invalid address access or protection violation | Example: write to a `PROT_NONE` page |
| `SIGBUS` | Address is inside a VMA but the backing object is invalid | Example: access a mapped page beyond the file backing |
| `mprotect()` | Changes page permissions for mapped pages | Commonly used for guard pages |
| `mlock()` | Pins pages in RAM so they are not swapped out | Useful for small secrets or realtime paths |
| Overcommit | Linux may promise more virtual memory than RAM plus swap | Allocation can succeed and later page touch can still OOM |

---

## Concept Overview

Basic process memory layout:

```text
High addresses
    +------------------------------+
    | stack                        | local variables, call frames
    +------------------------------+
    | memory mappings              | shared libraries, mapped files, large regions
    +------------------------------+
    | heap                         | malloc/free
    +------------------------------+
    | BSS                          | zero-initialized globals/statics
    +------------------------------+
    | data                         | initialized globals/statics
    +------------------------------+
    | text                         | executable code
    +------------------------------+
Low addresses
```

Remember these three memory groups first:

| Mechanism | Lifetime | Use for | Main bugs |
|-----------|----------|---------|-----------|
| Stack | automatic, tied to function calls | small local variables | stack overflow, pointer to a returned local |
| Heap | explicit, tied to ownership | runtime-sized objects/buffers | leak, use-after-free, double-free |
| Mapping | explicit with `munmap()` or removed on process exit/exec | files, shared memory, page-level control | wrong flags, `SIGBUS`, `SIGSEGV`, missing sync |

Demand paging:

```text
malloc() / mmap()
    |
    v
virtual range becomes valid
    |
    v
first page touch
    |
    v
page fault
    |
    +--> anonymous page: zero-filled RAM page
    +--> file mapping: page-cache page loaded from file
    +--> COW write: private copy is created
```

Fast comparison:

| Question | Heap | Mapping |
|----------|------|---------|
| API layer | C library | system call |
| Common use | normal dynamic objects | file memory, shared memory, page-level control |
| Release | `free()` | `munmap()` |
| Sharing | not IPC by itself | `MAP_SHARED` can share |
| Failure check | `NULL` | `MAP_FAILED` |

---

## System Context

Memory sits at the intersection of process, file system, signals, and the kernel
virtual-memory subsystem:

```text
ELF loader maps executable/shared libraries
    |
    v
kernel VM tracks VMA + page tables
    |
    +--> page fault handler
    +--> anonymous memory
    +--> page cache for mapped files
    +--> swap / reclaim / overcommit / OOM
    |
    v
glibc allocator implements malloc family
    |
    +--> may grow heap via brk/sbrk
    +--> may use mmap for large allocations
```

Production consequences:

| Failure | Consequence |
|---------|-------------|
| leak in daemon | RSS grows, latency increases, process may be OOM-killed |
| heap corruption | crash may happen far away from the original overwrite |
| use-after-free | data corruption or exploitable bug |
| wrong `mmap()` failure check | program uses `(void *) -1` as a valid pointer |
| mapped file too small | `SIGBUS` |
| wrong protection | `SIGSEGV` |
| shared memory without sync | race, torn data, inconsistent state |

---

## Architecture

The kernel tracks a process address space as VMAs:

```text
mm_struct
    |
    +--> VMA text: r-x, backed by executable
    +--> VMA heap: rw-, anonymous/private
    +--> VMA libc.so: r-x/rw-, backed by shared library
    +--> VMA mmap file: permissions + file + offset
    +--> VMA stack: rw-, grows as needed

page tables
    |
    +--> present page: points to RAM/page cache
    +--> non-present page: page fault needed
    +--> permission bits: read/write/execute
```

Allocator architecture, simplified:

```text
malloc(size)
    |
    +--> reuse a free block if possible
    +--> ask kernel for more memory if needed

free(ptr)
    |
    +--> return block to allocator structures
    +--> may not return memory to the kernel immediately
```

Core mapping matrix:

| Mapping type | `MAP_PRIVATE` | `MAP_SHARED` |
|--------------|---------------|--------------|
| File-backed | initialize from file; writes are private | memory-mapped I/O, file-backed IPC |
| Anonymous | private zero-filled memory | share memory between related processes after `fork()` |

---

## Execution Flow

### Flow 1 - Heap Allocation and Cleanup

```text
malloc(size)
    |
    v
allocator returns a block or asks kernel for more virtual memory
    |
    v
program uses the block
    |
    v
owner calls free(ptr)
    |
    v
allocator can reuse the block later
```

Production lesson: `free()` means "return to allocator"; it does not guarantee
that RSS drops immediately.

### Flow 2 - Safe File Mapping Reader

```text
open(file, O_RDONLY)
    |
    v
fstat() to get file size
    |
    v
mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0)
    |
    v
close(fd)
    |
    v
read bytes through pointer
    |
    v
munmap(ptr, size)
```

### Flow 3 - Shared Mapping IPC

```text
process A and process B map the same object with MAP_SHARED
    |
    v
both page tables point to the same physical/page-cache pages
    |
    v
process A writes bytes
    |
    v
process B can observe those bytes
    |
    v
lock/semaphore/atomic protocol is still required
```

### Flow 4 - Mapping Bug Lifecycle

```text
file size = 2200 bytes
mmap length = 8192 bytes
program touches offset 4096
    |
    v
address is inside a VMA but has no file backing
    |
    v
SIGBUS
```

Fix: extend the file first with `ftruncate()` before mapping/writing pages that
need backing storage.

### Flow 5 - Guard Page

```text
mmap(two pages, read/write)
    |
    v
mprotect(second page, PROT_NONE)
    |
    +--> normal access in first page works
    +--> overflow into second page causes SIGSEGV
    |
    v
munmap(region)
```

---

## 5.1 Memory Allocation

### `malloc()` and `free()`

```c
#include <stdlib.h>

void *malloc(size_t size);
void free(void *ptr);
```

Use for normal dynamic objects and buffers whose size is known only at runtime.

Production rules:

| Rule | Reason |
|------|--------|
| check `malloc()` when failure matters | embedded/daemon code can hit limits |
| one allocation has one owner | prevents leaks and double-free |
| call `free()` exactly once | allocator metadata must remain consistent |
| never use a pointer after `free()` | lifetime has ended |
| never write outside the allocated size | can corrupt allocator metadata |
| `free(NULL)` is valid | useful in cleanup paths |

Avoid:

- freeing stack/global/interior pointers;
- assuming `free()` immediately lowers RSS;
- designing unclear ownership across modules.

Production pitfall: heap corruption often crashes later in unrelated code, not
at the line that overwrote memory.

### `calloc()`, `realloc()`, `posix_memalign()`

```c
#include <stdlib.h>

void *calloc(size_t nitems, size_t size);
void *realloc(void *ptr, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size);
```

| API | Use when | Pitfall |
|-----|----------|---------|
| `calloc()` | need a zero-initialized array | still must be freed with `free()` |
| `realloc()` | need to grow/shrink a heap block | may move the block |
| `posix_memalign()` | need aligned memory for direct I/O, SIMD, or page work | returns an error number, not `-1` |

Safe `realloc()` pattern:

```c
void *tmp = realloc(ptr, new_size);
if (tmp != NULL) {
    ptr = tmp;
}
```

Do not write:

```c
ptr = realloc(ptr, new_size);  /* leak if realloc fails */
```

### `brk()`, `sbrk()`, and `alloca()`

```c
#include <unistd.h>

int brk(void *end_data_segment);
void *sbrk(intptr_t increment);
```

`brk/sbrk` explain how the traditional heap grows, but normal application code
should use the malloc family. Manually calling `sbrk()` beside glibc malloc is
not a normal production pattern.

```c
#include <alloca.h>

void *alloca(size_t size);
```

`alloca()` allocates on the stack and is automatically released when the function
returns. Recognize it for code reading and interviews, but avoid large or
untrusted sizes because stack overflow does not provide a clean `NULL` failure.

---

## 5.2 Memory Mappings

### `mmap()` and `munmap()`

```c
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
int munmap(void *addr, size_t length);
```

Use `mmap()` when you truly need one of these:

- file bytes accessible as memory;
- shared memory;
- a large/page-aligned region managed manually;
- guard pages or page-permission changes.

Argument checklist:

| Argument | Practical rule |
|----------|----------------|
| `addr` | pass `NULL` unless writing a runtime, allocator, or other low-level code |
| `length` | save it so `munmap()` uses the same logical size |
| `prot` | request only the permissions needed |
| `flags` | choose exactly one main type: `MAP_PRIVATE` or `MAP_SHARED` |
| `fd` | file mappings use an fd; anonymous mappings usually use `-1` |
| `offset` | must be page-aligned on Linux |

Pitfalls:

- failure is `MAP_FAILED`, not `NULL`;
- `MAP_FIXED` can replace existing mappings; avoid in normal code;
- touching memory after `munmap()` is invalid.

### Protection and Mapping Type

Protection flags:

| Flag | Meaning |
|------|---------|
| `PROT_NONE` | no access |
| `PROT_READ` | readable |
| `PROT_WRITE` | writable |
| `PROT_EXEC` | executable |

Mapping type:

| Feature | `MAP_PRIVATE` | `MAP_SHARED` |
|---------|---------------|--------------|
| writes visible to other mappings | no | yes |
| writes propagated to file | no | yes, for file mapping |
| mechanism | copy-on-write | shared physical/page-cache pages |
| common use | private file reader, executable/library data | mmap I/O, shared memory |

### File-backed Mapping

When mapping a file:

- open read-only for a read-only mapping;
- open read-write for writable `MAP_SHARED`;
- use `fstat()` to get size;
- handle empty files specially;
- use `ftruncate()` before mapping an output/shared file larger than current size;
- after successful `mmap()`, `close(fd)` is safe if the fd is no longer needed;
- use `msync()` if dirty shared file pages must be forced to storage.

```c
int msync(void *addr, size_t length, int flags);
```

First-pass knowledge:

| Flag | Meaning |
|------|---------|
| `MS_SYNC` | wait until dirty pages are written |
| `MS_ASYNC` | schedule writeback |

Linux note: Linux uses a unified page cache, so the mapping view and `read()`
view of the same file are normally consistent in the page cache. `msync(MS_SYNC)`
is still needed when you need to force data to storage; portable designs that
target non-unified VM systems need more explicit synchronization.

### Device-backed Mapping

Some file-backed mappings are not normal disk files. Embedded and driver-facing
code may map device memory through a device file such as a framebuffer, UIO
device, or a driver-specific character device.

Treat device mappings as a driver contract, not as ordinary RAM:

| Concern | Practical rule |
|---------|----------------|
| ownership | the kernel driver decides what physical/device region may be exposed; user space must not guess physical addresses |
| permissions | opening the device and mapping it often needs privileges, `udev` policy, or driver-specific `ioctl()` setup |
| alignment | `offset` is page-based for `mmap()`; many drivers use the offset to select a buffer/register window, so follow the ABI exactly |
| memory type | MMIO registers, coherent DMA buffers, and cached normal RAM have different access rules |
| caching | cacheability is selected by the driver mapping; wrong cache policy can produce stale DMA data or unsafe register access |
| ordering | MMIO and DMA often need driver-defined barriers, ownership bits, or doorbells; a C store is not automatically a hardware protocol |
| lifetime | stop DMA/interrupt use before unmapping when the ABI requires it; do not keep stale pointers after `munmap()` or device removal |
| portability | `/dev/mem` and raw physical mappings are board/kernel-policy dependent; prefer a small driver ABI over poking registers from user space |
| failures | bad offsets, hot-unplug, revoked access, or driver errors can surface as `mmap()` failure, `SIGBUS`, `EIO`, or broken device behavior |

Device mapping lifecycle:

```text
open device node
    |
    v
driver ioctl/setup chooses buffer, mode, ownership, or register window
    |
    v
mmap length/offset from the documented ABI
    |
    v
access only with the required volatile/atomic/barrier/synchronization protocol
    |
    v
stop hardware use if required -> munmap -> close
```

Debug device mappings from both sides: inspect `/proc/<PID>/maps`, trace
`open`, `ioctl`, `mmap`, and `munmap` with `strace`, and check `dmesg` for
driver messages. For DMA buffers, compare the driver documentation with
`/sys/class`, `/sys/bus`, device-tree data, or platform logs when available. If
memory contents are shared with hardware or DMA, normal C pointers are not
enough; the driver, cache policy, barriers, and ownership protocol decide
whether data is coherent.

### Anonymous Mapping

Anonymous mappings have no real file and start zero-filled:

```c
void *p = mmap(NULL, length, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
```

Use cases:

- private memory outside the normal allocator;
- shared memory between related processes:

```c
void *p = mmap(NULL, length, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
```

After `fork()`, parent and child can share that mapping. `MAP_ANONYMOUS` is
common on Linux/BSD. For unrelated processes, POSIX shared memory usually uses
`shm_open()` plus `mmap()`.

### `read/write` vs `mmap`

| Workload | Prefer |
|----------|--------|
| simple sequential I/O | `read()` / `write()` |
| repeated random access to a large file | `mmap()` may help |
| structured persistent data | `mmap()` can simplify addressing |
| small one-shot I/O | `read()` / `write()` |
| persistent region that must be shared | `MAP_SHARED` file mapping |

`mmap()` is not automatically faster. It replaces explicit I/O calls with page
faults, VMA management, TLB effects, and writeback behavior.

### Lifetime and Signals

| Case | Behavior |
|------|----------|
| `fork()` | child inherits mappings |
| `exec()` | mappings are removed |
| process exit | mappings are removed |
| `close(fd)` after `mmap()` | mapping remains valid |
| access unmapped address | `SIGSEGV` |
| access violates `PROT_*` | `SIGSEGV` |
| access mapped page beyond file backing | `SIGBUS` |

---

## 5.3 Virtual Memory Operations

These APIs are useful, but they are not daily tools for every C program.

### `mprotect()`

```c
#include <sys/mman.h>

int mprotect(void *addr, size_t length, int prot);
```

Use when you need page permission control, usually for guard pages or runtime
memory protection.

Rules:

- `addr` must be page-aligned on Linux;
- permissions apply to whole pages;
- invalid access after the protection change causes `SIGSEGV`.

Avoid casual use because one page can contain multiple C objects.

### `mlock()` and `mlockall()`

```c
#include <sys/mman.h>

int mlock(void *addr, size_t length);
int munlock(void *addr, size_t length);
int mlockall(int flags);
int munlockall(void);
```

Use when:

- a small secret should not be swapped;
- a realtime path cannot tolerate page faults.

Remember:

- locks apply to whole pages;
- `mlock()` faults pages into RAM before returning;
- unprivileged processes are limited by `RLIMIT_MEMLOCK`;
- locks are not inherited across `fork()` and are not preserved across `exec()`;
- `mlockall(MCL_FUTURE)` can make later allocation/stack growth fail.

Security note: `mlock()` avoids swap, but it does not protect against
hibernate/suspend writing all RAM to disk.

### `madvise()`

```c
int madvise(void *addr, size_t length, int advice);
```

`madvise()` tells the kernel how the program expects to use a memory range. Use
it for tuning after measurement, not as basic allocation logic.

Common `madvise()` values to recognize:

| Advice | Meaning |
|--------|---------|
| `MADV_RANDOM` | random access |
| `MADV_SEQUENTIAL` | sequential access |
| `MADV_WILLNEED` | likely needed soon |
| `MADV_DONTNEED` | current contents/pages are no longer needed |

Linux trap: `MADV_DONTNEED` on private mappings can discard modified pages.
Treat it as a content-affecting operation, not just a harmless hint.

---

## Work-Useful Patterns

| Pattern | Practical design |
|---------|------------------|
| Clear ownership | each heap allocation has one owner and one cleanup path |
| Cleanup-on-error | use one cleanup block so every successful allocation/mapping is released |
| Safe growable buffer | check overflow and use a temporary pointer for `realloc()` |
| Bounded embedded memory | prefer pools, arenas, or static buffers for predictable RAM use |
| File mapping reader | `open` -> `fstat` -> skip empty -> `mmap MAP_PRIVATE` -> `close` -> `munmap` |
| Writable mapped file | `open O_RDWR` -> `ftruncate` -> `mmap MAP_SHARED` -> write -> `msync` if durable -> `munmap` |
| Device mapping | confirm driver ABI, permissions, offset meaning, memory type, cache/coherency rules, hardware ownership protocol, and `dmesg` evidence |
| Shared memory IPC | shared mapping plus mutex/semaphore/atomic protocol |
| Guard page | allocate an extra page and set the boundary page to `PROT_NONE` |
| Secret/realtime memory | lock only the small pages that are truly needed; test `RLIMIT_MEMLOCK` |
| Memory spike debugging | compare `VmSize`, `VmRSS`, `/proc/<PID>/maps`, `smaps`, and OOM logs |

---

## Advanced / Recognize First

Do not study these deeply on a first pass unless your codebase/workload uses
them.

| Topic | Know this much |
|-------|----------------|
| `mremap()` | Linux-specific mapping resize/move; old pointers may become invalid |
| `MAP_FIXED` | forces an exact address and can replace an existing mapping |
| `MAP_NORESERVE` | changes swap reservation and can increase later OOM risk |
| `MAP_POPULATE` | faults/populates pages earlier; useful only after measuring latency |
| `mincore()` | reports page residency; useful for diagnostics, not normal correctness logic |
| Huge pages | TLB optimization for special workloads; operationally heavy |
| allocator tuning | `mallopt`-style tuning needs measurement, not guessing |

---

## Example

### Example 1 - Safe Heap Buffer Growth

```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    size_t capacity = 4;
    size_t count = 0;
    int *items = malloc(capacity * sizeof(*items));

    if (items == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (int value = 0; value < 10; value++) {
        if (count == capacity) {
            size_t new_capacity;
            int *tmp;

            if (capacity > SIZE_MAX / 2) {
                free(items);
                fprintf(stderr, "capacity overflow\n");
                return EXIT_FAILURE;
            }

            new_capacity = capacity * 2;
            if (new_capacity > SIZE_MAX / sizeof(*items)) {
                free(items);
                fprintf(stderr, "allocation size overflow\n");
                return EXIT_FAILURE;
            }

            tmp = realloc(items, new_capacity * sizeof(*items));
            if (tmp == NULL) {
                perror("realloc");
                free(items);
                return EXIT_FAILURE;
            }

            items = tmp;
            capacity = new_capacity;
        }

        items[count++] = value;
    }

    for (size_t i = 0; i < count; i++) {
        printf("%d%s", items[i], (i + 1 == count) ? "\n" : " ");
    }

    free(items);
    return EXIT_SUCCESS;
}
```

What it teaches:

- heap ownership should have a clear cleanup path;
- the `realloc()` result goes through a temporary pointer;
- production code checks allocation-size overflow before multiplication.

### Example 2 - Read a File Through `MAP_PRIVATE`

```c
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);

        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }

        buf += n;
        len -= (size_t)n;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat st;
    char *addr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    if (st.st_size == 0) {
        close(fd);
        return EXIT_SUCCESS;
    }

    addr = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    if (write_all(STDOUT_FILENO, addr, (size_t)st.st_size) == -1) {
        perror("write");
        munmap(addr, (size_t)st.st_size);
        return EXIT_FAILURE;
    }

    munmap(addr, (size_t)st.st_size);
    return EXIT_SUCCESS;
}
```

What it teaches:

- `mmap()` failure is `MAP_FAILED`;
- the file descriptor may be closed after successful mapping;
- `MAP_PRIVATE` fits read-only file input;
- production output handles `EINTR` and partial writes.

### Example 3 - Parent and Child Share Anonymous Memory

```c
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int *shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shared == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    *shared = 1;

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        munmap(shared, sizeof(*shared));
        return EXIT_FAILURE;
    }

    if (child == 0) {
        *shared += 41;
        _exit(EXIT_SUCCESS);
    }

    if (waitpid(child, NULL, 0) == -1) {
        perror("waitpid");
        munmap(shared, sizeof(*shared));
        return EXIT_FAILURE;
    }

    printf("shared value = %d\n", *shared);
    munmap(shared, sizeof(*shared));
    return EXIT_SUCCESS;
}
```

What it teaches:

- `MAP_SHARED | MAP_ANONYMOUS` shares pages across `fork()`;
- this example is safe because the child writes and exits before the parent reads;
- real concurrent shared memory still needs synchronization.

### Example 4 - Guard Page With `mprotect()`

```c
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    char *region;

    if (page_size == -1) {
        perror("sysconf");
        return EXIT_FAILURE;
    }

    region = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    if (mprotect(region + page_size, (size_t)page_size, PROT_NONE) == -1) {
        perror("mprotect");
        munmap(region, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    region[0] = 'A';
    printf("first page is writable; second page is a guard page\n");

    munmap(region, (size_t)page_size * 2);
    return EXIT_SUCCESS;
}
```

What it teaches:

- page-aligned mappings can be protected with `mprotect()`;
- a `PROT_NONE` guard page catches overflow as `SIGSEGV`;
- protection works by page, not by C object.

---

## Debugging

Inspect process memory:

```bash
cat /proc/<PID>/maps
pmap -x <PID>
cat /proc/<PID>/status
cat /proc/<PID>/smaps
```

Trace memory syscalls:

```bash
strace -e trace=brk,mmap,munmap,mprotect,mlock,mlockall,madvise ./program
```

Find heap bugs:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./program

gcc -Wall -Wextra -g -fsanitize=address,undefined program.c -o program
./program
```

Debug crashes:

```bash
gdb ./program
(gdb) run
(gdb) bt
(gdb) info proc mappings
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| leak | RSS grows over time | Valgrind/ASan, ownership cleanup |
| use-after-free | random crash, stale data | ASan, fix lifetime |
| double-free | allocator abort/corruption | one owner, one release |
| overflow | crash later in allocator | bounds check, `sizeof(*ptr)`, ASan |
| unsafe `realloc()` | leak on failure | use temporary pointer |
| wrong `mmap()` check | failure path missed | compare with `MAP_FAILED` |
| mapped file too small | `SIGBUS` | `ftruncate()` before mapping/writing |
| wrong protection | `SIGSEGV` | inspect `/proc/<PID>/maps`, check `mprotect()` |
| shared memory without sync | inconsistent data | mutex/semaphore/atomic protocol |
| overusing `mlock()` | `ENOMEM` or limit failure | check `VmLck`, `ulimit -l`, `RLIMIT_MEMLOCK` |

Memory pressure:

```bash
cat /proc/sys/vm/overcommit_memory
cat /proc/<PID>/oom_score
dmesg | grep -i 'out of memory\|killed process'
```

---

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| dynamic backend buffer | heap allocation with clear owner |
| long-running daemon | bounded caches, leak tests, cleanup-on-error |
| embedded tight RAM | static buffers, pools, arenas |
| large random-access file | `mmap MAP_PRIVATE` for read-only index/data |
| writable persistent region | `MAP_SHARED` file mapping plus `msync` when needed |
| embedded device registers/buffers | device-file `mmap()` only through the driver ABI; verify cache/coherency and permissions |
| parent/child fast exchange | `MAP_SHARED | MAP_ANONYMOUS` plus synchronization if concurrent |
| crash boundary detection | guard page with `mprotect(PROT_NONE)` |
| secret key/passphrase | small locked buffer, zero before release |
| realtime path | allocate/touch pages before critical loop; use `mlockall()` carefully |
| production memory spike | `/proc/<PID>/smaps`, `pmap -x`, OOM logs, allocator tooling |

---

## Interview-Relevant Questions

1. Explain text, data, BSS, heap, mappings, and stack.
2. What is virtual memory, and why is RSS different from virtual size?
3. How does demand paging work?
4. What is the program break, and how does it relate to heap growth?
5. Why does `free()` often not reduce RSS immediately?
6. Explain memory leak, use-after-free, and double-free.
7. Why should `realloc()` be assigned to a temporary pointer first?
8. When would you use `calloc()` or `posix_memalign()`?
9. What does `mmap()` create in the address space?
10. Compare file-backed mappings and anonymous mappings.
11. Compare `MAP_PRIVATE` and `MAP_SHARED`.
12. How does copy-on-write work?
13. Why can `close(fd)` after successful `mmap()` be safe?
14. When do you need `msync()`?
15. What causes `SIGSEGV`, and what causes `SIGBUS`?
16. What happens to mappings across `fork()` and `exec()`?
17. What is Linux overcommit, and why can OOM happen after allocation succeeds?
18. How can `mprotect()` implement a guard page?
19. What does `mlock()` guarantee, and what limits it?
20. What is `madvise()` used for, and why is `MADV_DONTNEED` tricky on Linux?
21. How would you debug a daemon whose RSS keeps growing?
22. How would you design shared memory IPC so readers do not see inconsistent data?
23. What makes device-memory `mmap()` different from mapping a normal file?

---

## Key Takeaways

1. Linux process memory is virtual; RAM is attached lazily through page faults.
2. Stack lifetime is automatic; heap and mappings need explicit lifetime management.
3. Heap correctness is mostly ownership, bounds checking, and cleanup.
4. `malloc()` failure is `NULL`; `mmap()` failure is `MAP_FAILED`.
5. `free()` returns memory to the allocator, not necessarily to the kernel immediately.
6. Use a temporary pointer for `realloc()`.
7. `mmap()` is for file memory, shared memory, and page-level control.
8. The core mapping matrix is file/anonymous plus private/shared.
9. `MAP_PRIVATE` uses copy-on-write; `MAP_SHARED` shares writes.
10. Shared memory still needs synchronization.
11. `SIGSEGV` usually means invalid/prohibited access; `SIGBUS` can mean missing file backing.
12. `fork()` inherits mappings; `exec()` removes mappings.
13. `mprotect()` is mainly for page permissions and guard pages.
14. `mlock()` is useful but limited; do not lock memory casually.
15. Device mappings are driver contracts; permissions, cacheability, coherency,
    and hardware lifetime matter as much as the `mmap()` call.
16. Advanced VM APIs such as `mincore()` and `mremap()` should be recognized first, then deep-dived only when a real workload needs them.
