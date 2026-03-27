# Chapter 5 — Memory
## Topics: 5.1 Memory Allocation · 5.2 Memory Mappings · 5.3 Virtual Memory Operations
> Sources: TLPI Ch07, Ch49, Ch50 | DevLinux Module 03

---

## 5.1 Memory Allocation

### Problem It Solves

Not all memory requirements are known at compile time. Linked lists, dynamic buffers, network data, binary trees — all require memory whose size is only known at runtime. The heap provides a region of virtual memory that can grow on demand, managed by the programmer via `malloc`/`free`.

---

### Concept Overview

The **heap** is a variable-size region of a process's virtual address space that begins immediately after the BSS segment and grows upward. Its upper boundary is called the **program break**. The C library functions `malloc`/`free` manage this region, built on top of the `brk()`/`sbrk()` system calls.

```
High address
  +-------------------+
  |   Kernel space    |
  +-------------------+
  |       Stack       |  ← grows DOWN ↓ (local vars, return addr)
  |        ...        |
  +-------------------+
  |   Memory Mapping  |  ← mmap region (shared libs, large malloc)
  |        ...        |
  |        ↑          |
  |       Heap        |  ← grows UP ↑  (malloc/free territory)
  | program break → --|
  +-------------------+
  |       BSS         |  uninitialized globals (zero-filled)
  +-------------------+
  |      Data         |  initialized globals
  +-------------------+
  |      Text         |  executable code
  +-------------------+
Low address  0x0
```

---

### System Context

- **What it solves:** Enables runtime dynamic memory allocation for data structures whose size is unknown at compile time.
- **Where in the system:** Sits between the BSS segment and the memory mapping region; managed by the C library (glibc ptmalloc) on top of the `brk()` syscall.
- **Subsystem interactions:** Interacts with the kernel VM subsystem via `brk()`/`sbrk()` for heap growth, and with `mmap()`/`munmap()` for allocations exceeding 128 KB. Demand paging means the kernel's page fault handler allocates physical frames on first access.
- **Failure scenarios:** A memory leak causes the heap to grow unboundedly, eventually exhausting virtual address space or triggering OOM kill. Double-free or use-after-free corrupts the allocator's free list metadata, typically manifesting as a crash or silent data corruption far from the original bug.

---

### Internal Mechanism

#### brk() / sbrk() — The Raw Interface

```c
#include <unistd.h>
int brk(void *end_data_segment); // set program break to absolute address
void *sbrk(intptr_t increment);  // shift program break by increment bytes
                                 // returns PREVIOUS program break on success
```

`sbrk(+N)` → `brk()` syscall → kernel updates `mm->brk` in the process's `mm_struct` → virtual address range is registered as valid in the page table — **no physical page is allocated yet**. Physical pages are allocated on first access via the kernel's **demand paging** mechanism.

```c
void *current_brk = sbrk(0);  // query current program break without moving it
```

#### glibc ptmalloc — The Allocator Layer

`malloc`/`free` are C library functions, not syscalls. glibc's allocator (ptmalloc, derived from Doug Lea's dlmalloc) implements a three-tier bin system:

```
malloc(N) dispatch:
    N ≤ ~72B     → Fast bins    (singly-linked, no coalesce, sub-millisecond)
    N ≤ 512B     → Small bins   (sorted by size, FIFO, doubly-linked)
    N ≤ 128KB    → Large bins   (best-fit search in doubly-linked sorted list)
    N > 128KB    → mmap(MAP_ANONYMOUS) directly — bypasses heap entirely
```

**Hidden block header:** Every `malloc`-returned pointer points past a length field `L` stored just before the usable memory:

```
+----------------+---------------------------+
| Length L       |   Memory returned to you  |
|  (hidden)      |   ← ptr points here       |
+----------------+---------------------------+
```

`free()` reads `L` to know the block size — this is why `free(ptr)` needs no size argument.

**Free list structure:**

```
+--------+--------+--------+-------+
|   L    |   P    |   N    |  ...  |   ← block on free list
+--------+--------+--------+-------+
           │         │
        prev free  next free        (doubly-linked → enables coalescing)
         block     block
```

When `free()` is called: block is added to the free list. On the next `malloc(size)`:
1. Scan the appropriate bin for a block large enough
2. Found → remove from free list, return to caller (split if too large)
3. Not found → `sbrk()` to extend heap, put excess on free list

**Why `free()` does not shrink the heap immediately:**

Freed blocks in the middle of the heap cannot lower the program break. glibc only calls `sbrk(-N)` when a contiguous free region **at the top of the heap** exceeds ~128 KB.

#### Demand Paging — Physical RAM is Lazy

```
malloc(100MB)          → program break moved: ~instantaneous
memset(buf, 0, 100MB)  → 25,600 page faults → physical pages allocated one by one
```

This is why `VIRT` and `RES` in `htop` show very different numbers. `VIRT` is virtual address space committed; `RES` is physical RAM actually in use.

#### Large Allocations via mmap

For blocks > `MMAP_THRESHOLD` (default 128 KB):

```
malloc(200KB) → mmap(MAP_PRIVATE | MAP_ANONYMOUS)  ← independent mapping
free(ptr)     → munmap()                            ← physical pages returned to kernel immediately
```

This avoids heap fragmentation for large blocks and allows instant physical memory reclaim on `free()`.

---

### Architecture / Execution Flow

```
User: malloc(N)
      │
      ├─ N ≤ 128KB ──→ ptmalloc bins path
      │                   1. check fast/small/large bins → reuse?
      │                   2. no fit → sbrk() → virtual pages mapped
      │                                      → physical pages: lazy (demand paging)
      │
      └─ N > 128KB ──→ mmap(MAP_ANONYMOUS) path
                          → independent mapping, physical pages: lazy
                          → free() → munmap() → RAM returned immediately

User: free(ptr)
      │
      ├─ sbrk path block:
      │     1. coalesce with adjacent free blocks
      │     2. place in appropriate bin
      │     3. if top of heap ≥ 128KB free → sbrk(-N) shrink
      │
      └─ mmap path block:
            → munmap() immediately
```

---

### Example

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    /* Monitor heap movement */
    printf("Initial brk: %p\n", sbrk(0));

    char *buf = malloc(1024);
    if (buf == NULL) { perror("malloc"); return 1; }
    printf("After malloc(1024) brk: %p\n", sbrk(0));

    memset(buf, 0xAB, 1024);  /* physical pages allocated HERE via page faults */

    /* Safe realloc pattern — never assign back to original pointer directly */
    char *new_buf = realloc(buf, 4096);
    if (new_buf == NULL) {
        free(buf);   /* original block still valid on failure */
        return 1;
    }
    buf = new_buf;  /* update only after success */

    free(buf);
    printf("After free() brk:  %p\n", sbrk(0)); /* likely unchanged */

    /* calloc: allocate + zero-initialize (avoids manual memset) */
    int *arr = calloc(100, sizeof(int));
    if (arr == NULL) { return 1; }
    free(arr);

    return 0;
}
```

---

### Debugging

| Tool | Use |
|---|---|
| `valgrind --leak-check=full` | Detect leaks, use-after-free, buffer overruns |
| `MALLOC_CHECK_=2 ./prog` | glibc corruption detection → `abort()` on error |
| `mtrace()` + `mtrace` script | Trace all malloc/free pairs, find unmatched allocations |
| `sbrk(0)` at checkpoints | Monitor heap growth manually |
| `/proc/PID/status` `VmHeap` | Inspect heap size from outside the process |

```bash
# Leak check
valgrind --leak-check=full --show-leak-kinds=all ./program

# Enable glibc internal malloc checks
MALLOC_CHECK_=2 ./program
```

---

### Real-world Usage

- **Daemon/server processes:** memory leaks are fatal — heap grows until OOM killer terminates the process. Use `valgrind` in testing and add leak test to CI.
- **Embedded systems:** heap fragmentation over time causes `malloc()` to fail despite having technically enough total free memory. Many embedded projects use fixed-size pool allocators to avoid this.
- **Large buffers (> 128KB):** `malloc` routes these through `mmap`, so `free()` returns physical memory to the kernel immediately — preferred for I/O buffers.
- `alloca(size)` allocates on the **stack** via a single stack pointer adjustment — zero allocation overhead, automatically freed on function return. Use for small, short-lived buffers. Never use for large sizes (no NULL return on stack overflow, just SIGSEGV).

---

### Key Takeaways

1. **Heap** = region above BSS, grows upward by moving the **program break** via `brk()`/`sbrk()`.
2. **`malloc`/`free` are library functions**, not syscalls — they manage a free list on top of `sbrk()` to reuse freed blocks without constantly calling the kernel.
3. **`free()` does not immediately shrink the heap** — blocks go to a free list; heap shrinks only when a large contiguous free region exists at the top end.
4. **Every `malloc` block has a hidden length header** — that is why `free(ptr)` needs no size argument.
5. **Large allocations (> 128KB)** bypass the heap and use `mmap(MAP_ANONYMOUS)` — `free()` calls `munmap()` and returns RAM to the kernel immediately.
6. **Physical RAM is allocated lazily** (demand paging) — `malloc` only claims virtual address space; physical pages fault in on first access.
7. **Double-free / use-after-free** corrupt free list metadata → undefined behavior, often SIGSEGV far from the original bug.

---

## 5.2 Memory Mappings (mmap)

### Problem It Solves

`read()`/`write()` always copies data twice: disk → kernel buffer cache → user-space buffer. For large files accessed repeatedly or randomly, this double-copy overhead is significant. `mmap()` maps a file (or anonymous memory) **directly into the process's virtual address space**, eliminating the kernel→user copy and enabling file access via pointer arithmetic rather than syscalls.

Beyond I/O, `mmap()` is the foundation for: loading executables and shared libraries, IPC via shared memory, and large anonymous allocations (as seen in 5.1).

---

### Concept Overview

`mmap()` creates a **VMA (Virtual Memory Area)** in the process's address space. The mapping can be:
- **File-backed:** pages are loaded from a file on demand
- **Anonymous:** pages are zero-filled and not backed by any file

Each mapping is either **private** (copy-on-write) or **shared** (modifications visible to all):

```
                  FILE mapping              ANONYMOUS mapping
               ┌──────────────────────┬────────────────────────┐
  MAP_PRIVATE  │ Load binary/libs     │ Allocate zeroed memory │
               │ (COW, no writeback)  │ (malloc large blocks,  │
               │                      │  stack, BSS)           │
               ├──────────────────────┼────────────────────────┤
  MAP_SHARED   │ Memory-mapped I/O    │ IPC between related    │
               │ IPC via shared file  │ processes (fork)       │
               │ (writes → disk)      │                        │
               └──────────────────────┴────────────────────────┘
```

---

### System Context

- **What it solves:** Eliminates the kernel→user copy cost of `read()`/`write()` for file I/O; provides the mechanism for shared memory IPC, executable loading, and large anonymous allocations.
- **Where in the system:** Operates at the intersection of the VM subsystem (VMAs in `mm_struct`) and the page cache (for file mappings). Lives in the memory-mapping segment between heap and stack.
- **Subsystem interactions:** File mappings share pages with the **page cache** (same physical frames that `read()`/`write()` use). The `munmap()` path interacts with the kernel's physical page reclaim. `fork()` inherits all mappings; `exec()` destroys them.
- **Failure scenarios:** Accessing beyond a file-backed mapping's valid range yields `SIGBUS` (no file content) or `SIGSEGV` (outside virtual mapping). Not calling `msync()` before `munmap()` on a `MAP_SHARED` mapping may lose writes on non-Linux systems (Linux usually unifies page cache, but POSIX portability requires explicit sync).

---

### Internal Mechanism

#### mmap() / munmap() Calls

```c
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
// Returns: mapping start address on success, MAP_FAILED on error

int munmap(void *addr, size_t length);
// Returns: 0 on success, -1 on error
```

| Argument | Notes |
|---|---|
| `addr` | Pass `NULL` — kernel selects a suitable address |
| `length` | Rounded up to next page boundary (4096B) by kernel |
| `prot` | `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`, `PROT_NONE` — or combination |
| `flags` | **Must** include `MAP_PRIVATE` or `MAP_SHARED`, optionally `MAP_ANONYMOUS` |
| `fd` | File descriptor for file mapping; `-1` for anonymous |
| `offset` | Byte offset in file — **must be a multiple of page size** |

#### What Happens at mmap() Time

```
mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)
    │
    ▼
kernel creates a VMA entry in the process mm_struct:
    vm_start = addr       vm_end = addr + length
    vm_prot = prot        vm_flags = MAP_SHARED
    vm_file = fd's inode  (file mapping links to page cache)
    │
    ▼
NO physical pages allocated yet.
    │
Process accesses addr[0] for the first time:
    → PAGE FAULT
    → FILE mapping:  kernel fetches page from disk into page cache
    → ANON mapping:  kernel provides a zero-filled physical page
    → page table updated → access completes, instruction retried
```

#### Page Cache Sharing — Key Advantage

For file mappings, the kernel uses the **same physical pages** as the page cache. When multiple processes `mmap()` the same file, they all see the same physical pages:

```
Process A: mmap("config.bin") ──┐
                                 ├──→ [PAGE CACHE physical pages] ←──→ disk
Process B: mmap("config.bin") ──┘
    read("config.bin") ─────────→ [same PAGE CACHE pages] → copy to user buf
```

`mmap()` saves the copy from page cache to user buffer — that is the performance advantage.

#### After close(fd) — Mapping Survives

```c
fd = open("file", O_RDONLY);
addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
close(fd);   // fd can be closed immediately — mapping holds its own reference
// addr remains valid
```

#### msync() — Explicit Flush to Disk

```c
int msync(void *addr, size_t length, int flags);
```

| Flag | Behavior |
|---|---|
| `MS_SYNC` | Block until all dirty pages are written to disk |
| `MS_ASYNC` | Mark dirty pages for writeback, return immediately |
| `MS_INVALIDATE` | Discard cached copies, re-read from disk on next access |

Linux uses a unified VM (page cache = mmap pages), so `msync()` is primarily for **persistence** (forcing flush to disk), not cross-process visibility.

---

### Architecture / Execution Flow

```
mmap(NULL, N, prot, MAP_SHARED, fd, 0)
    │
    ▼  [Kernel: create VMA, no physical pages]
    │
    ▼  [Process accesses mapping] → PAGE FAULT
    │
    ├─ FILE mapping   → load page from disk into PAGE CACHE
    │                 → multiple processes mapping same file share these pages
    │
    └─ ANON mapping   → provide zero-filled page
                      → MAP_PRIVATE: each process gets its own copy on write
                      → MAP_SHARED:  parent+child share pages (fork IPC)

munmap(addr, N)
    │
    ├─ FILE mapping (MAP_SHARED):
    │       1. Dirty pages written back to disk (eventually, or via msync)
    │       2. Pages may remain in page cache for other users
    │
    └─ ANON mapping (MAP_PRIVATE):
            → Physical pages returned to kernel immediately
            → (This is what free() calls for large malloc blocks)
```

---

### Example

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* --- Private file mapping: read file like memory --- */
void read_file_via_mmap(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open"); return; }

    struct stat sb;
    fstat(fd, &sb);

    char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);   /* fd no longer needed */
    if (addr == MAP_FAILED) { perror("mmap"); return; }

    /* Access file content directly as memory */
    fwrite(addr, 1, sb.st_size, stdout);

    munmap(addr, sb.st_size);
}

/* --- Shared file mapping: read-modify-write without read()/write() --- */
void update_record_via_mmap(const char *path, size_t offset, int new_value) {
    int fd = open(path, O_RDWR);
    if (fd == -1) { perror("open"); return; }

    struct stat sb;
    fstat(fd, &sb);

    int *records = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    close(fd);
    if (records == MAP_FAILED) { perror("mmap"); return; }

    records[offset] = new_value;  /* write directly → will flush to file */

    msync(records, sb.st_size, MS_SYNC);  /* ensure persistence */
    munmap(records, sb.st_size);
}

/* --- Anonymous shared mapping: IPC between parent and child --- */
void ipc_via_anon_mmap(void) {
    int *shared = mmap(NULL, sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) { perror("mmap"); return; }

    *shared = 0;

    pid_t pid = fork();
    if (pid == 0) {
        *shared = 42;   /* child writes */
        _exit(0);
    }
    wait(NULL);
    printf("Parent reads: %d\n", *shared);  /* sees 42 */

    munmap(shared, sizeof(int));
}
```

---

### Debugging

```bash
# Inspect all mappings of a running process
cat /proc/<PID>/maps
# Fields: start-end  perms  offset  dev  inode  pathname

# Example output:
# 7f3b2c000000-7f3b2c021000 r--p 00000000 fd:01 1234567  /lib/libc.so.6  (private file)
# 7f3b2e000000-7f3b2e100000 rw-p 00000000 00:00 0        [heap]
# 7ffd9e000000-7ffd9e021000 rw-p 00000000 00:00 0        [stack]

# Watch mapping count over time (mmap leak detection)
watch -n1 "wc -l /proc/<PID>/maps"
```

**SIGBUS vs SIGSEGV — distinguish with gdb:**
- `SIGBUS`: fault address is inside the mapping but beyond end-of-file → extend file with `ftruncate()` before mapping
- `SIGSEGV`: fault address is outside any valid VMA

---

### Real-world Usage

- **Program loading:** kernel uses `MAP_PRIVATE` file mapping for `.text` and `.data` segments of every `execve()`. All instances of a program share the same physical pages for code — that is how 1000 `bash` processes fit in RAM.
- **Shared libraries:** `ld.so` maps `libc.so`, `libpthread.so` etc. as `MAP_PRIVATE` file mappings — code pages shared system-wide.
- **Database engines** (PostgreSQL, SQLite WAL): use `MAP_SHARED` file mapping to access pages directly without buffer copies.
- **`malloc` large blocks:** routes through `mmap(MAP_ANONYMOUS)` for allocations > 128KB; `free()` calls `munmap()` to return physical RAM immediately.
- **POSIX Shared Memory** (Ch 7.10): `shm_open()` + `mmap(MAP_SHARED)` is the modern replacement for System V `shmget`.

---

### Key Takeaways

1. **`mmap()` creates a VMA** — no physical pages allocated at call time; demand paging populates pages on first access.
2. **4 mapping types** from the `FILE/ANON × PRIVATE/SHARED` matrix serve distinct purposes: binary loading, mmapped I/O, large malloc, and IPC.
3. **File mappings share page cache** with `read()`/`write()` — no extra copy between kernel and user space.
4. **`close(fd)` after `mmap()` is safe** — the mapping holds an independent reference to the file.
5. **`munmap()` on anonymous private mapping returns RAM immediately** — this is what `free()` exploits for large blocks.
6. **`msync(MS_SYNC)`** before `munmap()` ensures durability of `MAP_SHARED` file writes.
7. **SIGBUS** = valid VMA address but beyond end of backing file. **SIGSEGV** = address outside any VMA.

---

## 5.3 Virtual Memory Operations

### Problem It Solves

After acquiring a memory region (via `malloc` or `mmap`), three additional controls are sometimes needed:

| Need | Syscall |
|---|---|
| Change **access permissions** on an existing region | `mprotect()` |
| **Pin pages into RAM**, prevent swap-out | `mlock()` / `mlockall()` |
| **Advise the kernel** about future access patterns | `madvise()` |

All three operate at the **VMA level** — they modify metadata about how virtual pages are managed, not the data itself.

---

### Concept Overview

Each region of a process's virtual address space is described by a VMA (Virtual Memory Area) in the kernel's `mm_struct`. A VMA records: address range, permissions, backing file (if any), and flags. The syscalls in this topic manipulate these attributes or the physical frame residency of the pages within a VMA.

---

### System Context

- **What it solves:** Fine-grained control over memory region permissions, residency guarantees, and I/O read-ahead behavior — beyond what `mmap()` alone provides at mapping creation time.
- **Where in the system:** All three syscalls operate in the kernel VM subsystem, modifying VMA attributes or page table entries without touching the data in the pages.
- **Subsystem interactions:** `mprotect()` updates page table entries and sends `SIGSEGV` via the hardware protection fault path. `mlock()` interacts with the kernel page reclaim subsystem (prevents a page from entering the LRU eviction list). `madvise()` communicates with the readahead / page-out scheduler.
- **Failure scenarios:** `mprotect()` violations generate `SIGSEGV` immediately. `mlock()` exceeding `RLIMIT_MEMLOCK` returns `ENOMEM` — on embedded/realtime systems this causes a hard failure. `MADV_DONTNEED` silently discards data on MAP_PRIVATE mappings on Linux — unaware callers lose written data.

---

### Internal Mechanism

#### mprotect() — Change Page Protection

```c
#include <sys/mman.h>
int mprotect(void *addr, size_t length, int prot);
// addr: must be page-aligned
// prot: PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC (same as mmap prot)
// Returns 0 on success, -1 on error
```

`mprotect()` does not touch physical memory. It modifies **page table entries** and the VMA's permission flags:

```
Before: VMA [addr, addr+length] prot = PROT_READ|PROT_WRITE
        page table W bit = 1

mprotect(addr, length, PROT_READ)

After:  VMA [addr, addr+length] prot = PROT_READ
        page table W bit = 0  ← kernel updated this

Write attempt → hardware protection fault → kernel delivers SIGSEGV
```

**Guard pages — detecting buffer overflows:**

```c
char *buf = mmap(NULL, BUF_SIZE + PAGE_SIZE, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
mprotect(buf + BUF_SIZE, PAGE_SIZE, PROT_NONE);  /* guard page */
/* Overrun past BUF_SIZE → SIGSEGV immediately */
```

The kernel places a `PROT_NONE` guard page below each thread's stack using this same mechanism — that is how stack overflows produce SIGSEGV rather than silent memory corruption.

**JIT compilers — Write-then-Execute (W^X):**

```c
/* Step 1: write generated machine code */
char *jit_buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
emit_code(jit_buf);

/* Step 2: flip to executable, remove write permission */
mprotect(jit_buf, size, PROT_READ | PROT_EXEC);

/* PROT_WRITE | PROT_EXEC simultaneously is blocked by W^X policy */
```

#### mlock() — Pin Pages into Physical Memory

```c
#include <sys/mman.h>
int mlock(void *addr, size_t length);    /* lock a region */
int munlock(void *addr, size_t length);  /* unlock a region */

int mlockall(int flags);   /* flags: MCL_CURRENT | MCL_FUTURE */
int munlockall(void);
/* Returns 0 on success, -1 on error */
```

`mlock()` faults all pages in the range into RAM **immediately** (unlike demand paging) and marks them non-evictable. The kernel's page reclaim scanner skips locked pages.

Two reasons to lock pages:

**1. Realtime / low-latency:** page faults introduce jitter measured in milliseconds (disk read). Audio drivers, control system loops, and high-frequency trading cannot tolerate this.

**2. Security — prevent sensitive data on disk:**
```
Without mlock(): cryptographic key in RAM → memory pressure → swapped to disk
                                    → process exits → key remains on disk indefinitely
With mlock():    key page pinned in RAM → never written to swap space
```
OpenSSL, GnuPG, and password managers use `mlock()` on key material buffers.

**Privilege and limits:**
```
Kernel < 2.6.9:  only CAP_IPC_LOCK can lock memory
Kernel ≥ 2.6.9:  unprivileged can lock up to RLIMIT_MEMLOCK bytes
                  (default soft limit: 8 pages = 32KB)
```

**Critical semantics:**
- Locks are **not inherited** by `fork()`, not preserved across `exec()`
- Locks do **not nest**: two `mlock()` calls on the same pages → one `munlock()` removes the lock
- With `MAP_SHARED` pages: page stays locked as long as **at least one process** holds a lock

```c
mlockall(MCL_CURRENT | MCL_FUTURE);  /* lock everything — used by realtime daemons */
```

| Flag | Meaning |
|---|---|
| `MCL_CURRENT` | Lock all pages currently mapped in the process |
| `MCL_FUTURE` | Lock all pages mapped in the future (new `mmap`, stack growth, heap) |

#### madvise() — Hint the Kernel's Page Management

```c
#define _BSD_SOURCE
#include <sys/mman.h>
int madvise(void *addr, size_t length, int advice);
/* addr: page-aligned; length: rounded up to page size */
/* Returns 0 on success, -1 on error */
```

`madvise()` is a **hint** — the kernel may ignore it. Most advice values do not affect program semantics; they only influence performance.

| Advice | Effect on kernel |
|---|---|
| `MADV_NORMAL` | Default: moderate read-ahead (a few pages) |
| `MADV_RANDOM` | Disable read-ahead; fetch minimum on each fault |
| `MADV_SEQUENTIAL` | Aggressive read-ahead; free pages after access |
| `MADV_WILLNEED` | Prefetch pages now (read-ahead *before* access) |
| `MADV_DONTNEED` | **Discard pages** (Linux-specific semantics — see below) |

**`MADV_DONTNEED` — the only advice with real side effects on Linux:**

```c
char *buf = mmap(NULL, 1024*1024, PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
memset(buf, 0xFF, 1024*1024);  /* pages are dirty, all 0xFF */

madvise(buf, 1024*1024, MADV_DONTNEED);  /* discard physical pages */

printf("%d\n", buf[0]);  /* page fault → new zero-filled page → prints 0, not 255 */
```

Virtual address range **remains valid** — but the physical pages are discarded. Next access: page fault → zero-filled page (anonymous) or re-read from file (file mapping).

This is used to **return RAM to the kernel without `munmap()`** — useful when a large buffer will be reused but is temporarily idle:

```c
/* Server processed request; large buffer idle but will be needed again */
madvise(large_buf, buf_size, MADV_DONTNEED);  /* RAM returned to kernel */
/* virtual address range still valid; next use will page-fault back in */
```

#### mincore() — Query Which Pages Are Resident

```c
int mincore(void *addr, size_t length, unsigned char *vec);
/* vec: 1 byte per page; bit 0 = 1 → page is resident in RAM */
```

```c
long page_sz = sysconf(_SC_PAGESIZE);
int  n_pages  = (length + page_sz - 1) / page_sz;
unsigned char vec[n_pages];

mincore(addr, length, vec);
for (int i = 0; i < n_pages; i++)
    printf("page %d: %s\n", i, (vec[i] & 1) ? "resident" : "swapped/not-loaded");
```

Primary use: verify `mlock()` succeeded and all pages are truly pinned.

---

### Architecture / Execution Flow

```
VMA in mm_struct
┌─────────────────────────────────────────────────────┐
│  vm_start  vm_end  vm_prot  vm_flags  vm_file        │
│                        ↑                             │
│                  mprotect() modifies vm_prot         │
│                  + updates page table R/W/X bits     │
└─────────────────────────────────────────────────────┘
              │
              ▼  page table entries → physical frames
┌─────────────────────────────────────────────────────┐
│  Physical pages (resident in RAM)                   │
│                                                     │
│  mlock()   → mark pages non-evictable               │
│  madvise() → hint readahead / discard policy        │
│  mincore() → read residency status of each page     │
└─────────────────────────────────────────────────────┘
              │
              ▼  swap/eviction path
┌─────────────────────────────────────────────────────┐
│  Swap space (disk)                                  │
│  ← mlock'd pages never reach here                  │
└─────────────────────────────────────────────────────┘
```

---

### Example

```c
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096

int main(void) {
    size_t size = 8 * PAGE_SIZE;

    /* Allocate 8 pages */
    char *buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return 1; }

    /* --- mprotect: guard last page --- */
    if (mprotect(buf + size - PAGE_SIZE, PAGE_SIZE, PROT_NONE) == -1) {
        perror("mprotect"); return 1;
    }
    /* writing to buf[size - PAGE_SIZE] now → SIGSEGV */

    /* --- mlock: pin first 4 pages --- */
    if (mlock(buf, 4 * PAGE_SIZE) == -1) {
        perror("mlock");  /* EPERM if RLIMIT_MEMLOCK exceeded */
    }

    /* --- mincore: verify residency --- */
    int n_pages = size / PAGE_SIZE;
    unsigned char vec[n_pages];
    if (mincore(buf, size, vec) == 0) {
        for (int i = 0; i < n_pages; i++)
            printf("page %d: %s\n", i, (vec[i] & 1) ? "resident" : "not resident");
    }

    /* --- madvise: hint sequential scan on pages 1-3 --- */
    madvise(buf + PAGE_SIZE, 3 * PAGE_SIZE, MADV_SEQUENTIAL);

    /* --- madvise DONTNEED: reclaim pages 1-3 (temporary idle buffer) --- */
    memset(buf + PAGE_SIZE, 0xAB, 3 * PAGE_SIZE);
    madvise(buf + PAGE_SIZE, 3 * PAGE_SIZE, MADV_DONTNEED);
    /* buf[PAGE_SIZE] is now 0 again (zero-fill on next access) */

    munlock(buf, 4 * PAGE_SIZE);
    munmap(buf, size);
    return 0;
}
```

---

### Debugging

```bash
# Check locked memory for a process
grep VmLck /proc/<PID>/status

# Verify page residency distribution
# (check that mlock'd pages show as 'resident')
cat /proc/<PID>/smaps | grep -A 15 "locked"

# Check current RLIMIT_MEMLOCK
ulimit -l           # in shell (KB)
cat /proc/<PID>/limits | grep "Max locked"

# Raise limit (as root) for testing realtime code
ulimit -l unlimited
```

---

### Real-world Usage

- **`mprotect()` guard pages:** stack overflow detection in glibc, ASAN (AddressSanitizer) uses shadow memory + `mprotect` to catch out-of-bounds. OpenBSD enables guard pages by default between all heap allocations.
- **`mprotect()` JIT:** V8 (Node.js/Chrome), LuaJIT, Java HotSpot — all use `MAP_ANONYMOUS` + write code + `mprotect(PROT_READ|PROT_EXEC)` to implement W^X.
- **`mlock()` cryptography:** OpenSSL `CRYPTO_secure_malloc()`, gpg-agent, KeePassXC lock key material pages to prevent them appearing in swap.
- **`mlock()` realtime:** JACK audio server, Xenomai RTOS applications, Linux `SCHED_FIFO` daemons call `mlockall(MCL_CURRENT|MCL_FUTURE)` at startup to eliminate all page fault jitter.
- **`madvise(MADV_SEQUENTIAL)`:** `cat`, `cp` on large files. PostgreSQL sequential scans hint `MADV_SEQUENTIAL` to let the kernel aggressively prefetch and free behind.
- **`madvise(MADV_DONTNEED)`:** jemalloc and tcmalloc call `MADV_DONTNEED` to return physical RAM for idle arenas without changing virtual address layout — the approach used by many high-performance servers.

---

### Key Takeaways

1. **`mprotect()`** updates page table permission bits on an existing VMA — violation delivers `SIGSEGV`. Core technique for guard pages and JIT W^X enforcement.
2. **`mlock()`** pins pages into RAM: all pages are faulted in immediately and excluded from kernel eviction. Does not nest; not inherited across `fork()`/`exec()`.
3. **`RLIMIT_MEMLOCK`** limits lockable memory for unprivileged processes (default 32KB). Realtime daemons typically run with this raised or use `CAP_IPC_LOCK`.
4. **`madvise()`** is a hint to the kernel's readahead/eviction policy — does not alter program semantics, except `MADV_DONTNEED` on Linux which **discards physical pages** of `MAP_PRIVATE` mappings (virtual range remains valid, next access gets zero-filled pages).
5. **`MADV_DONTNEED`** is the mechanism for returning RAM to the kernel while keeping virtual address range — used by high-performance allocators (jemalloc, tcmalloc) to reclaim idle memory without `munmap()`.
6. **`mincore()`** queries page residency — primary use is verifying `mlock()` succeeded.
