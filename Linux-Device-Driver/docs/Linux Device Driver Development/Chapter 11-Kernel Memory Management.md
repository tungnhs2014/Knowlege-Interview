```bash
# Chapter 11 - Kernel Memory Management
```
On Linux systems, every memory address is virtual. They do not point to any address in the RAM directly. Whenever you access a memory location, a translation mechanism is performed in order to match the corresponding physical memory.
Let's start with a short story to introduce the virtual memory concept. Given a hotel, there can be a phone in each room, each with a private number. Any installed phone, of course,
belongs to the hotel. None of them can be connected directly from outside the hotel.
If you need to contact an occupant of a room, let's say your friend, he must give you the hotel's switchboard number and the room number in which he is staying. Once you call the switchboard and give the room number of the occupant you need to talk to, the receptionist redirects your call to the private phone in the room. Only the receptionist and the room occupant know the private number mapping:
(switchboard number + room number) <=> private (real) phone number
Every time someone in the city (or anywhere in the world) wants to contact a room occupant, he has to pass through the hotline. He needs to know the right hotline number of the hotel and the room number. This way, switchboard number + room number is the virtual address, whereas private phone number corresponds to the physical address.
There are some rules related to hotels that apply on Linux as well:
Hotel Linux
You cannot contact an occupant who has no private phone in the room. There is not even a way to attempt to do this. Your call will be ended suddenly.
You cannot access nonexistent memory in your address space. This will cause a segmentation fault.
You cannot contact an occupant who does not exist, or whose check-in the hotel is not aware of, or whose information is not found by the switchboard.
If you access unmapped memory, the CPU
raises a page fault and the OS handles it.
You can't contact an occupant whose stay is over.
You cannot access freed memory. Maybe it has been allocated to another process.
Many hotels may have the same brand, but located at different places, each of them with a different hotline number.
Different processes may have the same virtual addresses mapped in their address space, but pointing to different physical addresses.
```c
There is a book (or software with a database)
```
holding the mapping between the room number and the private phone number, and consulted by the receptionist on demand.
Virtual addresses are mapped to the physical memory by page tables, which are maintained by the operating system kernel and consulted by the processor.
That is how you can imagine virtual addresses working on a Linux system.
In this chapter, we will deal with the whole Linux memory management system, covering the following topics:
Memory layout along with address translation and MMU
Memory allocation mechanisms (page allocator, slab allocator, kmalloc allocator,
and so on)
I/O memory access
Mapping kernel memory to user space and implementing the mmap() callback function
Introducing the Linux caching system
```c
Introducing the device managed resource framework (devres)
```
System memory layout – kernel space and user space
Throughout this chapter, terms such as kernel space and user space will refer to their virtual address space. On Linux systems, each process owns a virtual address space. It is a kind of memory sandbox during the life of the process. That address space is 4 GB in size on 32-bit systems (even on a system with physical memory less than 4 GB). For each process, that 4 GB address space is split into two parts:
User space virtual addresses
Kernel space virtual addresses
The way the split is done depends on a special kernel configuration option,
CONFIG_PAGE_OFFSET, which defines where the kernel addresses section starts in a process address space. The common value is 0xC0000000 by default on 32-bit systems, but this may be changed, as is the case for i.MX6 family processors from NXP, which use 0x80000000. In the whole chapter, we will consider 0xC0000000 by default. This is called a 3G/1G split, where the user space is given the lower 3 GB of virtual address space,
and the kernel uses the upper remaining 1 GB. A typical process's virtual address space layout looks like the following:
.------------------------. 0xFFFFFFFF
| | (4 GB)
| Kernel addresses |
| |
| |
.------------------------.CONFIG_PAGE_OFFSET
| |(x86: 0xC0000000, ARM: 0x80000000)
| |
| |
| User space addresses |
| |
| |
| |
| |
'------------------------' 00000000
Both addresses used in the kernel and the user space are virtual addresses. The difference is that accessing a kernel address needs privileged mode. Privileged mode has extended privileges. When the CPU runs the user space-side code, the active process is said to be running in user mode; when the CPU runs the kernel space-side code, the active process is said to be running in kernel mode.
Given an address (virtual of course), you can distinguish whether it is a kernel space or a user space address by using the process layout shown previously. Every address falling into 0-3 GB comes from the user space;
otherwise, it is from the kernel.
There is a reason why the kernel shares its address space with every process: because every single process at a given moment uses system calls, which will involve the kernel. Mapping the kernel's virtual memory address into each process's virtual address space allows us to avoid the cost of switching out the memory address space on each entry to (and exit from)
the kernel. This is the reason why the kernel address space is permanently mapped on top of each process in order to speed up kernel access through system calls.
The memory management unit organizes memory into units of fixed size called pages. A
page consists of 4,096 bytes (4 KB). Even if this size may differ on other systems, it is fixed on ARM and x86, which are the architectures we are interested in:
Memory page, virtual page, or simply page are terms you use to refer to a fixedlength contiguous block of virtual memory. The same name, page, is used as a kernel data structure to represent a memory page.
On the other hand, a frame (or page frame) refers to a fixed-length contiguous block of physical memory on top of which the operating system maps a memory page. Each page frame is given a number, called a page frame number (PFN).
Given a page, you can easily get its PFN and vice versa, using the page_to_pfn and pfn_to_page macros, which will be discussed in detail in the next sections.
A page table is the kernel and architecture data structure used to store the mapping between virtual addresses and physical addresses. The key pair page/frame describes a single entry in the page table. This represents a mapping.
Since a memory page is mapped to a page frame, it goes without saying that pages and page frames have the same sizes, 4 K in our case. The size of a page is defined in the kernel through the PAGE_SIZE macro.
There are situations where you need memory to be page-aligned. You say a memory is page-aligned if its address starts exactly at the beginning of a page. For example, on a 4 K page size system, 4,096, 20,480, and 409,600
are instances of page-aligned memory addresses. In other words, any memory whose address is a multiple of the system page size is said to be page-aligned.
Kernel addresses – concept of low and high memory
The Linux kernel has its own virtual address space, as every user mode process does. The virtual address space of the kernel (1 GB sized in a 3G/1G split) is divided into two parts:
Low memory or LOWMEM, which is the first 896 MB
High memory or HIGHMEM, represented by the top 128 MB
Physical mem
```c
Process address space +------> +------------+
```
| | 3200 M |
| | |
4 GB +---------------+ <-----+ | HIGH MEM |
| 128 MB | | |
+---------------+ <---------+ | |
+---------------+ <------+ | | |
```c
| 896 MB | | +--> +------------+
3 GB +---------------+ <--+ +-----> +------------+
```
| | | | 896 MB | LOW MEM
```c
| ///// | +---------> +------------+
```
| |
0 GB +---------------+
## Low memory
The first 896 MB of kernel address space constitutes the low memory region. Early in the boot, the kernel permanently maps this 896 MB. Addresses that result from that mapping are called logical addresses. These are virtual addresses, but can be translated into physical addresses by subtracting a fixed offset, since the mapping is permanent and known in advance. Low memory matches with the lower bound of physical addresses. You could define low memory as being the memory for which logical addresses exist in the kernel space. Most of the kernel memory function returns low memory. In fact, to serve different purposes, kernel memory is divided into a zone. Actually, the first 16 MB of LOWMEM is reserved for DMA use. Because of hardware limitations, the kernel cannot treat all pages as identical. We can then identify three different memory zones in the kernel space:
ZONE_DMA: This contains page frames of memory below 16 MB, reserved for
```c
Direct Memory Access (DMA)
```
ZONE_NORMAL: This contains page frames of memory above 16 MB and below 896
MB, for normal use
ZONE_HIGHMEM: This contains page frames of memory at and above 896 MB
That says on a 512 MB system, there will be no ZONE_HIGHMEM, 16 MB for ZONE_DMA, and
496 MB for ZONE_NORMAL.
Another definition of logical addresses is addresses in kernel space,
mapped linearly on physical addresses, which can be converted into physical addresses just with an offset or by applying a bitmask. You can convert a physical address into a logical address using the
__pa(address) macro, and then revert it with the __va(address)
macro.
## High memory
The top 128 MB of the kernel address space is called the high memory region. It is used by the kernel to temporarily map physical memory above 1 GB. When physical memory above
1 GB (or more precisely, 896 MB) needs to be accessed, the kernel uses those 128 MB to create a temporary mapping to its virtual address space, thus achieving the goal of being able to access all physical pages. You could define high memory as being memory for which logical addresses do not exist, and which is not mapped permanently into the kernel address space. The physical memory above 896 MB is mapped on demand to the 128 MB of the HIGHMEM region.
Mapping to access high memory is created on the fly by the kernel, and destroyed when done. This makes high memory access slower. That said, the concept of high memory does not exist on 64-bit systems, due to the huge address range (264), where the 3G/1G split does not make sense anymore.
## User space addresses
In this section, we will deal with the user space by means of processes. Each process is represented in the kernel as an instance of struct task_struct (see include/linux/sched.h), which characterizes and describes a process. Each process is given a table of memory mapping, stored in a variable of type struct mm_struct (see include/linux/mm_types.h). You can then guess that there is at least one mm_struct field embedded in each task_struct. The following line is the part of the struct task_struct definition that we are interested in:
```c
struct task_struct{
```
[...]
```c
struct mm_struct *mm, *active_mm;
```
[...]
```c
}
The kernel global variable current points to the current process. The *mm field points to its memory mapping table. By definition, current->mm points to the current process memory mappings table.
```
Now, let's see what a struct mm_struct looks like:
```c
struct mm_struct {
struct vm_area_struct *mmap;
struct rb_root mm_rb;
unsigned long mmap_base;
unsigned long task_size;
unsigned long highest_vm_end;
pgd_t * pgd;
atomic_t mm_users;
atomic_t mm_count;
```
atomic_long_t nr_ptes;
#if CONFIG_PGTABLE_LEVELS > 2
atomic_long_t nr_pmds;
#endif int map_count;
```c
spinlock_t page_table_lock;
struct rw_semaphore mmap_sem;
unsigned long hiwater_rss;
unsigned long hiwater_vm;
unsigned long total_vm;
unsigned long locked_vm;
unsigned long pinned_vm;
unsigned long data_vm;
unsigned long exec_vm;
unsigned long stack_vm;
unsigned long def_flags;
unsigned long start_code, end_code, start_data, end_data;
unsigned long start_brk, brk, start_stack;
unsigned long arg_start, arg_end, env_start, env_end;
```
/* Architecture-specific MM context */
mm_context_t context;
```c
unsigned long flags;
struct core_state *core_state;
```
#ifdef CONFIG_MEMCG
/*
* "owner" points to a task that is regarded as the canonical
* user/owner of this mm. All of the following must be true in
* order for it to be changed:
*
```c
* current == mm->owner
* current->mm != mm
* new_owner->mm == mm
* new_owner->alloc_lock is held
```
*/
```c
struct task_struct __rcu *owner;
```
#endif struct user_namespace *user_ns;
/* store ref to file /proc/<pid>/exe symlink points to */
```c
struct file __rcu *exe_file;
};
```
I intentionally removed some fields we are not interested in. There are some fields we will talk about later: pgd for example, which is a pointer to the process's base (first entry) level 1
table (PGD), written in the translation table base address of the CPU at context switching.
Anyway, before going further, let's see a representation of a process address space:
## Process memory layout
From the process point of view, a memory mapping can be seen as nothing but a set of page table entries dedicated to a consecutive virtual address range. That consecutive virtual address range is called the memory area, or virtual memory area (VMA). Each memory mapping is described by a start address and length, permissions (such as whether the program can read, write, or execute from that memory), and associated resources (such as physical pages, swap pages, file contents, and so on).
A mm_struct has two ways to store process regions (VMA):
```c
1. In a red-black tree, whose root element is pointed by the mm_struct->mm_rb field
2. In a linked list, where the first element is pointed by the mm_struct->mmap field
```
## Virtual memory area (VMA)
The kernel uses virtual memory areas to keep track of the process's memory mappings; for example, a process has one VMA for its code, one VMA for each type of data, one VMA for each distinct memory mapping (if any), and so on. VMAs are processor-independent structures, with permissions and access control flags. Each VMA has a start address, a length, and their sizes are always a multiple of the page size (PAGE_SIZE). A VMA consists of a number of pages, each of which has an entry in the page table.
Memory regions described by VMA are always virtually contiguous, not physically. You can check all VMAs associated with a process through the
/proc/<pid>/maps file, or using the pmap command on a process ID.
Image source: http://duartes.org/gustavo/blog/post/how-the-kernel-manages-your-memory/
```bash
# cat /proc/1073/maps
```
00400000-00403000 r-xp 00000000 b3:04 6438 /usr/sbin/net-listener
00602000-00603000 rw-p 00002000 b3:04 6438 /usr/sbin/net-listener
00603000-00624000 rw-p 00000000 00:00 0 [heap]
7f0eebe4d000-7f0eebe54000 r-xp 00000000 b3:04 11717
/usr/lib/libffi.so.6.0.4
7f0eebe54000-7f0eec054000 ---p 00007000 b3:04 11717
/usr/lib/libffi.so.6.0.4
7f0eec054000-7f0eec055000 rw-p 00007000 b3:04 11717
/usr/lib/libffi.so.6.0.4
7f0eec055000-7f0eec069000 r-xp 00000000 b3:04 21629 /lib/libresolv-2.22.so
7f0eec069000-7f0eec268000 ---p 00014000 b3:04 21629 /lib/libresolv-2.22.so
[...]
7f0eee1e7000-7f0eee1e8000 rw-s 00000000 00:12 12532 /dev/shm/sem.thkmcp-231016-sema
[...]
Each line in the preceding excerpt represents a VMA, and the fields map the following pattern: {address (start-end)} {permissions} {offset} {device
(major:minor)} {inode} {pathname (image)}:
address: This represents the starting and ending address of the VMA.
permissions: This describes the access rights of the region: r (read), w (write),
and x (execute), including p (if the mapping is private) and s (for shared mapping).
Offset: In the case of file mapping (mmap system call), it is the offset in the file where the mapping takes place. It is 0 otherwise.
major:minor: In the case of file mapping, these represent the major and minor number of the devices in which the file is stored (device holding the file).
inode: In the case of mapping from a file, the inode number of the mapped file.
pathname: This is the name of the mapped file, or left blank otherwise. There are other region name such as [heap], [stack], or [vdso], which stands for virtual dynamic shared object, which is a shared library mapped by the kernel into every process address space, in order to reduce performance penalties when system calls switch to kernel mode.
Each page allocated to a process belongs to an area; thus, any page that does not live in the
VMA does not exist and cannot be referenced by the process.
High memory is perfect for the user space because the user space's virtual address must be explicitly mapped. Thus, most high memory is consumed by user applications. __GFP_HIGHMEM and GFP_HIGHUSER are the flags for requesting the allocation of (potentially) high memory. Without these flags, all kernel allocations return only low memory. There is no way to allocate contiguous physical memory from the user space in Linux.
You can use the find_vma function to find the VMA that corresponds to a given virtual address. find_vma is declared in linux/mm.h:
* Look up the first VMA which satisfies addr < vm_end, NULL if none. */
```c
extern struct vm_area_struct * find_vma(struct mm_struct * mm, unsigned long addr);
```
This is an example:
```c
struct vm_area_struct *vma = find_vma(task->mm, 0x13000);
if (vma == NULL) /* Not found ? */
return -EFAULT;
if (0x13000 >= vma->vm_end) /* Beyond the end of returned VMA ? */
return -EFAULT;
```
The whole process of memory mapping can be obtained by reading these files:
/proc/<PID>/map, /proc/<PID>/smap, and /proc/<PID>/pagemap.
## Address translation and MMU
Virtual memory is a concept, an illusion given to a process so it thinks it has large and almost infinite memory, and sometimes more than the system really has. It is up to the CPU
to make the conversion from a virtual to a physical address every time you access a memory location. That mechanism is called address translation, and is performed by the
Memory Management Unit (MMU), which is a part of the CPU.
MMU protects memory from unauthorized access. Given a process, any page that needs to be accessed must exist in one of the process VMAs, and thus must live in the process page table (every process has its own).
Memory is organized by chunks of fixed-size named pages for virtual memory and frames for physical memory, sized 4 KB in our case. Anyway, you do not need to guess the page size of the system you write the driver for. It is defined and accessible with the PAGE_SIZE
macro in the kernel. Remember, therefore, page size is imposed by the hardware (CPU).
Considering a 4 KB page size system, bytes 0 to 4095 fall in Page 0, bytes 4096-8191 fall in
Page 1, and so on.
The concept of the page table is introduced to manage mapping between pages and frames.
Pages are spread over tables, so that each PTE corresponds to a mapping between a page and a frame. Each process is then given a set of page tables to describe its whole memory space.
In order to walk through pages, each page is assigned an index (like an array), called the page number. When it comes to a frame, it is PFN. This way, virtual memory addresses are composed of two parts: a page number and an offset. The offset represents the 12 less significant bits of the address, whereas 13 less significant bits represent it on 8 KB page size systems:
Virtual address representation
How do the OS or CPU know which physical address corresponds to a given virtual address? They use the page table as the translation table, and know that each entry's index is a virtual page number, and the value is the PFN. To access physical memory given a virtual memory, the OS first extracts the offset, the virtual page number, and then walks through the process's page tables in order to match virtual page number to physical page.
Once a match occurs, it is then possible to access data in that page frame:
Address translation
The offset is used to point to the right location into the frame. The page table not only holds mapping between physical and virtual page numbers, but also access control information
(read/write access, privileges, and so on):
Virtual to physical address translation
The number of bits used to represent the offset is defined by the kernel macro PAGE_SHIFT.
PAGE_SHIFT is the number of bits to shift one bit left to obtain the PAGE_SIZE value. It is also the number of bits to right-shift to convert the virtual address to the page number and the physical address to the PFN. The following are the definitions of these macros from
/include/asm-generic/page.h in the kernel sources:
```c
#define PAGE_SHIFT 12
```
#ifdef __ASSEMBLY__
```c
#define PAGE_SIZE (1 << PAGE_SHIFT)
```
#else
```c
#define PAGE_SIZE (1UL << PAGE_SHIFT)
```
#endif
A page table is a partial solution. Let's see why. Most architecture requires 32 bits (4 bytes)
to represent a PTE. Each process has its private 3 GB user space address, so we need 786,432
entries to characterize and cover a process address space. It represents too much physical memory spent per process, just to characterize the memory mappings. In fact, a process generally uses a small but scattered portion of its virtual address space. To resolve that issue, the concept of level is introduced. Page tables are hierarchized by level (page level).
The space necessary to store a multilevel page table only depends on the virtual address space actually in use, instead of being proportional to the maximum size of the virtual address space. This way, unused memory is no longer represented, and the page table walkthrough time is reduced. This way, each table entry in level N will point to an entry in table of level N+1. Level 1 is the higher level.
Linux uses a four-level paging model:
Page Global Directory (PGD): It is the first-level (level 1) page table. Each entry's type is pgd_t in the kernel (generally an unsigned long), and points to an entry in the table at the second level. In the kernel, the tastk_struct structure represents a process's description, which in turn has a member (mm) whose type is mm_struct, and that characterizes and represents the process's memory space. In mm_struct, there is a processor-specific field, pgd,
which is a pointer to the first entry (entry 0) of the process's level-1 (PGD) page table. Each process has one and only one PGD, which may contain up to 1,024
entries.
Page Upper Directory (PUD): This exists only on architectures using four-level tables. It represents the second level of indirection.
Page Middle Directory (PMD): This is the third indirection level, and exists only on architectures using four-level tables.
Page Table (PTE): Leaves of the tree. It is an array of pte_t, where each entry points to the physical page.
Not all levels are always used. The i.MX6's MMU only supports a twolevel page table (PGD and PTE), which is the case for almost all 32-bit
CPUs) In this case, PUD and PMD are simply ignored.
Two-level tables overview
You might ask how MMU is aware of the process page table. It is simple, MMU does not store any addresses. Instead, there is a special register in the CPU, called page table base register (PTBR) or Translation Table Base Register 0 (TTBR0), which points to the base
```c
(entry 0) of the level-1 (top-level) page table (PGD) of the process. It is exactly where the pdg field of struct mm_struct points: current->mm.pgd == TTBR0.
```
At context switch (when a new process is scheduled and given the CPU), the kernel immediately configures the MMU and updates the PTBR with the new process's pgd. Now when a virtual address is given to MMU, it uses the PTBR's content to locate the process's level-1 page table (PGD), and then it uses the level-1 index, extracted from the most significant bits (MSBs) of the virtual address, to find the appropriate table entry, which contains a pointer to the base address of the appropriate level-2 page table. Then, from that base address, it uses the level-2 index to find the appropriate entry and so on until it reaches the PTE. ARM architecture (i.MX6 in our case) has a two-level page table. In this case, the level-2 entry is a PTE, and points to the physical page (PFN). Only the physical page is found at this step. To access the exact memory location in the page, the MMU
extracts the memory offset, also part of the virtual address, and points to the same offset in the physical page.
When a process needs to read from or write into a memory location (of course, we're talking about virtual memory), the MMU performs a translation into that process's page table to find the right entry (PTE). The virtual page number is extracted from the virtual address and used by the processor as an index into the process's page table to retrieve its page table entry. If there is a valid page table entry at that offset, the processor takes the
PFN from this entry. If not, it means the process accessed an unmapped area of its virtual memory. A page fault is then raised and the OS should handle it.
In the real world, address translation requires a page table walk, and it is not always a oneshot operation. There are at least as many instances of memory accesses as there are table levels. A four-level page table would require four memory accesses. In other words,
every instance of virtual access would result in five physical memory accesses. The virtual memory concept would be useless if its access were four times slower than physical access.
Fortunately, SoC manufacturers worked hard to find a clever trick to address this performance issue: modern CPUs use a small associative and very fast memory called translation lookaside buffer (TLB), in order to cache the PTEs of recently accessed virtual pages.
## Page lookup and TLB
Before the MMU proceeds to address translation, there is another step involved. As there is a cache for recently accessed data, there is also a cache for recently translated addresses. As a data cache speeds up the data access process, TLB speeds up virtual address translation.
Yes, address translation is a tricky task. It is content-addressable memory (CAM), where the key is the virtual address and the value is the physical address. In other words, the TLB
is a cache for the MMU. At each memory access, the MMU first checks for recently used pages in the TLB, which contains a few of the virtual address ranges to which physical pages are currently assigned.
## How does the TLB work?
On virtual memory access, the CPU walks through the TLB, trying to find the virtual page number of the page that is being accessed. This step is called TLB lookup. When a TLB
entry is found (a match occurs), you say there is a TLB hit and the CPU just keeps running and uses the PFN found in the TLB entry to calculate the target physical address. There is no page fault when a TLB hit occurs. As you can see, as long as a translation can be found in the TLB, virtual memory access will be as fast as physical access. If no TLB entry is found
(no match occurs), you say there is a TLB miss.
On a TLB miss event, there are two possibilities, depending on the processor type; TLB miss events can be handled by the software, or by the hardware, through the MMU:
Software handling: The CPU raises a TLB miss interruption, caught by the OS.
The OS then walks through the process's page table to find the right PTE. If there is a matching and valid entry, then the CPU installs the new translation in the
TLB. Otherwise, the page fault handler is executed.
Hardware handling: It is up to the CPU (the MMU in fact) to walk through the process's page table in hardware. If there is a matching and valid entry, the CPU
adds the new translation in the TLB. Otherwise, the CPU raises a page fault interruption, handled by the OS.
In both cases, the page fault handler is the same: the do_page_fault() function is executed, which is architecture-dependent. For ARM, the do_page_fault is defined in arch/arm/mm/fault.c:
MMU and TLB walkthrough process
Page table and page directory entries are architecture-dependent. It is up to the OS to ensure that the structure of the table corresponds to a structure recognized by the MMU. On the ARM processor, you must write the location of the translation table in CP15 (coprocessor 15) register c2,
and then enable the caches and the MMU by writing to the CP15 register c1. Have a look at both http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0
056d/BABHJIBH.htm and http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0
433c/CIHFDBEJ.html for detailed information.
## Memory allocation mechanism
Let's look at the following diagram, showing us the different memory allocators that exist on a Linux-based system, and discuss it later. (inspired by http://free-electrons.
com/doc/training/linux-kernel/linux-kernel-slides.pdf):
Overview of kernel memory allocator
There is an allocation mechanism to satisfy any kind of memory request. Depending on what you need memory for, you can choose the one closest to your goal. The main allocator is the page allocator, which only works with pages (a page being the smallest memory unit it can deliver). Then comes the SLAB allocator which is built on top of the page allocator,
getting pages from it and returning smaller memory entities (by mean of slabs and caches).
This is the allocator on which the kmalloc allocator relies.
## Page allocator
The page allocator is the lowest level allocator on the Linux system, the one on which other allocators rely. The system's physical memory is made up of fixed-size blocks (called page frames). A page frame is represented in the kernel as an instance of the struct page structure. A page is the smallest unit of memory that the OS will give to any memory request at a low level.
## Page allocation API
You will have understood that the kernel page allocator allocates and deallocates blocks of pages using the buddy algorithm. Pages are allocated in blocks that are powers of 2 in size
(in order to get the best from the buddy algorithm). That means that it can allocate a block 1
page, 2 pages, 4 pages, 8, 16, and so on:
1. alloc_pages(mask, order) allocates 2 order pages and returns an instance of struct page which represents the first page of the reserved block. To allocate only one page, the order should be 0. This is what alloc_page(mask) does:
```c
struct page *alloc_pages(gfp_t mask, unsigned int order)
#define alloc_page(gfp_mask) alloc_pages(gfp_mask, 0)
```
__free_pages() is used to free memory allocated with the alloc_pages()
function. It takes a pointer to the allocated page(s) as a parameter, with the same order as was used for allocation:
```c
void __free_pages(struct page *page, unsigned int order);
```
2. There are other functions working in the same way, but instead of an instance of struct page, they return the address (virtual of course) of the reserved block.
These are __get_free_pages(mask, order) and __get_free_page(mask):
```c
unsigned long __get_free_pages(gfp_t mask, unsigned int order);
unsigned long get_zeroed_page(gfp_t mask);
free_pages() is used to free pages allocated with __get_free_pages(). It takes the kernel address representing the start region of the allocated page(s),
```
along with the order, which should be the same as that used for allocation:
```c
free_pages(unsigned long addr, unsigned int order);
```
In either case, mask specifies details about the request, which are the memory zones and the behavior of allocators. Choices available are:
GFP_USER: For user memory allocation.
GFP_KERNEL: The commonly used flag for kernel allocation.
GFP_HIGHMEM: Requests memory from the HIGH_MEM zone.
GFP_ATOMIC: Allocates memory in an atomic manner that cannot sleep. Used when you need to allocate memory from an interrupt context.
There is a warning on using GFP_HIGHMEM, which should not be used with
__get_free_pages() (or __get_free_page()). Since HIGHMEM memory is not guaranteed to be contiguous, you can't return an address for memory allocated from that zone. Globally, only a subset of GFP_* is allowed in memory-related functions:
```c
unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
struct page *page;
```
/*
* __get_free_pages() returns a 32-bit address, which cannot represent
* a highmem page
*/
VM_BUG_ON((gfp_mask & __GFP_HIGHMEM) != 0);
page = alloc_pages(gfp_mask, order);
```c
if (!page)
return 0;
return (unsigned long) page_address(page);
}
```
The maximum number of pages you can allocate is 1,024. It means that on a 4 KB sized system, you can allocate up to 1,024*4 KB = 4 MB at most. It is the same for kmalloc.
## Conversion functions
The page_to_virt() function is used to convert the struct page (as returned by alloc_pages(), for example) into the kernel address. virt_to_page() takes a kernel virtual address and returns its associated struct page instance (as if it was allocated using the alloc_pages() function). Both virt_to_page() and page_to_virt() are defined in <asm/page.h>:
```c
struct page *virt_to_page(void *kaddr);
void *page_to_virt(struct page *pg)
```
The page_address() macro can be used to return the virtual address that corresponds to the beginning address (the logical address of course) of a struct page instance:
```c
void *page_address(const struct page *page)
```
We can see how it is used in the get_zeroed_page() function:
```c
unsigned long get_zeroed_page(unsigned int gfp_mask)
{
struct page * page;
```
page = alloc_pages(gfp_mask, 0);
```c
if (page) {
void *address = page_address(page);
clear_page(address);
return (unsigned long) address;
}
return 0;
}
```
__free_pages() and free_pages() can be mixed. The main difference between them is that free_page() takes a virtual address as a parameter, whereas __free_page() takes a struct page structure.
## Slab allocator
The slab allocator is the one on which kmalloc() relies. Its main purpose is to eliminate the fragmentation caused by memory (de)allocation that would be caused by the buddy system in the case of small-size memory allocation, and speed up memory allocation for commonly used objects.
## The buddy algorithm
To allocate memory, the requested size is rounded up to a power of two, and the buddy allocator searches the appropriate list. If no entries exist on the requested list, an entry from the next upper list (which has blocks of twice the size of the previous list) is split into two halves (called buddies). The allocator uses the first half, while the other is added to the next list down. This is a recursive approach, which stops when either the buddy allocator successfully finds a block that we can be split, or reaches the largest size of block and there are no free blocks available.
The following case study is heavily inspired by http://dysphoria.net/OperatingSystems1/4_allocation_buddy_system.html. As an example, if the minimum allocation size is 1 KB, and the memory size is 1 MB, the buddy allocator will create an empty list for 1 KB holes, empty list for 2 KB holes, one for 4 KB
holes, 8 KB, 16 KB, 32 KB, 64 KB, 128 KB, 256 KB, 512 KB, and one list for 1 MB holes. All of them are initially empty, except for the 1 MB list, which has only one hole.
Now, let's imagine a scenario where we want to allocate a 70K block. The buddy allocator will round it up to 128K, and end up splitting the 1 MB into two 512K blocks, then 256K,
and finally 128K, then it will allocate one of the 128K blocks to the user. The following are schemes that summarize this scenario:
Allocation using buddy algorithm
Deallocation is as fast as allocation. The following diagram summarizes the deallocation algorithm:
Deallocation using buddy algorithm
## A journey into the slab allocator
Before we introduce the slab allocator, let's define some terms it uses:
Slab: This is a contiguous piece of physical memory made of several page frames. Each slab is divided into equal chunks of the same size, used to store specific types of kernel object, such as inodes, mutexes, and so on. Each slab is then an array of objects.
Cache: It is made of one or more slabs in a linked list, and they are represented in the kernel as instances of the struct kmem_cache_t structure. The cache only stores objects of the same type (for example, inodes only, or only address space structures).
Slabs may be in one of the following states:
Empty: This is where all objects (chunks) on the slab are marked as free
Partial: Both used and free objects exist in the slab
Full: All objects on the slab are marked as used
It is up to the memory allocator to build caches. Initially, each slab is marked as empty.
When your code allocates memory for a kernel object, the system looks for a free location for that object on a partial/free slab in a cache for that type of object. If not found, the system allocates a new slab and adds it into the cache. The new object gets allocated from this slab, and the slab is marked as partial. When the code is done with the memory
(memory freed), the object is simply returned to the slab cache in its initialized state.
This is the reason why the kernel also provides helper functions to obtain zeroed initialized memory, in order to get rid of the previous content. The slab keeps a reference count of how many of its objects are being used, so that when all slabs in a cache are full and another object is requested, the slab allocator is responsible for adding new slabs:
Slab cache overview
It is a bit like creating a per-object allocator. The system allocate, one cache per type of object, and only objects of the same type can be stored in a cache (for example, only the task_struct structure).
There are different kinds of slab allocator in the kernel, depending on whether or not you need compactness, cache-friendliness, or raw speed:
The SLOB, which is as compact as possible
The SLAB, which is as cache-friendly as possible
The SLUB, which is quite simple and requires fewer instruction cost counts
## kmalloc family allocation kmalloc is a kernel memory allocation function, such as malloc() in user space. Memory returned by kmalloc is contiguous in physical memory and in virtual memory:
The kmalloc allocator is the general and higher-level memory allocator in the kernel, which relies on the SLAB allocator. Memory returned from kmalloc has a kernel logical address because it is allocated from the LOW_MEM region, unless HIGH_MEM is specified. It is declared in <linux/slab.h>, which is the header to include when using kmalloc in your driver.
The following is the prototype:
```c
void *kmalloc(size_t size, int flags);
```
size specifies the size of the memory to be allocated (in bytes). flag determines how and where memory should be allocated. Available flags are the same as the page allocator
(GFP_KERNEL, GFP_ATOMIC, GFP_DMA, and so on):
GFP_KERNEL: This is the standard flag. We cannot use this flag in the interrupt handler because its code may sleep. It always returns memory from the LOM_MEM
zone (hence a logical address).
GFP_ATOMIC: This guarantees the atomicity of the allocation. The only flag to use when we are in the interrupt context. Please do not abuse this, since it uses an emergence pool of memory.
GFP_USER: This allocates memory to a user space process. Memory is then distinct and separated from that allocated to the kernel.
GFP_HIGHUSER: This allocates memory from the HIGH_MEMORY zone.
GFP_DMA: This allocates memory from DMA_ZONE.
On successful allocation of memory, kmalloc returns the virtual address of the chunk allocated, guaranteed to be physically contiguous. On error, it returns NULL.
```c
kmalloc relies on SLAB caches when allocating small-size memories. In this case, the kernel rounds the allocated area size up to the size of the smallest SLAB cache in which it can fit.
```
Always use it as your default memory allocator. In architectures used in this book (ARM
and x86), the maximum size per allocation is 4 MB, and 128 MB for total allocations. Have a look at https://kaiwantech.wordpress.com/2011/08/17/kmalloc-and-vmalloc-linux-kernel-me mory-allocation-api-limits/.
The kfree function is used to free the memory allocated by kmalloc. The following is the prototype of kfree():
```c
void kfree(const void *ptr)
```
Let's see an example:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu");
void *ptr;
static int alloc_init(void)
{
```
size_t size = 1024; /* allocate 1024 bytes */
ptr = kmalloc(size,GFP_KERNEL);
```c
if(!ptr) {
```
/* handle error */
```c
pr_err("memory allocation failed\n");
return -ENOMEM;
}
else pr_info("Memory allocated successfully\n");
return 0;
}
static void alloc_exit(void)
{
kfree(ptr);
pr_info("Memory freed\n");
}
module_init(alloc_init);
module_exit(alloc_exit);
```
Other family-like functions are:
```c
void kzalloc(size_t size, gfp_t flags);
void kzfree(const void *p);
void *kcalloc(size_t n, size_t size, gfp_t flags);
void *krealloc(const void *p, size_t new_size, gfp_t flags);
```
krealloc() is the kernel equivalent of the user space realloc() function. Because memory returned by kmalloc() retains the contents from its previous incarnation, there could be a security risk if it's exposed to the user space. To get zeroed kmalloc'ed memory,
you should use kzalloc. kzfree() is the freeing function for kzalloc(), whereas kcalloc() allocates memory for an array, and its parameters n and size represent,
respectively, the number of elements in the array and the size of an element.
Since kmalloc() returns a memory area in the kernel's permanent mapping (which mean physically contiguous), the memory address can be translated to a physical address using virt_to_phys(), or to an IO bus address using virt_to_bus(). These macros internally call either
__pa() or __va()if necessary. The physical address
(virt_to_phys(kmalloc'ed address)), downshifted by PAGE_SHIFT ,
will produce a PFN of the first page from which the chunk is allocated.
## vmalloc allocator vmalloc() is the last kernel allocator we will discuss in the book. It returns memory only contiguous in virtual space (not physically contiguous):
The returned memory always comes from the HIGH_MEM zone. Addresses returned cannot be translated into physical ones or into bus addresses, because you cannot assert that the memory is physically contiguous. It means memory returned by vmalloc() can't be used outside the microprocessor (you cannot easily use it for DMA purposes). It is correct to use vmalloc() to allocate memory for a large sequential (it does not make sense to use it to allocate one page, for example) that exists only in software, for example, in a network buffer. It is important to note that vmalloc() is slower than kmalloc() or page allocator functions, because it must retrieve the memory, build the page tables, or even remap into a virtually contiguous range, whereas kmalloc() never does that.
Before using the vmalloc API, you should include this header in the code:
```c
#include <linux/vmalloc.h>
```
The following are the vmalloc family prototypes:
```c
void *vmalloc(unsigned long size);
void *vzalloc(unsigned long size);
void vfree( void *addr);
```
size is the size of memory you need to allocate. Upon successful allocation of memory, it returns the address of the first byte of the allocated memory block. On failure, it returns a
NULL. The vfree function is used to free the memory allocated by vmalloc().
The following is an example of using vmalloc:
```c
#include<linux/init.h>
#include<linux/module.h>
#include <linux/vmalloc.h>
void *ptr;
static int my_vmalloc_init(void)
{
unsigned long size = 8192;
```
ptr = vmalloc(size);
```c
if(!ptr)
{
```
/* handle error */
```c
printk("memory allocation failed\n");
return -ENOMEM;
}
else pr_info("Memory allocated successfully\n");
return 0;
}
static void my_vmalloc_exit(void) /* function called at the time of rmmod
```
*/
```c
{
```
vfree(ptr); //free the allocated memory printk("Memory freed\n");
```c
}
module_init(my_vmalloc_init);
module_exit(my_vmalloc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("john Madieu, john.madieu@gmail.com");
```
You can use /proc/vmallocinfo to display all memory used by vmalloc on the system.
VMALLOC_START and VMALLOC_END are two symbols that delimit the vmalloc address range. They are architecture-dependent and defined in <asm/pgtable.h>.
## Process memory allocation under the hood
Let's focus on the lower level allocator, which allocates pages of memory. The kernel will report allocation of frame pages (physical pages) until really necessary (when those are actually accessed, by reading or writing). This on-demand allocation is called lazy allocation, eliminating the risk of allocating pages that will never be used.
Whenever a page is requested, only the page table is updated, in most cases a new entry is created, which means only virtual memory is allocated. Only when you access the page is an interrupt called a page fault raised. This interrupt has a dedicated handler, called the page fault handler, and is called by the MMU in response to an attempt to access virtual memory that did not immediately succeed.
Actually, a page fault interrupt is raised whatever the access type is (read, write, execute) to a page whose entry in the page table has not got the appropriate permission bits set to allow that type of access. The response to that interrupt falls into one of the following three ways:
The hard fault: The page does not reside anywhere (neither in the physical memory nor a memory-mapped file), which means the handler cannot immediately resolve the fault. The handler will perform I/O operations in order to prepare the physical page needed to resolve the fault, and may suspend the interrupted process and switch to another while the system works to resolve the issue.
The soft fault: The page resides elsewhere in memory (in the working set of another process). It means the fault handler may resolve the fault by immediately attaching a page of physical memory to the appropriate page table entry,
adjusting the entry, and resuming the interrupted instruction.
The fault cannot be resolved: This will result in a bus error or segv. SIGSEGV is sent to the faulty process, killing it (the default behavior) unless a signal handler has been installed for SIGSEV to change the default behavior.
Memory mappings generally start out with no physical pages attached, by defining the virtual address ranges without any associated physical memory. The actual physical memory is allocated later in response to a page fault exception, when the memory is accessed, since the kernel provides some flags to determine whether the attempted access was legal and specify the behavior of the page fault handler. Thus, the user space brk(),
mmap() and similar allocated (virtual) space, but physical memory is attached later.
A page fault occurring in the interrupt context causes a double fault interrupt, which usually panics the kernel (calling the panic() function) .
It is the reason why memory allocated in the interrupt context is taken from a memory pool, which does not raise page fault interrupts. If an interrupt occurs when a double fault is being handled, a triple fault exception is generated, causing the CPU to shut down and the OS
immediately reboots. This behavior is actually arc-dependent.
## The copy-on-write (CoW) case
CoW (heavily used with fork()) is a kernel feature that does not allocate several times the memory for data shared by two or more processes, until a process touches it (writes into it);
in this case, memory is allocated for its private copy. The following shows how a page fault handler manages CoW (one-page case study):
1. A PTE is added to the process page table, and marked as unwritable.
2. The mapping will result in a VMA creation in the process VMA list. The page is added to that VMA and that VMA is marked as writable.
3. On page access (at the first write), the fault handler notices the difference, which means this is a CoW. It will then allocate a physical page, which is assigned to the PTE added before, update the PTE flags, flush the TLB entry, and execute the do_wp_page() function, which can copy the content from the shared address to the new location.
Working with I/O memory to talk with hardware
Apart from performing data RAM-oriented operations, you can perform I/O memory transactions to talk with the hardware. When it comes to the access device's register, the kernel offers two possibilities depending on the system architecture:
Through the I/O ports: This is also called Port Input Output (PIO). Registers are accessible through a dedicated bus, and specific instructions (in and out, in assembler generally) are needed to access those registers. This is the case on x86
architectures.
Memory Mapped Input Output (MMIO): This is the most common and most used method. The device's registers are mapped to memory. Simply read and write to a particular address to write to the registers of the device. This is the case on ARM architectures.
## PIO devices access
On a system in which PIO is used, there are two different address spaces, one for memory,
which we have already discussed, and the other one for I/O ports, called the port address space, limited to 65,536 ports only. This is an old way, and very uncommon nowadays.
The kernel exports a few functions (symbols) to handle the I/O port. Prior to accessing any port regions, we must first inform the kernel that we are using a range of ports using the request_region() function, which will return NULL on error. Once done with the region,
you must call release_region(). These are both declared in linux/ioport.h. Their prototypes are:
```c
struct resource *request_region(unsigned long start,
unsigned long len, char *name);
void release_region(unsigned long start, unsigned long len);
```
Those functions inform the kernel about your intention to use/release a region of len ports,
starting from start. The name parameter should be set with the name of your device. Their use is not mandatory. This is a kind of politeness, which prevents two or more drivers from referencing the same range of ports. You can display information about the ports actually in use on the system by reading the content of the /proc/ioports files.
Once you are done with region reservation, you can access the port using the following functions:
```c
u8 inb(unsigned long addr)
u16 inw(unsigned long addr)
u32 inl(unsigned long addr)
```
These respectively access (read) 8-, 16-, or 32-bit-sized (wide) ports, and the following functions:
```c
void outb(u8 b, unsigned long addr)
void outw(u16 b, unsigned long addr)
void outl(u32 b, unsigned long addr)
```
These write b data, 8, 16, or 32-bit-sized, into addr port.
The fact that PIO uses a different set of instruction to access I/O ports or MMIO is a disadvantage because PIO requires more instructions than normal memory to accomplish the same task. For instance, 1-bit testing has only one instruction in MMIO, whereas PIO
requires reading the data into a register before testing the bit, which is more than one instruction.
## MMIO device access
Memory-mapped I/O resides in the same address space as memory. The kernel uses part of the address space normally used by RAM (HIGH_MEM actually) to map the device registers,
so that instead of having real memory (that is, RAM) at that address, the I/O device takes its place. Thus, communicating to an I/O device becomes like reading and writing to memory addresses devoted to that I/O device.
In other words if I need to access for say the 4 MB of memory-mapped space assigned to
IPU-2 (from 0x02A00000 to 0x02DFFFFF) of the i.MX6, the CPU (by mean of the MMU) may assign me address range 0x10000000 to 0x10400000, which is virtual of course. This is not consuming physical RAM (except for building and storing page table entries), but just address space, meaning that the kernel will no longer use this virtual memory range to map
RAM. Now any writing/reading operation at this address range ( let's say 0x10000004, for example) will be redirected to the IPU-2 device.
Like PIO, there are MMIO functions to inform the kernel about our intention to use a memory region. Remember it is a pure reservation only. These are request_mem_region() and release_mem_region():
```c
struct resource* request_mem_region(unsigned long start,
unsigned long len, char *name)
void release_mem_region(unsigned long start, unsigned long len)
```
It is also a politeness.
You can display memory regions actually in use on the system by reading the content of the /proc/iomem file.
Prior to accessing a memory region (and after you successfully request it), the region must be mapped into kernel address space by calling special architecture-dependent functions
(which make use of MMU to build the page table, and thus cannot be called from the interrupt handler). These are ioremap() and iounmap(), which handle cache coherency too:
```c
void __iomem *ioremap(unsigned long phys_add, unsigned long size)
void iounmap(void __iomem *addr)
ioremap() returns a __iomem void pointer to the start of the mapped region. Do not be tempted to deference (get/set the value by reading/writing to the pointer) such pointers.
```
The kernel provides functions to access ioremap'ed memories. These are:
```c
unsigned int ioread8(void __iomem *addr);
unsigned int ioread16(void __iomem *addr);
unsigned int ioread32(void __iomem *addr);
void iowrite8(u8 value, void __iomem *addr);
void iowrite16(u16 value, void __iomem *addr);
void iowrite32(u32 value, void __iomem *addr);
ioremap builds new page tables, just as vmalloc does. However, it does not actually allocate any memory but instead, returns a special virtual address that you can use to access the specified physical address range.
```
On 32-bit systems, the fact that MMIO steals physical memory address space to create mapping for memory-mapped I/O devices is a disadvantage, since it prevents the system from using the stolen memory for general RAM purpose.
## __iomem cookie
__iomem is a kernel cookie used by Sparse, a semantic checker used by the kernel to find possible coding faults. To take advantage of the features offered by Sparse, it should be enabled at kernel compile time; if not, __iomem cookie will be ignored anyway.
The C=1 in the command line will enable Sparse for you, but Sparse should be installed first on your system:
```bash
sudo apt-get install sparse
```
For example, when building a module, use:
```bash
make -C $KPATH M=$PWD C=1 modules
```
Alternatively, if the makefile is well-written, just type:
```bash
make C=1
```
The following shows how __iomem is defined in the kernel:
```c
#define __iomem __attribute__((noderef, address_space(2)))
```
It protects us from faulty drivers performing I/O memory access. Adding the __iomem for all I/O accesses is a way to be stricter too. Since even I/O access is done through virtual memory (on systems with MMU), this cookie prevents us from using absolute physical addresses, and requires us to use ioremap(), which will return a virtual address tagged with the __iomem cookie:
```c
void __iomem *ioremap(phys_addr_t offset, unsigned long size);
```
So, we can use dedicated functions such as ioread23() and iowrite32(). You may wonder why you do not use the readl()/writel() function. Those are deprecated, since these do not make sanity checks and are less secure (no __iomem required) than ioreadX()/iowriteX() family functions, which accept only __iomem addresses.
In addition, noderef is an attribute used by Sparse to make sure programmers do not dereference a __iomem pointer. Even though it could work on some architectures, you are not encouraged to do that. Use the special ioreadX()/iowriteX() function instead. It is portable and works on every architecture. Now, let's see how Sparse will warn us when dereferencing a __iomem pointer:
```c
#define BASE_ADDR 0x20E01F8
void * _addrTX = ioremap(BASE_ADDR, 8);
```
First, Sparse is not happy because of the wrong type initializer:
warning: incorrect type in initializer (different address spaces)
expected void *_addrTX
got void [noderef] <asn:2>*
Or there is this option:
```c
u32 __iomem* _addrTX = ioremap(BASE_ADDR, 8);
```
*_addrTX = 0xAABBCCDD; /* bad. No dereference */
```c
pr_info("%x\n", *_addrTX); /* bad. No dereference */
```
Sparse is still not happy:
Warning: dereference of noderef expression
This last example makes Sparse happy:
```c
void __iomem* _addrTX = ioremap(BASE_ADDR, 8);
iowrite32(0xAABBCCDD, _addrTX);
pr_info("%x\n", ioread32(_addrTX));
```
The two rules that you must remember are:
Always use __iomem where it is required, whether it is as a return type or as a parameter type, and use Sparse to make sure you did so
Do not dereference a __iomem pointer; use a dedicated function instead
## Memory (re)mapping
Kernel memory sometimes needs to be remapped, either from kernel to user space, or from kernel to kernel space. The common use case is remapping the kernel memory to the user space, but there are other cases when you need to access high memory, for example.
## kmap
The Linux kernel permanently maps 896 MB of its address space to the lower 896 MB of the physical memory (low memory). On a 4 GB system, there is only 128 MB left to the kernel to map the remaining 3.2 GB of physical memory (high memory). Low memory is directly addressable by the kernel because of the permanent and one-to-one mapping. When it comes to high memory (memory above 896 MB), the kernel has to map the requested region of high memory into its address space, and the 128 MB mentioned before is especially reserved for this. The function used to perform this trick, kmap(). kmap() , is used to map a given page into the kernel address space:
```c
void *kmap(struct page *page);
```
page is a pointer to the struct page structure to map. When a high-memory page is allocated, it is not directly addressable. kmap() is the function you must call to temporarily map high memory into the kernel address space. The mapping will last until kunmap() is called:
```c
void kunmap(struct page *page);
```
By temporarily, I mean the mapping should be undone as soon as it is not needed anymore.
Remember, 128 MB is not enough to map 3.2 GB. The best programming practice is to unmap high memory mappings when no longer required. It is why the kmap() - kunmap()
sequence has to be entered around every access to the high memory page.
This function works on both high memory and low memory. That says, if the page structure resides in low memory, then just the virtual address of the page is returned
(because low-memory pages already have permanent mappings). If the page belongs to high memory, a permanent mapping is created in the kernel's page tables and the address is returned:
```c
void *kmap(struct page *page)
{
```
BUG_ON(in_interrupt());
```c
if (!PageHighMem(page))
return page_address(page);
return kmap_high(page);
}
```
## Mapping kernel memory to user space
Mapping physical addresses is one of the most useful functionalities, especially in embedded systems. Sometime, you may want to share part of kernel memory with the user space. As said earlier, CPU runs in unprivileged mode when running in user space. To let a process access a kernel memory region, we need to remap that region into the process address space.
## Using remap_pfn_range remap_pfn_range() maps physical memory (by means of kernel logical address) to a user space process. It is particularly useful for implementing the mmap() system call.
After calling the mmap() system call on a file (whether it is a device file or not), the CPU
will switch to privileged mode and run the corresponding file_operations.mmap()
kernel function, which in turn will call remap_pfn_range(). The kernel PTE of the mapped region will be derived and given to the process, of course with different protection flags. The process's VMA list is updated with a new VMA entry (with appropriate attributes) , which will use PTE to access the same memory.
Thus, instead of wasting memory by copying, the kernel just duplicates the PTEs. However,
kernel and user space PTEs have different attributes. remap_pfn_range() has the following prototype:
```c
int remap_pfn_range(struct vm_area_struct *vma, unsigned long addr,
unsigned long pfn, unsigned long size, pgprot_t flags);
```
A successful call will return 0, and a negative error code on failure. Most of the arguments for remap_pfn_range() are provided when the mmap() method is called:
vma: This is the virtual memory area provided by the kernel in the case of a file_operations.mmap() call. It corresponds to the user process vma into which the mapping should be done.
addr: This is the user virtual address where VMA should start
```c
(vma->vm_start), which will result in a mapping from a virtual address range between addr and addr + size.
pfn: This represents the PFN, of the kernel memory region to map. It corresponds to the physical address right-shifted by PAGE_SHIFT bits. The vma offset (offset into the object where the mapping must start) should be taken into account to produce the PFN. Since the vm_pgoff field of the VMA structure contains the offset value in the form of the number of pages, it is precisely what you need (with a PAGE_SHIFT left-shifting) to extract the offset in the form of bytes: offset = vma->vm_pgoff << PAGE_SHIFT). Finally, pfn =
```
virt_to_phys(buffer + offset) >> PAGE_SHIFT.
size: This is the dimension, in bytes, of the area being remapped.
```c
prot: This represents the protection requested for the new VMA. The driver can mangle the default value, but should use the value found in vma->vm_page_prot as the skeleton using the OR operator, since some of its bits are already set by user space. Some of these flags are:
```
VM_IO, which specifies a device's memory mapped I/O
VM_DONTCOPY, which tells the kernel not to copy this vma on fork
VM_DONTEXPAND, which prevents vma from expanding with mremap(2)
VM_DONTDUMP, which prevents the vma from being included in the core dump
```c
You may need to modify this value in order to disable caching if using this with I/O memory (vma->vm_page_prot =
pgprot_noncached(vma->vm_page_prot);).
```
## Using io_remap_pfn_range
The remap_pfn_range() function discussed does not apply anymore when it comes to mapping I/O memory to user space. In this case, the appropriate function is io_remap_pfn_range(), whose parameters are the same. The only thing that changes is where the PFN comes from. Its prototype looks like:
```c
int io_remap_page_range(struct vm_area_struct *vma,
unsigned long virt_addr,
unsigned long phys_addr,
unsigned long size, pgprot_t prot);
```
There is no need to use ioremap() when attempting to map I/O memory to user space.
```c
ioremap() is intended for kernel purposes (mapping I/O memory into the kernel address space), whereas io_remap_pfn_range is for user space purposes.
```
Just pass your real physical I/O address (downshifted by PAGE_SHIFT to produce a PFN)
directly to io_remap_pfn_range(). Even if there are some architectures where io_remap_pfn_range() is defined as being remap_pfn_range(), there are other architectures where this is not the case. For portability reasons, you should only use remap_pfn_range() in situations where the PFN parameter points to RAM, and io_remap_pfn_range() in situations where phys_addr refers to I/O memory.
## The mmap file operation
The kernel mmap function is part of the struct file_operations structure, which is executed when the user executes the mmap(2) system call used to map physical memory into a user virtual address. The kernel translates any access to that mapped region of memory through the usual pointer dereferences into a file operation. It is even possible to map device physical memory directly to the user space (see /dev/mem). Essentially, writing to memory becomes like writing into a file. It is just a more convenient way of calling write().
Normally, user space processes cannot access device memory directly for security purposes. Therefore, user space processes use the mmap() system call to ask the kernel to map the device into the virtual address space of the calling process. After the mapping, the user space process can write directly into the device memory through the returned address.
The mmap system call is declared as follows:
mmap (void *addr, size_t len, int prot,
```c
int flags, int fd, ff_t offset);
```
The driver should have defined the mmap file operation (file_operations.mmap) in order to support mmap(2). From the kernel side, the mmap field in the driver's file operation structure (struct file_operations structure) has the following prototype:
```c
int (*mmap) (struct file *filp, struct vm_area_struct *vma);
```
Here:
filp is a pointer to the open device file for the driver that results from the translation of the fd parameter.
vma is allocated and given as a parameter by the kernel. It is a pointer to the user process's vma where the mapping should go. To understand how the kernel creates the new vma, let's recall the mmap(2) system call's prototype:
```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```
The parameters of this function somehow affect some fields of the vma:
addr: This is the user space's virtual address where the mapping should start. It has an impact on vma>vm_start. If NULL (the most portable way) is specified,
automatically determines the correct address.
```c
length: This specifies the length of the mapping, and indirectly has an impact on vma->vm_end. Remember, the size of a vma is always a multiple of PAGE_SIZE.
```
In other words, PAGE_SIZE is always the smallest size a vma can have. The kernel will always alter the size of the vma so that is is a multiple of PAGE_SIZE.
If length <= PAGE_SIZE
```c
vma->vm_end - vma->vm_start == PAGE_SIZE.
```
If PAGE_SIZE < length <= (N * PAGE_SIZE)
```c
vma->vm_end - vma->vm_start == (N * PAGE_SIZE)
prot: This affects the permissions of the VMA, which the driver can find in vma->vm_pro. As discussed earlier, the driver can update these values, but not alter them.
flags: This determines the type of mapping that the driver can find in vma->vm_flags. The mapping can be private or shared.
offset: This specifies the offset within the mapped region, thus mangling the value of vma->vm_pgoff.
```
## Implementing mmap in the kernel
Since user space code cannot access kernel memory, the purpose of the mmap() function is to derive one or more protected kernel page table entries (which correspond to the memory to be mapped) and duplicate the user space page tables, remove the kernel flag protection,
and set permission flags that will allow the user to access the same memory as the kernel without needing special privileges.
The steps to write a mmap file operation are as follows:
1. Get the mapping offset and check whether it is beyond our buffer size or not:
```c
unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
if (offset >= buffer_size)
return -EINVAL;
```
2. Check whether the mapping size is bigger than our buffer size:
```c
unsigned long size = vma->vm_end - vma->vm_start;
if (size > (buffer_size - offset))
return -EINVAL;
```
3. Get the PFN that corresponds to the PFN of the page where the offset position of our buffer falls:
```c
unsigned long pfn;
```
/* we can use page_to_pfn on the struct page structure
* returned by virt_to_page
*/
/* pfn = page_to_pfn (virt_to_page (buffer + offset)); */
/* Or make PAGE_SHIFT bits right-shift on the physical
* address returned by virt_to_phys
*/
pfn = virt_to_phys(buffer + offset) >> PAGE_SHIFT;
4. Set the appropriate flag, whether I/O memory is present or not:
```c
Disable caching using vma->vm_page_prot =
pgprot_noncached(vma->vm_page_prot).
Set the VM_IO flag: vma->vm_flags |= VM_IO.
```
Prevent the VMA from swapping out:
```c
vma->vm_flags |= VM_DONTEXPAND |
```
VM_DONTDUMP. In kernel versions older than 3.7, you should only use the VM_RESERVED flag instead.
5. Call remap_pfn_range with the PFN calculated, the size, and the protection flags:
```c
if (remap_pfn_range(vma, vma->vm_start, pfn, size,
vma->vm_page_prot)) {
return -EAGAIN;
}
return 0;
```
6. Pass your mmap function to the struct file_operations structure:
```c
static const struct file_operations my_fops = {
```
.owner = THIS_MODULE,
[...]
.mmap = my_mmap,
[...]
```c
};
```
## Linux caching system
Caching is the process by which frequently accessed or newly written data is fetched from or written to a small and faster memory, called a cache.
Dirty memory is data-backed (for example, file-backed) memory whose content has been modified (typically in a cache) but not written back to the disk yet. The cached version of the data is newer than the on-disk version, meaning that both versions are out of sync. The mechanism by which cached data is written back on the disk (back store) is called writeback. We will eventually update the on-disk version, bringing the two in sync. Clean memory is file-backed memory in which the contents are in sync with the disk.
Linux delays write operations in order to speed up the read process, and reduces disk wear leveling by writing data only when necessary. A typical example is the dd command. Its complete execution does not mean that the data is written to the target device; this is the reason why dd in most cases is chained to a sync command.
## What is a cache?
A cache is temporary, small, and fast memory used to keep copies of data from larger and often very slow memory, typically placed in systems where there is a working set of data accessed far more often than the rest (for example, hard drive, memory).
When the first read occurs, let's say a process requests some data from the large and slower disk, the requested data is returned to the process, and a copy of accessed data is tracked and cached as well. Any consequent read will fetch data from the cache. Any data modification will be applied in the cache, not on the main disk. Then, the cache region whose content has been modified and differs from (is newer than) the on-disk version will be tagged as dirty. When the cache runs full, and since cached data is tacked, new data begins to evict the data that has not been accessed and has been sitting idle for the longest,
so that if it is needed again, it will have to be fetched from the large/slow storage again.
## CPU cache – memory caching
There are three cache memories on the modern CPU, ordered by size and access speed:
```c
The L1 cache has the smallest amount of memory (often between 1K and 64K)
```
and is directly accessible by the CPU in a single clock cycle, which makes it the fastest as well. Frequently used things are in L1 and remain in L1 until some other thing's usage becomes more frequent than the existing one and there is less space in L1. If so, it is moved to a bigger L2.
The L2 cache is the middle level, with a larger amount of memory (up to several megabytes) adjacent to the processor, which can be accessed in a small number of clock cycles. This applies when moving things from L2 to L3.
The L3 cache, even slower than L1 and L2, may be twice as fast as the main memory (RAM). Each core may have its own L1 and L2 cache; therefore, they all share the L3 cache. Size and speed are the main criteria that change between each cache level: L1 < L2 < L3. Whereas original memory access may be 100 ns, for example, the L1 cache access can be 0.5 ns.
A real-life example is how a library may put several copies of the most popular titles on display for easy and fast access, but have a large-scale archive with a far greater collection available, at the inconvenience of having to wait for a librarian to go get it for you. The display cases would be analogous to a cache, and the archive would be the large, slow memory.
The main issue that a CPU cache addresses is latency, which indirectly increases the throughput, because access to uncached memory may take a while.
## The Linux page cache – disk caching
The page cache, as its name suggests, is a cache of pages in RAM, containing chunks of recently accessed files. The RAM acts as a cache for pages that resides on the disk. In other words, it is the kernel cache of file contents. Cached data may be regular filesystem files,
block device files, or memory-mapped files. Whenever a read() operation is invoked, the kernel first checks whether the data resides in the page cache, and immediately returns it if found. Otherwise, the data will be read from the disk.
If a process needs to write data without any caching involved, it has to use the O_SYNC flag, which guarantees the write() command will not return before all data has been transferred to the disk, or the O_DIRECT, flag,
which only guarantees that no caching will be used for data transfer. That said, O_DIRECT actually depends on the filesystem used and is not recommended.
## Specialized caches (user space caching)
Web browser cache: This stores frequently accessed web pages and images onto the disk, instead of fetching them from the web. Whereas the first access of online data may last for more than hundreds of milliseconds, the second access will fetch data from the cache (which is a disk in this case) in only 10 ms.
libc or user-app cache: Memory and disk cache implementations will try to guess what you need to use next, while browser caches keep a local copy in case you need to use it again.
## Why delay writing data to disk?
There are essentially two reasons for that:
Better use of the disk characteristics; this is efficiency
Allows the application to continue immediately after a write; this is performance
For example, delaying disk access and processing data only when it reaches a certain size may improve disk performance and reduce wear leveling of eMMC (on embedded systems). Every chunk write is merged into a single and contiguous write operation.
Additionally, written data is cached, allowing the process to return immediately so that any subsequent read will fetch the data from the cache, resulting in a more responsive program.
Storage devices prefer a small number of large operations instead of several small operations.
By reporting write operations on the permanent storage later, we can get rid of latency issues introduced by these disks, which are relatively slow.
## Write caching strategies
Depending on the cache strategy, several benefits may be enumerated:
Reduced latency on data accessing, thus increasing application performance
Improved storage lifetime
Reduced system workload
Reduced risk of data loss
Caching algorithms usually fall into one of the following three different strategies:
1. The write-through cache is where any write operation will automatically update both the memory cache and the permanent storage. This strategy is preferred for applications where data loss cannot be tolerated, and applications that write and then frequently re-read data (since data is stored in the cache and results in low read latency).
2. The write-around cache is similar to write-through, with the difference that it immediately invalidates the cache (which is also costly for the system since any write results in automatic cache invalidation). The main consequence is that any subsequent read will fetch data from the disk, which is slow, thus increasing latency. It prevents the cache from being flooded with data that will not be subsequently read.
3. Linux employs the third and last strategy, called the write-back cache, which can write data to the cache every time a change occurs without updating the corresponding location in the main memory. Instead, the corresponding pages in the page cache are marked as dirty (this task is done by MMU using TLB) and added to a so-called list, maintained by the kernel. The data is written into the corresponding location in the permanent storage only at specified intervals or under certain conditions. When the data in the pages is up to date with the data in the page cache, the kernel removes the pages from the list, and they are not marked dirty.
4. On Linux systems, you can find this in /proc/meminfo under Dirty:
```bash
cat /proc/meminfo | grep Dirty
```
## The flusher threads
The write back cache defers I/O data operations in the page cache. A set or kernel threads,
called flusher threads, are responsible for that. Dirty page write back occurs when any one of the following situations is satisfied:
When free memory falls below a specified threshold to regain memory consumed by dirty pages.
When dirty data lasts until a specific period. The oldest data is written back to the disk to ensure that dirty data does not remain dirty indefinitely.
When a user process invokes the sync() and fsync() system calls. This is an on-demand write back.
## Device-managed resources – Devres
Devres is a kernel facility helping the developer by automatically freeing the allocated resource in a driver. It simplifies errors handling in init/probe/open functions. With
Devres, each resource allocator has its managed version that will take care of resource release and freeing for you.
This section heavily relies on the Documentation/drivermodel/devres.txt file in the kernel source tree, which deals with the devres API and lists supported functions along with their descriptions.
The memory allocated with resource-managed functions is associated with the device.
devres consists of a linked list of arbitrarily sized memory areas associated with a struct device. Each devres resource allocator inserts the allocated resource in the list. The resource remains available until it is manually freed by the code, when the device is detached from the system, or when the driver is unloaded. Each devres entry is associated with a release function. There are different ways to release a devres. No matter what, all devres entries are released on driver detach. On release, the associated release function is invoked and then the devres entry is freed.
The following is the list of resources available for a driver:
Memory for private data structures
```c
Interrutps (IRQs)
```
Memory region allocation (request_mem_region())
I/O mapping of memory regions (ioremap())
```c
Buffer memory (possibly with DMA mapping)
```
Different framework data structures: clocks, GPIOs, PWMs, USB phy, regulators,
DMA, and so on
Almost every function discussed in this chapter has its managed version. In the majority of cases, the name given to the managed version of a function is obtained by prefixing the original function name with devm. For example, devm_kzalloc() is the managed version of kzalloc(). Additionally, parameters remain unchanged, but are shifted to the right,
since the first parameter is the struct device for which the resource is allocated. There is an exception for functions for which the non-managed version is already given a struct device in its parameters:
```c
void *kmalloc(size_t size, gfp_t flags)
void * devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
```
When the device is detached from the system or the driver for the device is unloaded, that memory is freed automatically. It is possible to free the memory with devm_kfree() if it's no longer needed.
This is the old way:
ret = request_irq(irq, my_isr, 0, my_name, my_data);
```c
if(ret) {
dev_err(dev, "Failed to register IRQ.\n");
```
ret = -ENODEV;
goto failed_register_irq; /* Unroll */
```c
}
```
This is the right way:
ret = devm_request_irq(dev, irq, my_isr, 0, my_name, my_data);
```c
if(ret) {
dev_err(dev, "Failed to register IRQ.\n");
return -ENODEV; /* Automatic unroll */
}
```
## Summary
This chapter is one of the most important chapters. It demystifies memory management and allocation (how and where) in the kernel. Every memory aspect is discussed and detailed, and devres is also explained. The caching mechanism is briefly discussed in order to give an overview of what goes on under the hood during I/O operations. It is a strong base from which to introduce and understand the next chapter, which deals with DMA