# Chapter 5 Interview - Memory

> Scope: Linux virtual memory, process memory layout, heap ownership, memory mappings, page protection, memory locking, memory advice, and production memory debugging.
> Main repo source: `knowledge/ch05_memory.md`.
> Mapped sources: TLPI-derived `ch07_memory_allocation.md`, `ch49_memory_mappings.md`, `ch50_virtual_memory_operations.md`, plus DevLinux Module 03 process memory layout.

---

## Review Basis

This interview set is intentionally selective. It does not turn every Chapter 5 detail into an interview question; it keeps the items most likely to matter in Linux System, Embedded Linux, backend daemon, and systems software interviews.

Repo and book-derived sources used for correctness:

- `LINUX_SYSTEM_LEARNING_MAP.md`: Chapter 5 maps to memory allocation, memory mappings, and virtual memory operations.
- `knowledge/ch05_memory.md`: process memory layout, heap ownership, mapping matrix, page faults, VM operations, debugging commands, and production failure patterns.
- `docs/Linux-Programming-Interface/ch07_memory_allocation.md`: `malloc()`, `calloc()`, `realloc()`, `free()`, `brk()`, `sbrk()`, allocator metadata, leak/corruption rules, and `alloca()`.
- `docs/Linux-Programming-Interface/ch49_memory_mappings.md`: `mmap()`, `munmap()`, file-backed versus anonymous mappings, `MAP_PRIVATE`, `MAP_SHARED`, copy-on-write, `msync()`, `SIGSEGV`, `SIGBUS`, `MAP_FIXED`, `MAP_NORESERVE`, overcommit, and OOM behavior.
- `docs/Linux-Programming-Interface/ch50_virtual_memory_operations.md`: `mprotect()`, guard pages, `mlock()`, `mlockall()`, `RLIMIT_MEMLOCK`, `mincore()`, and `madvise()`.
- `docs/Linux-Programming-DevLinux/INDEX.md`, `README.md`, and `03-Linux-Process/README.md`: practical process memory layout, `/proc/<PID>/maps`, process isolation, and copy-on-write context.

Official technical references used to verify API semantics:

- Linux man-pages: [`malloc(3)`](https://man7.org/linux/man-pages/man3/malloc.3.html), [`brk(2)`](https://man7.org/linux/man-pages/man2/brk.2.html), [`mmap(2)` / `munmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html), [`mprotect(2)`](https://man7.org/linux/man-pages/man2/mprotect.2.html), [`mlock(2)`](https://man7.org/linux/man-pages/man2/mlock.2.html), [`madvise(2)`](https://man7.org/linux/man-pages/man2/madvise.2.html), [`proc_pid_maps(5)`](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html), [`proc_pid_smaps(5)`](https://man7.org/linux/man-pages/man5/proc_pid_smaps.5.html), [`proc_pid_status(5)`](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html), and [`core(5)`](https://man7.org/linux/man-pages/man5/core.5.html).
- Official tooling docs: [Valgrind Memcheck manual](https://valgrind.org/docs/manual/mc-manual.html), [LLVM AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html), [LLVM LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html), and Red Hat documentation on [Valgrind](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/developer_guide/valgrind), [memory monitoring](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/system_administrators_guide/ch-system_monitoring_tools), and [memory diagnostics](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/performance_tuning_guide/sect-red_hat_enterprise_linux-performance_tuning_guide-memory-monitoring_and_diagnosing_performance_problems).

Interview calibration sources:

- Amazon official SDE prep lists operating systems among technical topics and emphasizes applying fundamentals: <https://amazon.jobs/content/en/how-we-hire/interview-prep/software-development-topics>
- Microsoft technical interviewing guidance emphasizes principles, problem solving, testing, and communication: <https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html>
- Meta official full-loop preparation page frames interviews around technical skill and building at Meta: <https://www.metacareers.com/careers/SWE-prep-onsite>
- Google career resources were used as general preparation context: <https://www.google.com/about/careers/applications/buildyourfuture/resources/>
- Common OS interview banks were used only to detect recurring themes, not as technical authority.

---

## Coverage Trace

| Coverage item | Source | Required interview coverage | Priority coverage |
|---------------|--------|-----------------------------|-------------------|
| 5.1 Memory Allocation - `malloc/free`, heap, `brk()`, memory leak | Learning map, TLPI Ch07, reviewed `knowledge/ch05_memory.md` | Heap ownership, allocator metadata, leaks, fragmentation, safe growth, corruption debugging, `brk/sbrk` background | A1, A2, A7, B10-B15, C23, C27 |
| 5.2 Memory Mappings - `mmap()`, file mapping, device memory | Learning map, TLPI Ch49, reviewed `knowledge/ch05_memory.md` | File-backed, anonymous, private/shared, device-backed mappings, `munmap()` cleanup, `msync()`, `SIGBUS`, overcommit/OOM, lifecycle across `fork()/exec()/exit` | A3-A7, A9, B10, B16-B20, C24-C26 |
| 5.3 Virtual Memory Ops - `mprotect()`, `mlock()`, `madvise()` | Learning map, TLPI Ch50, reviewed `knowledge/ch05_memory.md` | Page protection, guard pages, locked memory, limits, page residency, `mincore()`, memory advice, security/performance tradeoffs | A5, A8, B18, B21-B22, C25 |
| Heap allocation model: `malloc()`, `free()`, `brk()`, allocator metadata, leaks, fragmentation, ownership | Chapter 5 Must Cover | Scenario/debug coverage for daemon growth and allocator crashes; comparison coverage for stack/heap/mapping, safe `realloc()`, `calloc()`, `brk/sbrk`, and `free()` behavior | A1, A2, A7, B10-B15, C23, C27 |
| Virtual memory fundamentals: address space, pages, mappings, protection, page faults, overcommit, copy-on-write connections to `fork()` | Chapter 5 Must Cover | Scenario/comparison coverage for VIRT vs RSS, demand paging, normal versus fatal page faults, protection failures, overcommit/OOM, and COW after `fork()` | A3-A6, A8, B10-B11, B16, B18-B21, C24-C25 |
| `mmap()` use cases: anonymous mapping, file mapping, shared/private mapping, device memory, cleanup | Chapter 5 Must Cover | Scenario/comparison coverage for large-file mapping, parent/child shared mapping, device-backed mapping, mapping cleanup, and private/shared tradeoffs | A4-A7, A9, B10, B16-B20, C24-C26 |
| Virtual memory operations: `mprotect()`, `mlock()`, `madvise()`, page residency, and security/performance tradeoffs | Chapter 5 Must Cover | Scenario/debug coverage for guard pages, locked secrets/realtime pages, `MADV_DONTNEED`, major-fault latency, residency checks, and dump/security policy | A8, B18, B21-B22, C25 |
| Embedded constraints: limited RAM/no swap, memory pressure, OOM behavior, deterministic allocation, and target-appropriate debugging | Chapter 5 Must Cover, DevLinux Module 03, reviewed `knowledge/ch05_memory.md` | Scenario coverage for bounded allocation, OOM debugging, stripped target symbols, driver/device mappings, and `/proc`, `pmap`, Valgrind, ASan, and target tools | A1-A2, A7-A9, B21 |

---

## Priority Map

### A - Project and production scenarios

Study these deeply. The interviewer is usually testing whether you can reason from a real symptom to the right memory model, not whether you can recite an API prototype.

- A daemon's RSS grows over hours or days.
- A crash happens in `free()` or `malloc()` under load.
- A process has huge VIRT/VmSize but stable RSS.
- A file-mapping reader crashes with `SIGBUS` after a file is truncated or rotated.
- A project must choose between `read()/write()` and `mmap()` for a large file.
- Two processes share memory and sometimes see inconsistent data.
- An embedded target is killed by OOM or slows down due to page faults, no swap, fragmentation, or limited RAM.
- A security/realtime component needs guard pages, locked memory, or memory advice without overengineering.
- A device-backed mapping fails only on target hardware or after a driver/firmware change.

### B - Design comparisons and senior follow-ups

Know these well enough to discuss trade-offs, constraints, and failure modes.

- Stack versus heap versus mapping lifetime.
- File-backed versus anonymous mapping.
- `MAP_PRIVATE` versus `MAP_SHARED`.
- `calloc()` versus `malloc()` plus `memset()`.
- Safe `realloc()` and growable-buffer design.
- Why `free()` does not necessarily return memory to the OS.
- Program break, `brk()`, and `sbrk()` as allocator background.
- `mprotect()` guard pages and page granularity.
- `mlock()`, `mlockall()`, `RLIMIT_MEMLOCK`, and `VmLck`.
- `madvise()` hints, especially `MADV_DONTNEED`.
- `mincore()` page-residency evidence versus RSS/VmLck.
- Core dumps, `/proc/<PID>/coredump_filter`, and secret data exposure.

### C - Lower-priority / know enough to recognize

Recognize these names and risks; do not spend first-pass interview time memorizing flag tables.

- `alloca()`, `mallopt()`, `malloc_trim()`, `malloc_info()`.
- `MAP_FIXED`, `MAP_POPULATE`, `MAP_NORESERVE`, `MAP_LOCKED`, `MAP_HUGETLB`, `MAP_UNINITIALIZED`.
- `mremap()`, `mincore()`, nonlinear mappings, `remap_file_pages()`.
- Deep OOM killer scoring and allocator-tuning internals.
- Huge pages, DAX mappings, memory protection keys, and custom malloc replacement rules.

---

## Final Interview List

### Priority A

1. A long-running service's RSS grows after every request batch. How do you decide whether this is a leak, allocator retention, cache growth, fragmentation, or normal file-cache behavior?
2. A production process crashes inside `free()` during peak traffic. How do you investigate heap corruption, double-free, use-after-free, and buffer overflow?
3. A process shows huge VIRT/VmSize but modest RSS. Is it using too much RAM? How do virtual memory, physical memory, address-space isolation, and demand paging explain this?
4. A file-backed `mmap()` reader sometimes dies with `SIGBUS` after log rotation or firmware update. What happened and how would you fix it?
5. Your team wants to replace `read()/write()` with `mmap()` for a large data file. What workload and failure-mode questions do you ask before approving it?
6. Parent and child processes need fast shared state. How would you choose and use `MAP_SHARED`, anonymous mapping, file-backed mapping, or POSIX shared memory?
7. An embedded daemon has limited RAM, no swap, stripped binaries, and occasional OOM kills. How do you design and debug memory use?
8. A realtime or security-sensitive component wants `mprotect()`, `mlock()`, and `madvise()`. Which parts are justified, and what can go wrong?
9. A userspace driver tool maps device memory and crashes or reads stale data only on target hardware. How do you debug the mapping contract?

### Priority B

10. Compare stack, heap, and memory mappings.
11. Explain text, data, BSS, heap, stack, mmap region, and kernel space in a Linux process.
12. Why can `free()` fail to reduce RSS immediately?
13. How should `realloc()` be used safely in a growable buffer?
14. When is `calloc()` better than `malloc()`?
15. What are `brk()` and `sbrk()`, and why should normal application code avoid them?
16. Compare `MAP_PRIVATE` and `MAP_SHARED`.
17. Compare file-backed and anonymous mappings.
18. What causes `SIGSEGV` versus `SIGBUS`?
19. How do mappings behave across `fork()`, `exec()`, and process exit?
20. What does `msync()` guarantee, and what does it not guarantee?
21. A process has stable RSS but increasing latency due to major page faults. How do `mincore()`, RSS, and `VmLck` help, and what would you change?
22. How can core dumps help debugging and also leak secrets?

### Priority C

23. What is `alloca()`, and why is it risky for untrusted or large sizes?
24. What are `MAP_FIXED`, `MAP_POPULATE`, and `MAP_NORESERVE` used for?
25. What are `mremap()` and `mincore()` useful for?
26. What are huge pages and `MAP_HUGETLB`?
27. What are allocator tuning functions such as `mallopt()` and `malloc_trim()` for?

---

## High-Value Comparisons

| Comparison | Interview-grade answer |
|------------|------------------------|
| Virtual memory vs physical memory | Virtual memory is the private address space a process sees. Physical memory is RAM. Page tables and the MMU translate virtual pages to RAM frames, page cache, swap, or no resident page yet. |
| VmSize/VIRT vs RSS | VmSize/VIRT is address space mapped or reserved. RSS is the resident portion currently in RAM. A high VIRT is not automatically a leak. |
| Stack vs heap | Stack is automatic and call-frame scoped. Heap is explicit and ownership scoped through allocator APIs. |
| Heap vs mmap region | Heap is managed by the C allocator. An mmap region is a kernel VMA created by `mmap()` and removed by `munmap()`. |
| `malloc()` vs `mmap()` | `malloc()` is the normal dynamic-allocation API. `mmap()` is for file mappings, shared memory, large/page-level regions, or explicit VM control. |
| `malloc()` vs `calloc()` | `malloc()` returns uninitialized storage. `calloc()` returns zero-filled array storage and detects multiplication overflow in the array-size calculation. |
| `free()` vs `munmap()` | `free()` returns a heap block to the allocator. `munmap()` removes a virtual memory mapping from the process. |
| `MAP_PRIVATE` vs `MAP_SHARED` | `MAP_PRIVATE` hides writes through copy-on-write. `MAP_SHARED` makes writes visible through shared mappings and can propagate file-backed changes to the file. |
| File-backed vs anonymous mapping | File-backed mappings are initialized from a file or device. Anonymous mappings have no file and start zero-filled. |
| `SIGSEGV` vs `SIGBUS` | `SIGSEGV` usually means invalid address or permission violation. `SIGBUS` commonly means the address is inside a mapping but the backing object cannot supply that page. |
| `mprotect()` vs `mlock()` | `mprotect()` changes page permissions. `mlock()` keeps pages resident and prevents swap. |
| `madvise()` vs `posix_fadvise()` | `madvise()` describes access patterns for a memory range. `posix_fadvise()` describes expected file I/O through a file descriptor. |

---

## Common Project Failure Patterns

| Failure pattern | Why it happens | Best first checks |
|-----------------|----------------|-------------------|
| RSS grows every request | leak, unbounded cache, allocator arenas, fragmentation, or touched anonymous pages | `/proc/<PID>/status`, `smaps_rollup`, `smaps`, heap profiler, ASan/LSan, Valgrind |
| Crash inside allocator | earlier overflow, double-free, invalid free, or use-after-free damaged allocator metadata | ASan, Valgrind, core dump, allocator abort message, recent ownership changes |
| Stack overflow | deep recursion, huge local array, or small thread stack | core dump, `ulimit -s`, thread stack size, backtrace |
| Buffer overflow corrupts adjacent memory | C does not track object bounds | compiler warnings, ASan, bounds checks, safer length-carrying APIs |
| `mmap()` file reader gets `SIGBUS` | mapped page no longer has file backing after short file, truncation, or rotation | fault address, `/proc/<PID>/maps`, file size, `strace`, core dump |
| Missing `munmap()` | mapping lifetime is not tied to heap ownership | `/proc/<PID>/maps`, `pmap -x`, `strace -e mmap,munmap` |
| Device mapping stale data/crash | driver ABI mismatch, wrong offset/window, cache/coherency issue, DMA ownership bug, hot-unplug | `/proc/<PID>/maps`, `strace -e open,ioctl,mmap,munmap`, `dmesg`, `/sys`, driver logs |
| Confusing VIRT with RSS | reserved address space is counted as virtual memory before pages are resident | `top`, `/proc/<PID>/status`, `smaps_rollup`, `smaps` |
| OOM killer on target | overcommit, cgroup limit, no swap, burst allocation, or leak | `dmesg`, cgroup logs, `/proc/<PID>/oom_score`, `free`, `vmstat` |
| Fragmentation in long-running process | repeated mixed-size allocations leave reusable but poorly shaped free space | allocation profile, pools/arenas, bounded object lifetimes |
| Secret data in memory or core dump | buffers, swap, or dumps retain sensitive bytes | `mlock()`, explicit zeroing, `MADV_DONTDUMP`, core-dump policy |

---

## Detailed Answers - Priority A

### 1. A long-running service's RSS grows after every request batch. How do you decide whether this is a leak, allocator retention, cache growth, fragmentation, or normal file-cache behavior?

**What the interviewer is testing**

They are testing whether you can separate virtual address space from resident memory, and whether you debug from evidence instead of calling every growth pattern a leak.

**Strong answer**

I would first determine what is growing: VIRT/VmSize, RSS, anonymous RSS, file-backed RSS, shared memory, or locked memory. A real leak usually grows with repeated operations and does not plateau. Allocator retention can keep RSS high after `free()` because memory returns to allocator arenas, not necessarily to the kernel. Bounded caches should grow and then stabilize. File-backed RSS may come from mapped files or shared libraries and is not the same as private heap growth.

**Mechanism**

Linux gives each process a virtual address space. `malloc()` and `mmap()` can reserve ranges before all pages are resident. Pages become resident after faults. `free()` returns heap blocks to the allocator, and only some allocator states allow memory to be returned to the kernel through lowering the program break or unmapping large anonymous mappings.

**Pitfalls**

Do not debug only from `top` VIRT. Do not assume `free()` must immediately reduce RSS. Do not ignore cgroup memory limits on services. In embedded systems, no swap and small RAM make allocator retention and fragmentation more visible.

**Debug angle**

Start with `cat /proc/<PID>/status`, `/proc/<PID>/smaps_rollup` if available, `/proc/<PID>/smaps`, and `pmap -x <PID>`. Compare `VmSize`, `VmRSS`, `RssAnon`, `RssFile`, `RssShmem`, `VmSwap`, and `VmLck`. Reproduce under `ASAN_OPTIONS=detect_leaks=1`, LeakSanitizer, or `valgrind --leak-check=full --show-leak-kinds=all`. Use `strace -e trace=brk,mmap,munmap,mprotect` to see whether address-space growth comes from heap or mappings.

**Follow-up keywords**

`RSS`, `VmSize`, `VIRT`, `RssAnon`, `smaps`, `smaps_rollup`, `pmap`, `malloc()`, `free()`, allocator arenas, overcommit, OOM killer.

### 2. A production process crashes inside `free()` during peak traffic. How do you investigate heap corruption, double-free, use-after-free, and buffer overflow?

**What the interviewer is testing**

They want to know whether you understand that allocator crashes often happen after the real bug, not necessarily at the bad write.

**Strong answer**

I would treat a crash in `free()` or `malloc()` as a symptom of possible earlier heap corruption. The key suspects are writing past an allocation, freeing the same pointer twice, freeing a pointer not returned by the malloc family, using memory after `free()`, or losing ownership across modules. I would reproduce with sanitizers first, then use Valgrind if sanitizers are unavailable or the target cannot be rebuilt easily.

**Mechanism**

Allocators keep metadata near or inside heap blocks and maintain free lists or arenas. A buffer overflow or invalid free can corrupt this metadata. The allocator may only notice later when it coalesces blocks, follows a corrupted free-list pointer, validates a chunk, or reuses memory.

**Pitfalls**

Do not "fix" the line that crashed without finding the ownership violation. Do not pass stack, global, already-freed, or interior pointers to `free()`. Do not assign `realloc()` directly to the only pointer. Do not forget that `realloc()` may move the block and invalidate interior pointers.

**Debug angle**

Build with `-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer`. Use ASan reports to find allocation, free, and invalid-access stacks. Use Valgrind Memcheck when recompilation is hard, accepting the slowdown. Inspect core dumps with `gdb`, `bt`, `info proc mappings`, and allocator error messages. On a stripped target, collect symbols from the matching host build or debug package.

**Follow-up keywords**

`malloc()`, `calloc()`, `realloc()`, `free()`, `reallocarray()`, double-free, use-after-free, heap overflow, allocator metadata, ASan, Valgrind, core dump.

### 3. A process shows huge VIRT/VmSize but modest RSS. Is it using too much RAM? How do virtual memory, physical memory, address-space isolation, and demand paging explain this?

**What the interviewer is testing**

They are testing the mental model of virtual memory and the ability to avoid false alarms in production.

**Strong answer**

Huge VIRT does not necessarily mean huge RAM usage. VIRT/VmSize counts mapped or reserved virtual address ranges. RSS counts resident pages in RAM. A process may reserve a large sparse mapping, map large files, load shared libraries, or use sanitizer/runtime address reservations while touching only a small fraction.

**Mechanism**

Each process has its own virtual address space. Page tables map virtual pages to physical frames, file-cache pages, swap, or no resident page yet. Demand paging means a valid virtual range may not consume RAM until first access. A page fault can be normal if the mapping is valid and just needs a page loaded or allocated.

**Pitfalls**

Do not compare address values between processes as if they reference the same physical memory. Do not treat all page faults as crashes. Do not ignore RSS growth just because VIRT is expected; growing private dirty anonymous RSS is usually more suspicious.

**Debug angle**

Use `top` or `ps` only as a starting point. Confirm with `/proc/<PID>/status`, `/proc/<PID>/maps`, `/proc/<PID>/smaps`, and `pmap -x <PID>`. In `maps`, look for `[heap]`, `[stack]`, shared libraries, deleted files, anonymous mappings, and permissions such as `r-xp`, `rw-p`, `rw-s`.

**Follow-up keywords**

Virtual address space, physical RAM, MMU, page table, VMA, page fault, demand paging, copy-on-write, `/proc/<PID>/maps`, `/proc/<PID>/smaps`.

### 4. A file-backed `mmap()` reader sometimes dies with `SIGBUS` after log rotation or firmware update. What happened and how would you fix it?

**What the interviewer is testing**

They are testing whether you know the difference between invalid virtual access and missing file backing.

**Strong answer**

A likely cause is that the process mapped a file region and later touched a page for which the underlying file no longer has backing storage. This can happen when the file was too small, not extended with `ftruncate()`, or was truncated by another process after mapping. `SIGBUS` is the classic signal for this file-mapping failure.

**Mechanism**

`mmap()` creates a VMA. For a file-backed mapping, pages are supplied from the file through the page cache. If an address is inside the mapping but the corresponding file page does not exist, Linux can deliver `SIGBUS`. Access outside any mapping or against mapping permissions usually gives `SIGSEGV`.

**Pitfalls**

Do not map an output file and then write beyond its current size without first extending it. Do not assume closing the fd invalidates the mapping; after successful `mmap()`, closing the fd is fine. Do not assume log rotation is harmless if the producer truncates the same inode that readers mapped.

**Debug angle**

Inspect the signal and fault address in the core dump. Compare it against `/proc/<PID>/maps` or `gdb info proc mappings`. Check the mapped file size and whether it was truncated. Use `strace -e trace=open,stat,fstat,ftruncate,mmap,munmap` around the reproducer. Fix by validating file size, handling empty files, mapping only valid ranges, extending writable files with `ftruncate()`, and coordinating rotation/truncation.

**Follow-up keywords**

`mmap()`, `munmap()`, file-backed mapping, `fstat()`, `ftruncate()`, `SIGBUS`, `SIGSEGV`, `MAP_SHARED`, `MAP_PRIVATE`, `core(5)`.

### 5. Your team wants to replace `read()/write()` with `mmap()` for a large data file. What workload and failure-mode questions do you ask before approving it?

**What the interviewer is testing**

They want trade-off judgment, not blind belief that `mmap()` is faster.

**Strong answer**

I would ask whether access is repeated and random, whether multiple processes share the same data, whether pointer-style access simplifies code, and whether page-fault latency is acceptable. I would keep `read()/write()` for simple sequential or streaming I/O unless measurement shows `mmap()` helps. I would prefer `mmap()` for large read-mostly indexes, shared persistent regions, or data structures where mapping avoids extra copies and simplifies random access.

**Mechanism**

`read()` and `write()` expose explicit I/O syscalls and error points. `mmap()` maps file pages into the process, and I/O is triggered by page faults and writeback. `mmap()` can avoid one copy between kernel and user space, but it adds VMA setup, page faults, TLB effects, and trickier crash/writeback semantics.

**Pitfalls**

Small one-shot mappings can be slower than simple I/O. Writable mappings need durability rules. A mapped file that is truncated can crash readers. Mixing mapped writes with normal I/O is easier on Linux's unified page cache than on some systems, but portable designs still need explicit synchronization.

**Debug angle**

Measure with realistic file size and access pattern. Use `perf`, page-fault counters, `strace`, and application latency traces. For mapped files, observe `/proc/<PID>/maps`, `smaps`, major/minor faults, `msync()` behavior, and storage writeback.

**Follow-up keywords**

`read()`, `write()`, `mmap()`, page fault, page cache, TLB, `msync()`, `MAP_SHARED`, random access, sequential access.

### 6. Parent and child processes need fast shared state. How would you choose and use `MAP_SHARED`, anonymous mapping, file-backed mapping, or POSIX shared memory?

**What the interviewer is testing**

They are testing whether you distinguish sharing bytes from synchronizing state.

**Strong answer**

For related processes after `fork()`, `MAP_SHARED | MAP_ANONYMOUS` is simple and avoids a disk file. For unrelated processes, POSIX shared memory with `shm_open()` plus `mmap()` is usually cleaner than inventing a file convention. For persistent shared state, a `MAP_SHARED` file mapping can be appropriate, but then the design must handle file sizing, `msync()` or other durability rules, and crash consistency.

**Mechanism**

A child inherits mappings across `fork()`. Private mappings use copy-on-write, so normal variables diverge after writes. Shared mappings point processes at the same physical or page-cache pages. `exec()` removes old mappings, and process exit removes mappings automatically.

**Pitfalls**

`MAP_SHARED` is not a lock. Without a protocol, processes can race, observe torn data, or read half-updated structures. Pointers stored in shared memory are usually wrong for unrelated processes because each process may map the region at a different virtual address; offsets are safer.

**Debug angle**

Inspect both processes' `/proc/<PID>/maps`. Use `strace -e mmap,munmap,futex,semop,sem_wait,sem_post` depending on synchronization. Reproduce with stress tests. Add process-shared pthread mutexes, semaphores, atomics with clear memory ordering, or a single-writer protocol.

**Follow-up keywords**

`MAP_SHARED`, `MAP_PRIVATE`, `MAP_ANONYMOUS`, copy-on-write, `fork()`, `exec()`, POSIX shared memory, `shm_open()`, process-shared mutex, semaphore, atomic.

### 7. An embedded daemon has limited RAM, no swap, stripped binaries, and occasional OOM kills. How do you design and debug memory use?

**What the interviewer is testing**

They are testing whether you can adapt desktop/server memory habits to constrained targets.

**Strong answer**

I would design for bounded memory first: fixed upper limits, pools or arenas for hot paths, clear ownership, cleanup-on-error, preallocation for realtime paths, and no unbounded per-request growth. I would avoid dynamic allocation in critical loops when predictable latency matters. For debugging, I would capture target memory statistics and OOM logs, then reproduce under instrumented host builds with matching library versions when possible.

**Mechanism**

On a no-swap system, touching overcommitted pages can lead quickly to OOM. Fragmentation can make long-running allocation patterns expensive even if total freed memory looks sufficient. Stripped binaries hide symbols unless matching debug info is kept. Cross-target differences in libc, allocator, page size, cgroup limits, or kernel config can change symptoms.

**Pitfalls**

Do not rely on "the process exits eventually" for a daemon. Do not assume host ASan behavior exactly matches target behavior. Do not leave memory-mapped files, buffers, or shared-memory objects without clear lifecycle cleanup. Do not enable broad `mlockall(MCL_FUTURE)` without testing locked-memory limits.

**Debug angle**

On target, collect `free`, `vmstat`, `top`, `/proc/<PID>/status`, `/proc/<PID>/smaps_rollup`, `/proc/<PID>/maps`, `dmesg`, and cgroup memory events. Use `strace -e trace=brk,mmap,munmap,mprotect` if feasible. On host, run ASan/LSan or Valgrind with representative workloads. Keep unstripped symbols and build IDs for `gdb` and `coredumpctl`.

**Follow-up keywords**

Embedded Linux, no swap, RSS, OOM killer, cgroups, fragmentation, pools, arenas, stripped binaries, target/host mismatch, `dmesg`, `vmstat`.

### 8. A realtime or security-sensitive component wants `mprotect()`, `mlock()`, and `madvise()`. Which parts are justified, and what can go wrong?

**What the interviewer is testing**

They want balanced judgment around specialized VM APIs.

**Strong answer**

`mprotect()` is justified for page-level permission control such as guard pages or making a region read-only after initialization. `mlock()` is justified for small secrets or latency-sensitive pages that must stay resident. `madvise()` is justified after measurement when the kernel can benefit from access-pattern hints. I would keep these scoped and tested rather than applying them broadly.

**Mechanism**

`mprotect()` works at page granularity and invalid access raises `SIGSEGV`. `mlock()` locks whole pages in RAM and is limited by `RLIMIT_MEMLOCK`, privileges, and available memory; `VmLck` reports locked memory in `/proc/<PID>/status`. `madvise()` gives hints such as random, sequential, will-need, or do-not-need. On Linux, `MADV_DONTNEED` can discard private modified pages and reload file contents or zero-fill anonymous memory on the next fault.

**Pitfalls**

Do not put unrelated objects on the same page if you plan to independently protect or lock them. Do not assume `mlock()` protects secrets from hibernation or core dumps. Do not use `mlockall(MCL_FUTURE)` casually; later `malloc()`, `mmap()`, or stack growth can fail or fault. Do not treat `MADV_DONTNEED` as a harmless cache hint for data whose contents still matter.

**Debug angle**

Use `/proc/<PID>/maps` for permissions, `/proc/<PID>/status` for `VmLck`, `/proc/<PID>/smaps` for locked and advice-related VMA flags, `ulimit -l` or `prlimit` for `RLIMIT_MEMLOCK`, and `strace -e mprotect,mlock,mlockall,munlock,madvise`. For crashes, use `gdb` and the fault address.

**Follow-up keywords**

`mprotect()`, `PROT_NONE`, guard page, `mlock()`, `mlockall()`, `RLIMIT_MEMLOCK`, `CAP_IPC_LOCK`, `madvise()`, `MADV_DONTNEED`, `MADV_DONTDUMP`.

### 9. A userspace driver tool maps device memory and crashes or reads stale data only on target hardware. How do you debug the mapping contract?

**What the interviewer is testing**

They are testing whether you know that device-backed `mmap()` is a driver and hardware contract, not just normal file mapping with a stranger pathname.

**Strong answer**

I would first verify the driver ABI: which device node is opened, which `ioctl()` or setup call selects the buffer/register window, what `mmap()` offset and length mean, and what access protocol the driver expects. For device registers or DMA buffers, stale data can be a cache/coherency or ownership problem, not a plain C pointer problem. I would prefer a documented driver interface over raw `/dev/mem` mappings.

**Mechanism**

The driver decides which physical/device pages are exposed to user space and with which memory attributes. MMIO registers, coherent DMA buffers, and cached normal memory have different access and ordering rules. `mmap()` creates the user VMA, but correctness depends on driver-selected cacheability, barriers, hardware ownership bits, interrupt/DMA state, and lifetime rules.

**Pitfalls**

Do not guess physical addresses from user space. Do not assume a normal load/store is enough for a device protocol. Do not ignore page-based offsets, driver-specific offset meanings, hot-unplug, revoked access, or firmware changes. Do not keep using a pointer after `munmap()` or after the driver says the buffer is no longer owned by user space.

**Debug angle**

Trace `open`, `ioctl`, `mmap`, and `munmap` with `strace`. Inspect `/proc/<PID>/maps` to confirm the VMA and permissions. Check `dmesg`, driver logs, `/sys/class`, `/sys/bus`, device-tree/platform data, and the driver documentation. If the symptom is stale DMA data, check cache-coherency guarantees, synchronization/ownership handoff, and whether the target kernel/driver changed memory attributes.

**Follow-up keywords**

device-backed `mmap()`, MMIO, DMA buffer, cache coherency, barriers, driver ABI, `ioctl()`, `/dev/mem`, UIO, framebuffer, `dmesg`, `/sys`, `SIGBUS`, `EIO`.

---

## Short Answers - Priority B

### 10. Compare stack, heap, and memory mappings.

Stack memory is automatic and tied to function calls. Heap memory is explicit and tied to ownership through the malloc family. Memory mappings are VM regions created by `mmap()` and released with `munmap()`. Stack bugs are often stack overflow or returning a pointer to a local. Heap bugs are leaks, double-free, use-after-free, and overflows. Mapping bugs are wrong flags, bad failure checks, missing `munmap()`, `SIGBUS`, and incorrect synchronization.

### 11. Explain text, data, BSS, heap, stack, mmap region, and kernel space.

Text stores executable code. Data stores initialized globals/statics. BSS stores zero-initialized or uninitialized globals/statics. Heap stores dynamic allocations. The mmap region contains shared libraries, mapped files, anonymous mappings, and shared memory. Stack stores call frames and automatic variables. Kernel space is protected from direct user access and is entered through system calls or exceptions.

### 12. Why can `free()` fail to reduce RSS immediately?

`free()` returns a block to the allocator, not necessarily to the kernel. The allocator may reuse the block later, the block may sit in the middle of the heap, and avoiding frequent `brk()` or `munmap()` calls improves performance. Large allocator-managed mappings may be returned more directly, but this is implementation and size dependent.

### 13. How should `realloc()` be used safely in a growable buffer?

Assign `realloc()` to a temporary pointer first. If it fails, the original block remains valid. Also check multiplication overflow before computing byte sizes, and remember that successful `realloc()` may move the block, invalidating interior pointers.

### 14. When is `calloc()` better than `malloc()`?

Use `calloc()` when allocating an array that should start zero-filled. It also detects overflow in `n * size`, which a manual `malloc(n * size)` can miss.

### 15. What are `brk()` and `sbrk()`, and why should normal application code avoid them?

They adjust the program break, the traditional end of the heap. They are useful for understanding allocator history, but normal code should use the malloc family because it is portable, allocator-aware, and safer in threaded programs. Manually mixing `sbrk()` with the process allocator can violate allocator assumptions.

### 16. Compare `MAP_PRIVATE` and `MAP_SHARED`.

`MAP_PRIVATE` gives private copy-on-write changes; writes are not visible to other processes and do not update the file. `MAP_SHARED` exposes writes through shared mappings and can propagate file-backed changes to the underlying file. `MAP_SHARED` still requires synchronization.

### 17. Compare file-backed and anonymous mappings.

File-backed mappings get contents from a file or device and can be used for mapped I/O, executable/shared-library loading, device memory, or persistent sharing. Anonymous mappings have no file and start zero-filled; they are used for private memory, allocator internals, guard-page regions, or parent-child shared memory.

### 18. What causes `SIGSEGV` versus `SIGBUS`?

`SIGSEGV` usually means an unmapped address or a protection violation, such as writing to a read-only mapping or touching a `PROT_NONE` guard page. `SIGBUS` commonly means the virtual address is inside a file-backed mapping, but the backing file cannot provide the page.

### 19. How do mappings behave across `fork()`, `exec()`, and process exit?

Mappings are inherited across `fork()` with the same mapping type. Private mappings then use copy-on-write; shared mappings stay shared. A successful `exec()` removes the old mappings. Process exit removes mappings automatically.

### 20. What does `msync()` guarantee, and what does it not guarantee?

`msync()` gives explicit control over synchronizing dirty pages of a shared file mapping. `MS_SYNC` waits for writeback; `MS_ASYNC` schedules it. `msync()` is not a complete crash-consistency or transaction protocol; designs may still need ordering, metadata handling, `fsync()`, checksums, journals, or versioning.

### 21. A process has stable RSS but increasing latency due to major page faults. How do `mincore()`, RSS, and `VmLck` help, and what would you change?

Stable RSS does not mean stable latency. I would measure minor faults, major faults, mapped-file access patterns, storage latency, cgroup pressure, and whether the hot path touches cold pages that were reclaimed or never prefaulted. `mincore()` asks whether pages in a specific mapped range are currently resident, but the answer can become stale immediately unless the pages are locked. RSS summarizes process-level resident memory and does not prove that a particular page will avoid a fault later. `VmLck` shows memory locked with `mlock()` or `mlockall()`, which is stronger evidence for a small no-swap/no-major-fault working set. Fixes include improving locality, pre-touching critical pages, using `madvise(MADV_WILLNEED)` or `MADV_SEQUENTIAL` after measurement, locking only a small realtime range, reducing memory pressure, or switching away from `mmap()` if page-fault latency is unacceptable.

### 22. How can core dumps help debugging and also leak secrets?

A core dump captures process memory at crash time, which helps inspect stack, heap, mappings, and fault addresses in `gdb` or `coredumpctl`. The same captured memory may include tokens, keys, passwords, or user data. Production systems should configure core limits, systemd-coredump policy, dumpability, and `coredump_filter`; sensitive mappings may use `MADV_DONTDUMP` where appropriate.

---

## Recognition Notes - Priority C

- `alloca()` allocates on the stack and is released automatically on function return. It is risky for large or untrusted sizes because stack overflow does not give a clean allocation failure path.
- `mallopt()`, `malloc_trim()`, `malloc_info()`, and related allocator controls are implementation-specific tuning/debugging tools. Use them only after measurement.
- `MAP_FIXED` requests an exact address and can replace existing mappings. It is for runtimes, loaders, allocators, or unusual mapping layouts, not ordinary application code.
- `MAP_POPULATE` prefaults or prepares pages earlier. It can reduce later faults in measured cases but increases upfront work.
- `MAP_NORESERVE` changes swap reservation and can increase later OOM risk when pages are touched.
- `MAP_LOCKED` resembles memory locking but explicit `mlock()` gives clearer post-call expectations.
- `MAP_HUGETLB` and huge pages can reduce TLB pressure for special workloads but add operational complexity.
- `MAP_UNINITIALIZED` is Linux/embedded-specific, security-sensitive, and only honored on kernels configured for it.
- `mremap()` resizes or moves mappings on Linux; old pointers can become invalid, so offset-based references are safer.
- `mincore()` reports current page residency, but the information can become stale immediately unless pages are locked.
- Deep OOM scoring is operationally useful, but interview depth should start with overcommit, cgroups, RSS growth, and OOM logs.

---

## Extra Questions Worth Adding

1. A leak appears in production but not under Valgrind. What timing, workload, suppression, allocator, and target/host differences would you investigate?
2. A service uses `mlockall(MCL_FUTURE)` and later `malloc()` starts failing. How do you explain and fix it?
3. A shared mapping stores raw pointers and works in one process pair but fails after restart. Why are offsets usually safer?
4. A core dump from a customer target is missing expected mappings. Which limits and dump filters would you inspect?

---

## One-Minute Review

- Linux process memory is virtual; physical RAM is attached lazily through page faults.
- Learn the layout: text, data, BSS, heap, mmap region, stack, and kernel space.
- VmSize/VIRT is address space; RSS is resident RAM.
- Stack lifetime is automatic; heap and mappings need explicit ownership and cleanup.
- `malloc()` failure is `NULL`; `mmap()` failure is `MAP_FAILED`.
- `free()` returns memory to the allocator, not necessarily to the OS immediately.
- Use a temporary pointer for `realloc()` and check allocation-size overflow.
- `mmap()` is best for mapped files, shared memory, and page-level control, not as a universal faster I/O API.
- `MAP_PRIVATE` means copy-on-write; `MAP_SHARED` means visible shared writes.
- Shared memory still needs synchronization.
- `SIGSEGV` usually means invalid or prohibited access; `SIGBUS` often means missing file backing.
- Use `/proc/<PID>/maps`, `smaps`, `status`, `pmap`, `top`, `free`, `vmstat`, `dmesg`, `coredumpctl`, `gdb`, `strace`, Valgrind, ASan, LSan, and `perf` according to the symptom.
- Embedded constraints change the design: limited RAM, no swap, stripped binaries, target/host library mismatch, flash storage, and tighter latency budgets.
