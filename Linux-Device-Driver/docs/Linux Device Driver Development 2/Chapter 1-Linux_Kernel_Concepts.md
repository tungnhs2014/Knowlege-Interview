# Linux Kernel Concepts for Embedded Developers

1
## Linux Kernel
Concepts for
## Embedded
Developers
## As a standalone software, the Linux kernel implements a set of functions that help not to
reinvent the wheel and ease device driver developments. The importance of these helpers
is that it’s not a requirement to use these for code to be accepted upstream. This is the
kernel core that drivers rely on. We’ll cover the most popular of these core functionalities
in this book, though other ones also exist. We will begin by looking at the kernel locking
API before discussing how to protect shared objects and avoid race conditions. Then, we
will look at various work deferring mechanisms available, where you will learn what part
of the code to defer in which execution context. Finally, you will learn how interrupts
work and how to design interrupt handlers from within the Linux kernel.
4 Linux Kernel Concepts for Embedded Developers
This chapter will cover the following topics:
• The kernel locking API and shared objects
• Work deferring mechanisms
• Linux kernel interrupt management
Let’s get started!
## Technical requirements
To understand and follow this chapter’s content, you will need the following:
• Advanced computer architecture knowledge and C programming skills
• Linux kernel 4.19 sources, available at https://github.com/torvalds/
linux
## The kernel locking API and shared objects
A resource is said to be shared when it can be accessed by several contenders, regardless
of their exclusively. When they are exclusive, access must be synchronized so that only
the allowed contender(s) may own the resource. Such resources might be memory
locations or peripheral devices, while the contenders might be processors, processes, or
threads. Operating systems perform mutual exclusion by atomically (that is, by means of
an operation that can be interrupted) modifying a variable that holds the current state
of the resource, making this visible to all contenders that might access the variable at the
same time. This atomicity guarantees that the modification will either be successful, or
not successful at all. Nowadays, modern operating systems rely on the hardware (which
should allow atomic operations) used for implementing synchronization, though a simple
system may ensure atomicity by disabling interrupts (and avoiding scheduling) around the
critical code section.
In this section, we’ll describe the following two synchronization mechanisms:
• Locks: Used for mutual exclusion. When one contender holds the lock, no
other contender can hold it (others are excluded). The most well-known locking
primitives in the kernel are spinlocks and mutexes.
• Conditional variables: Mostly used to sense or wait for a state change. These are
implemented differently in the kernel, as we will see later, mainly in the Waiting,
sensing, and blocking in the Linux kernel section.
## The kernel locking API and shared objects 5
When it comes to locking, it is up to the hardware to allow such synchronizations by
means of atomic operations. The kernel then uses these to implement locking facilities.
## Synchronization primitives are data structures that are used for coordinating access to
shared resources. Because only one contender can hold the lock (and thus access the
shared resource), it might perform an arbitrary operation on the resource associated with
the lock that would appear to be atomic to others.
## Apart from dealing with the exclusive ownership of a given shared resource, there are
```c
situations where it is better to wait for the state of the resource to change; for example,
```
waiting for a list to contain at least one object (its state then passes from empty to not
empty) or for a task to complete (a DMA transaction, for example). The Linux kernel
does not implement conditional variables. From our user space, we could think of using a
conditional variable for both situations, but to achieve the same or even better, the kernel
provides the following mechanisms:
• Wait queue: Mainly used to wait for a state change. It’s designed to work in concert
with locks.
• Completion queue: Used to wait for a given computation to complete.
## Both mechanisms are supported by the Linux kernel and are exposed to drivers thanks to
a reduced set of APIs (which significantly ease their use when used by a developer). We
will discuss these in the upcoming sections.
## Spinlocks
A spinlock is a hardware-based locking primitive. It depends on the capabilities of the
hardware at hand to provide atomic operations (such as test_and_set, which, in
a non-atomic implementation, would result in read, modify, and write operations).
## Spinlocks are essentially used in an atomic context where sleeping is not allowed or simply
not needed (in interrupts, for example, or when you want to disable preemption), but also
as an inter-CPU locking primitive.
It is the simplest locking primitive and also the base one. It works as follows:
Figure 1.1 – Spinlock contention flow
6 Linux Kernel Concepts for Embedded Developers
Let’s explore the diagram by looking at the following scenario:
## When CPUB, which is running task B, wants to acquire the spinlock thanks to the
spinlock’s locking function and this spinlock is already held by another CPU (let’s say
CPUA, running task A, which has already called this spinlock’s locking function), then
## CPUB will simply spin around a while loop, thus blocking task B until the other CPU
releases the lock (task A calls the spinlock’s release function). This spinning will only
happen on multi-core machines, which is why the use case described previously, which
involves more than one CPU since it’s on a single core machine, cannot happen: the task
either holds a spinlock and proceeds or doesn’t run until the lock is released. I used to
say that a spinlock is a lock held by a CPU, which is the opposite of a mutex (we will
discuss this in the next section), which is a lock held by a task. A spinlock operates by
disabling the scheduler on the local CPU (that is, the CPU running the task that called
the spinlock’s locking API). This also means that the task currently running on that CPU
cannot be preempted by another task, except for IRQs if they are not disabled (more on
this later). In other words, spinlocks protect resources that only one CPU can take/access
at a time. This makes spinlocks suitable for SMP safety and for executing atomic tasks.
## Important note
Spinlocks are not the only implementation that take advantage of hardware’s
atomic functions. In the Linux kernel, for example, the preemption status
depends on a per-CPU variable that, if equal to 0, means preemption is
enabled. However, if it’s greater than 0, this means preemption is disabled
(schedule() becomes inoperative). Thus, disabling preemption
(preempt_disable()) consists of adding 1 to the current per-CPU
variable (preempt_count actually), while preempt_enable()
subtracts 1 (one) from the variable, checks whether the new value is 0, and calls
schedule(). These addition/subtraction operations should then be atomic,
and thus rely on the CPU being able to provide atomic addition/subtraction
functions.
## The kernel locking API and shared objects 7
There are two ways to create and initialize a spinlock: either statically using the DEFINE_
## SPINLOCK macro, which will declare and initialize the spinlock, or dynamically by calling
```c
spin_lock_init() on an uninitialized spinlock.
```
First, we’ll introduce how to use the DEFINE_SPINLOCK macro. To understand how this
works, we must look at the definition of this macro in include/linux/spinlock_
types.h, which is as follows:
```c
#define DEFINE_SPINLOCK(x) spinlock_t x = __SPIN_LOCK_
```
## UNLOCKED(x)
This can be used as follows:
```c
static DEFINE_SPINLOCK(foo_lock)
```
After this, the spinlock will be accessible through its name, foo_lock. Note that its
address would be &foo_lock. However, for dynamic (runtime) allocation, you need to
embed the spinlock into a bigger structure, allocate memory for this structure, and then
call spin_lock_init() on the spinlock element:
```c
struct bigger_struct {
spinlock_t lock;
unsigned int foo;
```
[...]
```c
};
static struct bigger_struct *fake_alloc_init_function()
{
struct bigger_struct *bs;
bs = kmalloc(sizeof(struct bigger_struct), GFP_KERNEL);
if (!bs)
return -ENOMEM;
spin_lock_init(&bs->lock);
return bs;
}
```
It is better to use DEFINE_SPINLOCK whenever possible. It offers compile-time
initialization and requires less lines of code with no real drawback. At this stage, we
can lock/unlock the spinlock using the spin_lock() and spin_unlock() inline
functions, both of which are defined in include/linux/spinlock.h:
```c
void spin_unlock(spinlock_t *lock)
void spin_lock(spinlock_t *lock)
```
8 Linux Kernel Concepts for Embedded Developers
That being said, there are some limitations to using spinlocks this way. Though a spinlock
prevents preemption on the local CPU, it does not prevent this CPU from being hogged
by an interrupt (thus, executing this interrupt’s handler). Imagine a situation where the
CPU holds a “spinlock” in order to protect a given resource and an interrupt occurs. The
CPU will stop its current task and branch out to this interrupt handler. So far, so good.
Now, imagine that this IRQ handler needs to acquire this same spinlock (you’ve probably
already guessed that the resource is shared with the interrupt handler). It will infinitely
spin in place, trying to acquire a lock that’s already been locked by a task that it has
preempted. This situation is known as a deadlock.
To address this issue, the Linux kernel provides _irq variant functions for spinlocks,
which, in addition to disabling/enabling the preemption, also disable/enable interrupts
on the local CPU. These functions are spin_lock_irq() and spin_unlock_irq(),
and they are defined as follows:
```c
void spin_unlock_irq(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
```
You might think that this solution is sufficient, but it is not. The _irq variant partially
solves this problem. Imagine that interrupts are already disabled on the processor before
your code starts locking. So, when you call spin_unlock_irq(), you will not just
release the lock, but also enable interrupts. However, this will probably happen in an
erroneous manner since there is no way for spin_unlock_irq() to know which
interrupts were enabled before locking and which weren’t.
The following is a short example of this:
1. Let’s say interrupts x and y were disabled before a spinlock was acquired, while z
was not.
2. spin_lock_irq() will disable the interrupts (x, y, and z are now disabled) and
take the lock.
3. spin_unlock_irq() will enable the interrupts. x, y, and z will all be enabled,
which was not the case before the lock was acquired. This is where the problem
arises.
This makes spin_lock_irq() unsafe when it’s called from IRQs that are off-context
as its counterpart, spin_unlock_irq(), will naively enable IRQs with the risk of
enabling those that were not enabled while spin_lock_irq() was invoked. It only
```c
makes sense to use spin_lock_irq()when you know that interrupts are enabled; that
```
is, you are sure nothing else might have disabled interrupts on the local CPU.
## The kernel locking API and shared objects 9
Now, imagine that you save the status of your interrupts in a variable before acquiring
the lock and restoring them to how they were while they were releasing. In this situation,
there would be no more issues. To achieve this, the kernel provides _irqsave variant
functions. These behave just like the _irq ones, while also saving and restoring the
interrupts status feature. These functions are spin_lock_irqsave() and spin_
lock_irqrestore(), and they are defined as follows:
```c
spin_lock_irqsave(spinlock_t *lock, unsigned long flags)
```
spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
## Important note
```c
spin_lock() and all its variants automatically call preempt_
```
disable(), which disables preemption on the local CPU. On the other
hand, spin_unlock() and its variants call preempt_enable(),
which try to enable (yes, try! – it depends on whether other spinlocks are
locked, which would affect the value of the preemption counter) preemption,
and which internally call schedule() if enabled (depending on the current
value of the counter, which should be 0). spin_unlock() is then a
preemption point and might reenable preemption.
## Disabling interrupts versus only disabling preemption
Though disabling interrupts may prevent kernel preemption (a scheduler’s timer
interrupts would be disabled), nothing prevents the protected section from invoking the
scheduler (the schedule() function). Lots of kernel functions indirectly invoke the
scheduler, such as those that deal with spinlocks. As a result, even a simple printk()
function may invoke the scheduler since it deals with the spinlock that protects the kernel
message buffer. The kernel disables or enables the scheduler (performs preemption) by
increasing or decreasing a kernel-global and per-CPU variable (that defaults to 0, meaning
“enabled”) called preempt_count. When this variable is greater than 0 (which is
checked by the schedule() function), the scheduler simply returns and does nothing.
Every time a spin_lock*-related helper gets invoked, this variable is increased by 1. On the
other hand, releasing a spinlock (any spin_unlock* family function) decreases it by
1, and whenever it reaches 0, the scheduler is invoked, meaning that your critical section
would not be very atomic.
## Thus, if your code does not trigger preemption itself, it can only be protected from
preemption by disabling interrupts. That being said, code that’s locked a spinlock may not
sleep as there would be no way to wake it up (remember, timer interrupts and schedulers
are disabled on the local CPU).
10 Linux Kernel Concepts for Embedded Developers
Now that we are familiar with the spinlock and its subtilities, let’s look at the mutex, which
is our second locking primitive.
## Mutexes
The mutex is the other locking primitive we will discuss in this chapter. It behaves just like
the spinlock, with the only difference being that your code can sleep. If you try to lock a
mutex that is already held by another task, your task will find itself suspended, and it will
only be woken when the mutex is released. There’s no spinning this time, which means
that the CPU can process something else while your task is waiting. As I mentioned
previously, a spinlock is a lock held by a CPU, while a mutex is a lock held by a task.
## A mutex is a simple data structure that embeds a wait queue (to put contenders to sleep),
while a spinlock protects access to this wait queue. The following is what struct mutex
looks like:
```c
struct mutex {
atomic_long_t owner;
spinlock_t wait_lock;
```
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
```c
struct optimistic_spin_queue osq; /* Spinner MCS lock */
```
#endif
```c
struct list_head wait_list;
```
[...]
```c
};
```
## In the preceding code, the elements that are only used in debugging mode have been
removed for the sake of readability. However, as you can see, mutexes are built on top of
spinlocks. owner represents the process that actually owns (hold) the lock. wait_list
is the list in which the mutex’s contenders are put to sleep. wait_lock is the spinlock
that protects wait_list while contenders are inserted and are put to sleep. This helps
keep wait_list coherent on SMP systems.
The mutex APIs can be found in the include/linux/mutex.h header file. Prior
to acquiring and releasing a mutex, it must be initialized. As for other kernel core data
structures, there may be a static initialization, as follows:
```c
static DEFINE_MUTEX(my_mutex);
```
## The kernel locking API and shared objects 11
The following is the definition of the DEFINE_MUTEX() macro:
```c
#define DEFINE_MUTEX(mutexname) \
struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)
```
The second approach the kernel offers is dynamic initialization. This can be done by
making a call to the low-level __mutex_init() function, which is actually wrapped by
a much more user-friendly macro known as mutex_init():
```c
struct fake_data {
struct i2c_client *client;
u16 reg_conf;
struct mutex mutex;
};
static int fake_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
```
[...]
```c
mutex_init(&data->mutex);
```
[...]
```c
}
```
## Acquiring (also known as locking) a mutex is as simple calling one of the following
three functions:
```c
void mutex_lock(struct mutex *lock);
int mutex_lock_interruptible(struct mutex *lock);
int mutex_lock_killable(struct mutex *lock);
```
## If the mutex is free (unlocked), your task will immediately acquire it without going to
sleep. Otherwise, your task will be put to sleep in a manner that depends on the locking
function you use. With mutex_lock(), your task will be put in an uninterruptible
sleep (TASK_UNINTERRUPTIBLE) while you wait for the mutex to be released (in case
it is held by another task). mutex_lock_interruptible() will put your task in an
interruptible sleep, in which the sleep can be interrupted by any signal. mutex_lock_
killable() will allow your task’s sleep to be interrupted, but only by signals that
actually kill the task. Both functions return zero if the lock has been acquired successfully.
## Moreover, interruptible variants return -EINTR when the locking attempt is interrupted
by a signal.
12 Linux Kernel Concepts for Embedded Developers
## Whatever locking function is used, the mutex owner (and only the owner) should release
the mutex using mutex_unlock(), which is defined as follows:
```c
void mutex_unlock(struct mutex *lock);
```
If you wish to check the status of your mutex, you can use mutex_is_locked():
```c
static bool mutex_is_locked(struct mutex *lock)
```
## This function simply checks whether the mutex owner is NULL and returns true if it is, or
false otherwise.
## Important note
It is only recommended to use mutex_lock()when you can guarantee
that the mutex will not be held for a long time. Typically, you should use the
interruptible variant instead.
There are specific rules when using mutexes. The most important are enumerated in the
kernel’s mutex API header file, include/linux/mutex.h. The following is an excerpt
from it:
* - only one task can hold the mutex at a time
* - only the owner can unlock the mutex
* - multiple unlocks are not permitted
* - recursive locking is not permitted
* - a mutex object must be initialized via the API
* - a mutex object must not be initialized via memset or
copying
* - task may not exit with mutex held
* - memory areas where held locks reside must not be freed
* - held mutexes must not be reinitialized
* - mutexes may not be used in hardware or software interrupt
* contexts such as tasklets and timers
The full version can be found in the same file.
Now, let’s look at some cases where we can avoid putting the mutex to sleep while it is
being held. This is known as the try-lock method.
## The kernel locking API and shared objects 13
The try-lock method
## There are cases where we may need to acquire the lock if it is not already held by another
elsewhere. Such methods try to acquire the lock and immediately (without spinning if we
are using a spinlock, nor sleeping if we are using a mutex) return a status value. This tells
us whether the lock has been successfully locked. They can be used if we do not need to
access the data that’s being protected by the lock when some other thread is holding
the lock.
Both the spinlock and mutex APIs provide a try-lock method. They are called spin_
trylock() and mutex_trylock(), respectively. Both methods return 0 on a failure
(the lock is already locked) or 1 on a success (lock acquired). Thus, it makes sense to use
these functions along with an statement:
```c
int mutex_trylock(struct mutex *lock)
```
spin_trylock() actually targets spinlocks. It will lock the spinlock if it is not already
locked in the same way that the spin_lock() method is. However, it immediately
returns 0 without spinning if the spinlock is already locked:
```c
static DEFINE_SPINLOCK(foo_lock);
```
[...]
```c
static void foo(void)
{
```
[...]
```c
if (!spin_trylock(&foo_lock)) {
```
/* Failure! the spinlock is already locked */
[...]
```c
return;
}
```
/*
* reaching this part of the code means
that the
* spinlock has been successfully locked
*/
[...]
```c
spin_unlock(&foo_lock);
```
[...]
```c
}
```
14 Linux Kernel Concepts for Embedded Developers
On the other hand, mutex_trylock() targets mutexes. It will lock the mutex if it is
not already locked in the same way that the mutex_lock() method is. However, it
immediately returns 0 without sleeping if the mutex is already locked. The following is an
example of this:
```c
static DEFINE_MUTEX(bar_mutex);
```
[...]
```c
static void bar (void)
{
```
[...]
```c
if (!mutex_trylock(&bar_mutex))
```
/* Failure! the mutex is already locked */
[...]
```c
return;
}
```
/*
* reaching this part of the code means that the mutex has
* been successfully locked
*/
[...]
```c
mutex_unlock(&bar_mutex);
```
[...]
```c
}
```
## In the preceding code, the try-lock is being used along with the if statement so that the
driver can adapt its behavior.
## Waiting, sensing, and blocking in the Linux kernel
This section could have been named kernel sleeping mechanism as the mechanisms we
will deal with involve putting the processes involved to sleep. A device driver, during its
life cycle, can initiate completely separate tasks, some of which depend on the completion
of others. The Linux kernel addresses such dependencies with struct completion
items. On the other hand, it may be necessary to wait for a particular condition to become
true or the state of an object to change. This time, the kernel provides work queues to
address this situation.
## The kernel locking API and shared objects 15
Waiting for completion or a state change
## You may not necessarily be waiting exclusively for a resource, but for the state of a
given object (shared or not) to change or for a task to complete. In kernel programming
practices, it is common to initiate an activity outside the current thread, and then wait
for that activity to complete. Completion is a good alternative to sleep() when you’re
waiting for a buffer to be used, for example. It is suitable for sensing data, as is the
case with DMA transfers. Working with completions requires including the <linux/
completion.h> header. Its structure looks as follows:
```c
struct completion {
unsigned int done;
wait_queue_head_t wait;
};
```
## You can create instances of the struct completion structure either statically using the
```c
static DECLARE_COMPLETION(my_comp) function or dynamically by wrapping
```
the completion structure into a dynamic (allocated on the heap, which will be
alive for the lifetime of the function/driver) data structure and invoking init_
completion(&dynamic_object->my_comp). When the device driver performs
some work (a DMA transaction, for example) and others (threads, for example) need to
be notified of their completion, the waiter has to call wait_for_completion() on the
previously initialized struct completion object in order to be notified of this:
```c
void wait_for_completion(struct completion *comp);
```
## When the other part of the code has decided that the work has been completed (the
transaction has been completed, in the case of DMA), it can wake up anyone (the code
that needs to access the DMA buffer) who is waiting by either calling complete(),
which will only wake one waiting process, or complete_all(), which will wake
everyone waiting for this to complete:
```c
void complete(struct completion *comp);
void complete_all(struct completion *comp);
```
16 Linux Kernel Concepts for Embedded Developers
## A typical usage scenario is as follows (this excerpt has been taken from the kernel
documentation):
CPU#1 CPU#2
```c
struct completion setup_done;
init_completion(&setup_done);
initialize_work(...,&setup_done,...);
```
/* run non-dependent code */ /* do some setup */
[...] [...]
```c
wait_for_completion(&setup_done); complete(setup_done);
```
The order in which wait_for_completion() and complete() are called does not
matter. As semaphores, the completions API is designed so that they will work properly,
even if complete() is called before wait_for_completion(). In such a case, the
waiter will simply continue immediately once all the dependencies have been satisfied.
Note that wait_for_completion() will invoke spin_lock_irq() and spin_
unlock_irq(), which, according to the Spinlocks section, are not recommended to be
used from within an interrupt handler or with disabled IRQs. This is because it would
result in spurious interrupts being enabled, which are hard to detect. Additionally,
by default, wait_for_completion() marks the task as uninterruptible (TASK_
UNINTERRUPTIBLE), making it unresponsive to any external signal (even kill). This may
block for a long time, depending on the nature of the activity it’s waiting for.
## You may need the wait not to be done in an uninterruptible state, or at least you may need
the wait being able to be interrupted either by any signal or only by signals that kill the
process. The kernel provides the following APIs:
• wait_for_completion_interruptible()
• wait_for_completion_interruptible_timeout()
• wait_for_completion_killable()
• wait_for_completion_killable_timeout()
## The kernel locking API and shared objects 17
_killable variants will mark the task as TASK_KILLABLE, thus only making it
responsive to signals that actually kill it, while _interruptible variants mark the
task as TASK_INTERRUPTIBLE, allowing it to be interrupted by any signal. _timeout
variants will, at most, wait for the specified timeout:
```c
int wait_for_completion_interruptible(struct completion *done)
```
long wait_for_completion_interruptible_timeout(
```c
struct completion *done, unsigned long timeout)
```
long wait_for_completion_killable(struct completion *done)
long wait_for_completion_killable_timeout(
```c
struct completion *done, unsigned long timeout)
```
Since wait_for_completion*() may sleep, it can only be used in this process
context. Because the interruptible, killable, or timeout variant may return
before the underlying job has run until completion, their return values should be checked
carefully so that you can adopt the right behavior. The killable and interruptible variants
```c
return -ERESTARTSYS if they’re interrupted and 0 if they’ve been completed. On the
```
other hand, the timeout variants will return -ERESTARTSYS if they’re interrupted,
0 if they’ve timed out, and the number of jiffies (at least 1) left until timeout if they’ve
completed before timeout. Please refer to kernel/sched/completion.c in the
kernel source for more on this, as well as more functions that will not be covered in this
book.
On the other hand, complete() and complete_all() never sleep and internally
call spin_lock_irqsave()/spin_unlock_irqrestore(), making completion
signaling, from an IRQ context, completely safe.
## Linux kernel wait queues
Wait queues are high-level mechanisms that are used to process block I/O, wait for
particular conditions to be true, wait for a given event to occur, or to sense data or
resource availability. To understand how they work, let’s have a look at the structure in
include/linux/wait.h:
```c
struct wait_queue_head {
spinlock_t lock;
struct list_head head;
};
```
18 Linux Kernel Concepts for Embedded Developers
## A wait queue is nothing but a list (in which processes are put to sleep so that they can
be awaken if some conditions are met) where there’s a spinlock to protect access to this
list. You can use a wait queue when more than one process wants to sleep and you’re
waiting for one or more events to occur so that it can be woke up. The head member is the
list of processes waiting for the event(s). Each process that wants to sleep while waiting for
the event to occur puts itself in this list before going to sleep. When a process is in the list,
it is called a wait queue entry. When the event occurs, one or more processes on the
list are woken up and moved off the list. We can declare and initialize a wait queue in
two ways. First, we can declare and initialize it statically using DECLARE_WAIT_QUEUE_
HEAD, as follows:
```c
DECLARE_WAIT_QUEUE_HEAD(my_event);
```
We can also do this dynamically using init_waitqueue_head():
```c
wait_queue_head_t my_event;
init_waitqueue_head(&my_event);
```
Any process that wants to sleep while waiting for my_event to occur can invoke either
```c
wait_event_interruptible() or wait_event(). Most of the time, the event is
```
just the fact that a resource has become available. Thus, it only makes sense for a process
to go to sleep after the availability of that resource has been checked. To make things easy
for you, these functions both take an expression in place of the second argument so that
the process is only put to sleep if the expression evaluates to false:
```c
wait_event(&my_event, (event_occurred == 1) );
```
/* or */
```c
wait_event_interruptible(&my_event, (event_occurred == 1) );
wait_event() and wait_event_interruptible() simply evaluate the
```
condition when it’s called. If the condition is false, the process is put into either a TASK_
UNINTERRUPTIBLE or a TASK_INTERRUPTIBLE (for the _interruptible variant)
state and removed from the running queue.
## There may be cases where you need the condition to not only be true, but to time out
after waiting a certain amount of time. You can address such cases using wait_event_
timeout(), whose prototype is as follows:
```c
wait_event_timeout(wq_head, condition, timeout)
```
## The kernel locking API and shared objects 19
This function has two behaviors, depending on the timeout having elapsed or not:
1. timeout has elapsed: The function returns 0 if the condition is evaluated to false
or 1 if it is evaluated to true.
2. timeout has not elapsed yet: The function returns the remaining time (in jiffies –
must at least be 1) if the condition is evaluated to true.
The time unit for the timeout is jiffies. So that you don’t have to bother with seconds
to jiffies conversion, you should use the msecs_to_jiffies() and usecs_
to_jiffies() helpers, which convert milliseconds or microseconds into jiffies,
respectively:
unsigned long msecs_to_jiffies(const unsigned int m)
unsigned long usecs_to_jiffies(const unsigned int u)
## After a change has been made to any variable that could mangle the result of the wait
condition, you must call the appropriate wake_up* family function. That being said, in
order to wake up a process sleeping on a wait queue, you should call either wake_up(),
wake_up_all(), wake_up_interruptible(), or wake_up_interruptible_
all(). Whenever you call any of these functions, the condition is reevaluated. If the
condition is true at this time, then a process (or all the processes for the _all() variant)
```c
in wait queue will be awakened, and its (their) state will be set to TASK_RUNNING;
```
otherwise (the condition is false), nothing will happen:
/* wakes up only one process from the wait queue. */
```c
wake_up(&my_event);
```
/* wakes up all the processes on the wait queue. */
```c
wake_up_all(&my_event);:
```
/* wakes up only one process from the wait queue that is in
* interruptible sleep.
*/
wake_up_interruptible(&my_event)
/* wakes up all the processes from the wait queue that
* are in interruptible sleep.
*/
```c
wake_up_interruptible_all(&my_event);
```
20 Linux Kernel Concepts for Embedded Developers
Since they can be interrupted by signals, you should check the return values of _
interruptible variants. A non-zero value means your sleep has been interrupted by
some sort of signal, so the driver should return ERESTARTSYS:
```c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
#include<linux/workqueue.h>
static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static int condition = 0;
```
/* declare a work queue*/
```c
static struct work_struct wrk;
static void work_handler(struct work_struct *work)
{
pr_info(“Waitqueue module handler %s\n”, __FUNCTION__);
msleep(5000);
pr_info(“Wake up the sleeping module\n”);
condition = 1;
wake_up_interruptible(&my_wq);
}
static int __init my_init(void)
{
pr_info(“Wait queue example\n”);
INIT_WORK(&wrk, work_handler);
schedule_work(&wrk);
pr_info(“Going to sleep %s\n”, __FUNCTION__);
wait_event_interruptible(my_wq, condition != 0);
pr_info(“woken up by the work job\n”);
return 0;
}
void my_exit(void)
{
pr_info(“waitqueue example cleanup\n”);
```
## The kernel locking API and shared objects 21
```c
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR(“John Madieu <john.madieu@labcsmart.com>”);
MODULE_LICENSE(“GPL”);
```
## In the preceding example, the current process (actually, this is insmod) will be put to
sleep in the wait queue for 5 seconds and woken up by the work handler. The output of
dmesg is as follows:
[342081.385491] Wait queue example
[342081.385505] Going to sleep my_init
[342081.385515] Waitqueue module handler work_handler
[342086.387017] Wake up the sleeping module
[342086.387096] woken up by the work job
[342092.912033] waitqueue example cleanup
You may have noticed that I did not check the return value of wait_event_
interruptible(). Sometimes (if not most of the time), this can lead to serious issues.
The following is a true story: I’ve had to intervene in a company to fix a bug where killing
(or sending a signal to) a user space task was making their kernel module crash the system
(panic and reboot – of course, the system was configured so that it rebooted on panic).
## The reason this happened was because there was a thread in this user process that did an
ioctl() on the char device exposed by their kernel module. This resulted in a call to
```c
wait_event_interruptible() in the kernel on a given flag, which meant there was
```
some data that needed to be processed in the kernel (the select() system call could not
be used).
So, what was their mistake? The signal that was sent to the process was making wait_
event_interruptible() return without the flag being set (which meant data was
still not available), and its code was not checking its return value, nor rechecking the flag
or performing a sanity check on the data that was supposed to be available. The data was
being accessed as if the flag had been set and it actually dereferenced an invalid pointer.
The solution could have been as simple as using the following code:
```c
if (wait_event_interruptible(...)){
pr_info(“catching a signal supposed make us crashing\n”);
```
/* handle this case and do not access data */
[….]
```c
} else {
```
22 Linux Kernel Concepts for Embedded Developers
/* accessing data and processing it */
[…]
```c
}
```
## However, for some reason (historical to their design), we had to make it uninterruptible,
which resulted in us using wait_event(). However, note that this function puts
the process into an exclusive wait (an uninterruptible sleep), which means it can’t be
interrupted by signals. It should only be used for critical tasks. Interruptible functions are
recommended in most situations.
## Now that we are familiar with the kernel locking APIs, we will look at various work
deferring mechanisms, all of which are heavily used when writing Linux device drivers.
## Work deferring mechanisms
Work deferring is a mechanism the Linux kernel offers. It allows you to defer work/a task
until the system’s workload allows it to run smoothly or after a given time has lapsed.
## Depending on the type of work, the deferred task can run either in a process context or
in an atomic context. It is common to using work deferring to complement the interrupt
handler in order to compensate for some of its limitations, some of which are as follows:
• The interrupt handler must be as fast as possible, meaning that only a critical task
should be performed in the handler so that the rest can be deferred later when the
system is less busy.
• In the interrupt context, we cannot use blocking calls. The sleeping task should be
scheduled in the process context.
## The deferring work mechanism allows us to perform the minimum possible work in the
interrupt handler (the so-called top-half, which runs in an interrupt context) and schedule
an asynchronous action (the so-called bottom-half, which may – but not always – run in
a user context) from the interrupt handler so that it can be run at a later time and execute
the rest of the operations. Nowadays, the concept of bottom-half is mostly assimilated to
deferred work running in a process context, since it was common to schedule work that
might sleep (unlike rare work running in an interrupt context, which cannot happen).
Linux now has three different implementations of this: softIRQs, tasklets, and work
queues. Let’s take a look at these:
• SoftIRQs: These are executed in an atomic context.
• Tasklets: These are also executed in an atomic context.
• Work queues: These run in a process context.
## Work deferring mechanisms 23
We will learn about each of them in detail in the next few sections.
## SoftIRQs
As the name suggests, softIRQ stands for software interrupt. Such a handler can preempt
all other tasks on the system except for hardware IRQ handlers, since they are executed
with IRQs enabled. SoftIRQs are intended to be used for high frequency threaded job
scheduling. Network and block devices are the only two subsystems in the kernel that
make direct use of softIRQs. Even though softIRQ handlers run with interrupts enabled,
they cannot sleep, and any shared data needs proper locking. The softIRQ API is defined
as kernel/softirq.c in the kernel source tree, and any drivers that wish to use this
API need to include <linux/interrupt.h>.
Note that you cannot dynamically register nor destroy softIRQs. They are statically
allocated at compile time. Moreover, the usage of softIRQs is restricted to statically
```c
compiled kernel code; they cannot be used with dynamically loadable modules. SoftIRQs
```
are represented by struct softirq_action structures defined in <linux/
interrupt.h>, as follows:
```c
struct softirq_action {
void (*action)(struct softirq_action *);
};
```
## This structure embeds a pointer to the function to be run when the softirq action is
raised. Thus, the prototype of your softIRQ handler should look as follows:
```c
void softirq_handler(struct softirq_action *h)
```
Running a softIRQ handler results in this action function being executed. It only has one
parameter: a pointer to the corresponding softirq_action structure. You can register
the softIRQ handler at runtime by means of the open_softirq() function:
```c
void open_softirq(int nr,
void (*action)(struct softirq_action *))
```
nr represents the softIRQ’s index, which is also considered as the softIRQ’s priority
(where 0 is the highest). action is a pointer to the softIRQ’s handler. Any possible
indexes are enumerated in the following enum:
```c
enum
{
```
HI_SOFTIRQ=0, /* High-priority tasklets */
24 Linux Kernel Concepts for Embedded Developers
TIMER_SOFTIRQ, /* Timers */
NET_TX_SOFTIRQ, /* Send network packets */
NET_RX_SOFTIRQ, /* Receive network packets */
BLOCK_SOFTIRQ, /* Block devices */
BLOCK_IOPOLL_SOFTIRQ, /* Block devices with I/O polling
blocked on other CPUs */
TASKLET_SOFTIRQ, /* Normal Priority tasklets */
SCHED_SOFTIRQ, /* Scheduler */
HRTIMER_SOFTIRQ, /* High-resolution timers */
RCU_SOFTIRQ, /* RCU locking */
NR_SOFTIRQS /* This only represent the number or
* softirqs type, 10 actually
*/
```c
};
```
## SoftIRQs with lower indexes (highest priority) run before those with higher indexes
(lowest priority). The names of all the available softIRQs in the kernel are listed in the
following array:
```c
const char * const softirq_to_name[NR_SOFTIRQS] = {
```
“HI”, “TIMER”, “NET_TX”, “NET_RX”, “BLOCK”, “BLOCK_IOPOLL”,
“TASKLET”, “SCHED”, “HRTIMER”, “RCU”
```c
};
```
It is easy to check the output of the /proc/softirqs virtual file, as follows:
~$ cat /proc/softirqs
## CPU0 CPU1 CPU2 CPU3
HI: 14026 89 491 104
TIMER: 862910 817640 816676 808172
NET_TX: 0 2 1 3
NET_RX: 1249 860 939 1184
BLOCK: 130 100 138 145
IRQ_POLL: 0 0 0 0
TASKLET: 55947 23 108 188
SCHED: 1192596 967411 882492 835607
HRTIMER: 0 0 0 0
RCU: 314100 302251 304380 298610
~$
## Work deferring mechanisms 25
A NR_SOFTIRQS entry array of struct softirq_action is declared in kernel/
softirq.c:
```c
static struct softirq_action softirq_vec[NR_SOFTIRQS] ;
```
Each entry in this array may contain one and only one softIRQ. As a consequence of this,
there can be a maximum of NR_SOFTIRQS (10 in v4.19, which is the last version at the
time of writing this) for registered softIRQs:
```c
void open_softirq(int nr,
void (*action)(struct softirq_action *))
{
softirq_vec[nr].action = action;
}
```
## A concrete example of this is the network subsystem, which registers softIRQs that it
needs (in net/core/dev.c) as follows:
```c
open_softirq(NET_TX_SOFTIRQ, net_tx_action);
open_softirq(NET_RX_SOFTIRQ, net_rx_action);
```
Before a registered softIRQ gets a chance to run, it should be activated/scheduled. To do
this, you must call raise_softirq() or raise_softirq_irqoff() (if interrupts
are already off):
```c
void __raise_softirq_irqoff(unsigned int nr)
void raise_softirq_irqoff(unsigned int nr)
void raise_softirq(unsigned int nr)
```
## The first function simply sets the appropriate bit in the per-CPU softIRQ bitmap (the
__softirq_pending field in the struct irq_cpustat_t data structure, which is
allocated per-CPU in kernel/softirq.c), as follows:
```c
irq_cpustat_t irq_stat[NR_CPUS] ____cacheline_aligned;
EXPORT_SYMBOL(irq_stat);
```
This allows it to run when the flag is checked. This function has been described here for
study purposes and should not be used directly.
26 Linux Kernel Concepts for Embedded Developers
raise_softirq_irqoff needs be called with interrupts disabled. First, it internally
calls __raise_softirq_irqoff(), as described previously, to activate the softIRQ.
## Then, it checks whether it has been called from within an interrupt (either hard or soft)
context by means of the in_interrupt() macro (which simply returns the value of
current_thread_info( )->preempt_count, where 0 means preemption is
enabled. This states that we are not in an interrupt context. A value greater than 0 means
we are in an interrupt context). If in_interrupt() > 0, this does nothing as we are in
an interrupt context. This is because softIRQ flags are checked on the exit path of any I/O
IRQ handler (asm_do_IRQ() for ARM or do_IRQ() for x86 platforms, which makes
a call to irq_exit()). Here, softIRQs run in an interrupt context. However, if in_
interrupt() == 0, then wakeup_softirqd() gets invoked. This is responsible for
waking the local CPU ksoftirqd thread up (it schedules it) to ensure the softIRQ runs
soon but in a process context this time.
raise_softirq first calls local_irq_save() (which disables interrupts on the
local processor after saving its current interrupt flags). It then calls raise_softirq_
irqoff(), as described previously, to schedule the softIRQ on the local CPU (remember,
this function must be invoked with IRQs disabled on the local CPU). Finally, it calls
local_irq_restore()to restore the previously saved interrupt flags.
There are a few things to remember about softIRQs:
• A softIRQ can never preempt another softIRQ. Only hardware interrupts can.
## SoftIRQs are executed at a high priority with scheduler preemption disabled, but
with IRQs enabled. This makes softIRQs suitable for the most time-critical and
important deferred processing on the system.
• While a handler runs on a CPU, other softIRQs on this CPU are disabled. SoftIRQs
can run concurrently, however. While a softIRQ is running, another softIRQ (even
the same one) can run on another processor. This is one of the main advantages
of softIRQs over hardIRQs, and is the reason why they are used in the networking
subsystem, which may require heavy CPU power.
• For locking between softIRQs (or even the same softIRQ as it may be running on a
different CPU), you should use spin_lock() and spin_unlock().
## Work deferring mechanisms 27
• SoftIRQs are mostly scheduled in the return paths of hardware interrupt handlers.
## SoftIRQs that are scheduled outside of the interrupt context will run in a process
context if they are still pending when the local ksoftirqd thread is given the
CPU. Their execution may be triggered in the following cases:
--By the local per-CPU timer interrupt (on SMP systems only, with CONFIG_SMP
enabled). See timer_tick(), update_process_times(), and run_local_
timers() for more.
--By making a call to the local_bh_enable() function (mostly invoked by the
network subsystem for handling packet receiving/transmitting softIRQs).
--On the exit path of any I/O IRQ handler (see do_IRQ, which makes a call to
irq_exit(), which in turn invokes invoke_softirq()).
--When the local ksoftirqd is given the CPU (that is, it’s been awakened).
The actual kernel function responsible for walking through the softIRQ’s pending bitmap
and running them is __do_softirq(), which is defined in kernel/softirq.c.
This function is always invoked with interrupts disabled on the local CPU. It performs the
following tasks:
1. Once invoked, the function saves the current per-CPU pending softIRQ’s bitmap in
a so-called pending variable and locally disables softIRQs by means of __local_
bh_disable_ip.
2. It then resets the current per-CPU pending bitmask (which has already been saved)
and then reenables interrupts (softIRQs run with interrupts enabled).
3. After this, it enters a while loop, checking for pending softIRQs in the saved
bitmap. If there is no softIRQ pending, nothing happens. Otherwise, it will execute
the handlers of each pending softIRQ, taking care to increment their executions'
statistics.
4. After all the pending handlers have been executed (we are out of the while loop),
__do_softirq() once again reads the per-CPU pending bitmask (required to
disable IRQs and save them into the same pending variable) in order to check if any
softIRQs were scheduled when it was in the while loop. If there are any pending
softIRQs, the whole process will restart (based on a goto loop), starting from step
2. This helps with handling, for example, softIRQs that have rescheduled themselves.
28 Linux Kernel Concepts for Embedded Developers
However, __do_softirq() will not repeat if one of the following conditions occurs:
• It has already repeated up to MAX_SOFTIRQ_RESTART times, which is set to 10
in kernel/softirq.c. This is actually the limit for the softIRQ processing loop,
not the upper bound of the previously described while loop.
• It has hogged the CPU more than MAX_SOFTIRQ_TIME, which is set to 2 ms
(msecs_to_jiffies(2)) in kernel/softirq.c, since this prevents the
scheduler from being enabled.
If one of the two situations occurs, __do_softirq() will break its loop and call
wakeup_softirqd()to wake the local ksoftirqd thread, which will later execute the
pending softIRQs in the process context. Since do_softirq is called at many points in
the kernel, it is likely that another invocation of __do_softirqs will handle pending
softIRQs before ksoftirqd has the chance to run.
## Note that softIRQs do not always run in an atomic context, but in this case, this is quite
specific. The next section explains how and why softIRQs may be executed in a process
context.
## A word on ksoftirqd
A ksoftirqd is a per-CPU kernel thread that’s raised in order to handle unserved
software interrupts. It is spawned early on in the kernel boot process, as stated in
kernel/softirq.c:
```c
static __init int spawn_ksoftirqd(void)
{
```
cpuhp_setup_state_nocalls(CPUHP_SOFTIRQ_DEAD,
“softirq:dead”, NULL,
```c
takeover_tasklets);
BUG_ON(smpboot_register_percpu_thread(&softirq_threads));
return 0;
}
early_initcall(spawn_ksoftirqd);
```
## Work deferring mechanisms 29
After running the top command, you will be able to see some ksoftirqd/n entries,
where n is the logical CPU index of the CPU running the ksoftirqd thread. Since the
ksoftirqds run in a process context, they are equal to classic processes/threads, and so
are their competing claims for the CPU. ksoftirqd hogging CPUs for a long time may
indicate a system under heavy load.
## Now that we have finished looking at our first work deferring mechanism in the Linux
kernel, we’ll discuss tasklets, which are an alternative (from an atomic context point of
view) to softIRQs, though the former are built using the latter.
## Tasklets
Tasklets are bottom halves built on top of the HI_SOFTIRQ and TASKLET_SOFTIRQ
softIRQs, with the only difference being that HI_SOFTIRQ-based tasklets run prior to the
TASKLET_SOFTIRQ-based ones. This simply means tasklets are softIRQs, so they follow
the same rules. Unlike softIRQs however, two of the same tasklets never run concurrently.
The tasklet API is quite basic and intuitive.
Tasklets are represented by the struct tasklet_struct structure, which is defined
in <linux/interrupt.h>. Each instance of this structure represents a unique tasklet:
```c
struct tasklet_struct {
struct tasklet_struct *next; /* next tasklet in the list */
unsigned long state; /* state of the tasklet,
```
* TASKLET_STATE_SCHED or
* TASKLET_STATE_RUN */
```c
atomic_t count; /* reference counter */
void (*func)(unsigned long); /* tasklet handler function */
unsigned long data; /* argument to the tasklet function */
};
```
## The func member is the handler of the tasklet that will be executed by the underlying
softIRQ. It is the equivalent of what action is to a softIRQ, with the same prototype and
the same argument meaning. data will be passed as its sole argument.
30 Linux Kernel Concepts for Embedded Developers
You can use the tasklet_init() function to dynamically allocate and initialize
tasklets at run-ime. For the static method, you can use the DECLARE_TASKLET macro.
## The option you choose will depend on your need (or requirement) to have a direct or
indirect reference to the tasklet. Using tasklet_init() would require embedding
the tasklet structure into a bigger and dynamically allocated object. An initialized
tasklet can be scheduled by default – you could say it is enabled. DECLARE_TASKLET_
## DISABLED is an alternative to declaring default-disabled tasklets, and this will require
the tasklet_enable() function to be invoked to make the tasklet schedulable.
Tasklets are scheduled (similar to raising a softIRQ) via the tasklet_schedule() and
tasklet_hi_schedule() functions. You can use tasklet_disable() to disable a
tasklet. This function disables the tasklet and only returns when the tasklet has terminated
its execution (assuming it was running). After this, the tasklet can still be scheduled, but
it will not run on the CPU until it is enabled again. The asynchronous variant known
as tasklet_disable_nosync() can be used too and returns immediately, even if
termination has not occurred. Moreover, a tasklet that has been disabled several times
should be enabled exactly the same number of times (this is allowed thanks to its
count field):
```c
DECLARE_TASKLET(name, func, data)
DECLARE_TASKLET_DISABLED(name, func, data);
tasklet_init(t, tasklet_handler, dev);
void tasklet_enable(struct tasklet_struct*);
void tasklet_disable(struct tasklet_struct *);
void tasklet_schedule(struct tasklet_struct *t);
void tasklet_hi_schedule(struct tasklet_struct *t);
```
The kernel maintains normal priority and high priority tasklets in two different queues.
## Queues are actually singly linked lists, and each CPU has its own queue pair (low and
high priority). Each processor has its own pair. tasklet_schedule() adds the tasklet
to the normal priority list, thereby scheduling the associated softIRQ with a TASKLET_
SOFTIRQ flag. With tasklet_hi_schedule(), the tasklet is added to the high
priority list, thereby scheduling the associated softIRQ with a HI_SOFTIRQ flag. Once
the tasklet has been scheduled, its TASKLET_STATE_SCHED flag is set, and the tasklet is
added to a queue. At the time of execution, the TASKLET_STATE_RUN flag is set and the
TASKLET_STATE_SCHED state is removed, thus allowing the tasklet to be rescheduled
during its execution, either by the tasklet itself or from within an interrupt handler.
## Work deferring mechanisms 31
High-priority tasklets are meant to be used for soft interrupt handlers with low latency
requirements. Calling tasklet_schedule() on a tasklet that’s already been scheduled,
but whose execution has not started, will do nothing, resulting in the tasklet being
executed only once. A tasklet can reschedule itself, which means you can safely call
tasklet_schedule() in a tasklet. High-priority tasklets are always executed before
```c
normal ones and should be used carefully; otherwise, you may increase system latency.
```
Stopping a tasklet is as simple as calling tasklet_kill(), which will prevent the
tasklet from running again or waiting for it to complete before killing it if the tasklet is
currently scheduled to run. If the tasklet reschedules itself, you should prevent the tasklet
from rescheduling itself prior to calling this function:
```c
void tasklet_kill(struct tasklet_struct *t);
```
That being said, let’s take a look at the following example of tasklet code usage:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h> /* for tasklets API */
```
char tasklet_data[] =
```c
“We use a string; but it could be pointer to a structure”;
```
/* Tasklet handler, that just prints the data */
```c
void tasklet_work(unsigned long data)
{
printk(“%s\n”, (char *)data);
}
static DECLARE_TASKLET(my_tasklet, tasklet_function,
(unsigned long) tasklet_data);
static int __init my_init(void)
{
tasklet_schedule(&my_tasklet);
return 0;
}
void my_exit(void)
{
tasklet_kill(&my_tasklet);
}
module_init(my_init);
```
32 Linux Kernel Concepts for Embedded Developers
```c
module_exit(my_exit);
MODULE_AUTHOR(“John Madieu <john.madieu@gmail.com>”);
MODULE_LICENSE(“GPL”);
```
In the preceding code, we statically declared our my_tasklet tasklet and the function
that’s supposed to be invoked when this tasklet is scheduled, along with the data that will
be given as an argument to this function.
## Important note
Because the same tasklet never runs concurrently, the locking case between a
tasklet and itself doesn’t need to be addressed. However, any data that’s shared
between two tasklets should be protected with spin_lock() and spin_
unlock(). Remember, tasklets are implemented on top of softIRQs.
## Workqueues
In the previous section, we dealt with tasklets, which are atomically deferred mechanisms.
## Apart from atomic mechanisms, there are cases where we may want to sleep in the
deferred task. Workqueues allow this.
## A workqueue is an asynchronous work deferring mechanism that is widely used across
kernels, allowing them to run a dedicated function asynchronously in a process execution
context. This makes them suitable for long-running and lengthy tasks or work that needs
to sleep, thus improving the user experience. At the core of the workqueue subsystem,
there are two data structures that can explain the concept behind this mechanism:
• The work to be deferred (that is, the work item) is represented in the kernel by
instances of struct work_struct, which indicates the handler function to
be run. Typically, this structure is the first element of a user’s structure of the
work definition. If you need a delay before the work can be run after it has been
submitted to the workqueue, the kernel provides struct delayed_work
instead. A work item is a basic structure that holds a pointer to the function that is
to be executed asynchronously. To summarize, we can enumerate two types of work
item structures:
--The struct work_struct structure, which schedules a task to be run at a
later time (as soon as possible when the system allows it).
--The struct delayed_work structure, which schedules a task to be run after at
least a given time interval.
## Work deferring mechanisms 33
• The workqueue itself, which is represented by a struct workqueue_struct.
This is the structure that work is placed on. It is a queue of work items.
Apart from these data structures, there are two generic terms you should be familiar with:
• Worker threads, which are dedicated threads that execute the functions off the
queue, one by one, one after the other.
• Workerpools are a collection of worker threads (a thread pool) that are used to
manage worker threads.
## The first step in using work queues consists of creating a work item, represented by
```c
struct work_struct or struct delayed_work for the delayed variant, that’s
```
defined in linux/workqueue.h. The kernel provides either the DECLARE_WORK
macro for statically declaring and initializing a work structure, or the INIT_WORK
macro for doing the same by dynamically. If you need delayed work, you can use the
```c
INIT_DELAYED_WORK macro for dynamic allocation and initialization, or DECLARE_
```
DELAYED_WORK for the static option:
```c
DECLARE_WORK(name, function)
DECLARE_DELAYED_WORK(name, function)
INIT_WORK(work, func);
INIT_DELAYED_WORK(work, func);
```
The following code shows what our work item structure looks like:
```c
struct work_struct {
atomic_long_t data;
struct list_head entry;
work_func_t func;
```
#ifdef CONFIG_LOCKDEP
```c
struct lockdep_map lockdep_map;
```
#endif
```c
};
struct delayed_work {
struct work_struct work;
struct timer_list timer;
```
/* target workqueue and CPU ->timer uses to queue ->work */
```c
struct workqueue_struct *wq;
```
34 Linux Kernel Concepts for Embedded Developers
```c
int cpu;
};
```
The func field, which is of the work_func_t type, tells us a bit more about the header
of a work function:
```c
typedef void (*work_func_t)(struct work_struct *work);
```
work is an input parameter that corresponds to the work structure associated with your
work. If you’ve submitted a delayed work, this would correspond to the delayed_work.
work field. Here, it will be necessary to use the to_delayed_work() function to get
the underlying delayed work structure:
```c
struct delayed_work *to_delayed_work(struct work_struct *work)
```
## Workqueues let your driver create a kernel thread, called a worker thread, to handle
deferred work. A new workqueue can be created with these functions:
```c
struct workqueue_struct *create_workqueue(const char *name
```
name)
```c
struct workqueue_struct
```
*create_singlethread_workqueue(const char *name)
create_workqueue() creates a dedicated thread (a worker) per CPU on the system,
which is probably not a good idea. On an 8-core system, this will result in 8 kernel
threads being created to run work that’s been submitted to your workqueue. In most
cases, a single system-wide kernel thread should be enough. In this case, you should use
create_singlethread_workqueue() instead, which creates, as its name suggests,
```c
a single threaded workqueue; that is, with one worker thread system-wide. Either
```
normal or delayed work can be enqueued on the same queue. To schedule works on your
created workqueue, you can use either queue_work() or queue_delayed_work(),
depending on the nature of the work:
bool queue_work(struct workqueue_struct *wq,
```c
struct work_struct *work)
```
bool queue_delayed_work(struct workqueue_struct *wq,
```c
struct delayed_work *dwork,
```
unsigned long delay)
## Work deferring mechanisms 35
These functions return false if the work was already on a queue and true otherwise.
queue_dalayed_work() can be used to plan (delayed) work for execution with
a given delay. The time unit for the delay is jiffies. If you don’t want to bother with
seconds-to-jiffies conversion, you can use the msecs_to_jiffies() and usecs_
to_jiffies() helpers, which convert milliseconds or microseconds into jiffies,
respectively:
unsigned long msecs_to_jiffies(const unsigned int m)
unsigned long usecs_to_jiffies(const unsigned int u)
The following example uses 200 ms as a delay:
schedule_delayed_work(&drvdata->tx_work, usecs_to_
```c
jiffies(200));
```
Submitted work items can be canceled by calling either cancel_delayed_work(),
cancel_delayed_work_sync(), or cancel_work_sync():
bool cancel_work_sync(struct work_struct *work)
bool cancel_delayed_work(struct delayed_work *dwork)
bool cancel_delayed_work_sync(struct delayed_work *dwork)
The following describes what these functions do:
• cancel_work_sync() synchronously cancels the given workqueue entry.
In other words, it cancels work and waits for its execution to finish. The kernel
guarantees that work won’t be pending or executing on any CPU when it’s return
from this function, even if the work migrates to another workqueue or requeues
itself. It returns true if work was pending, or false otherwise.
• cancel_delayed_work() asynchronously cancels a pending workqueue entry
(a delayed one). It returns true (a non-zero value) if dwork was pending and
canceled and false if it wasn’t pending, probably because it is actually running,
and thus might still be running after cancel_delayed_work(). To ensure
the work really ran to its end, you may want to use flush_workqueue(),
which flushes every work item in the given queue, or cancel_delayed_work_
sync(), which is the synchronous version of cancel_delayed_work().
36 Linux Kernel Concepts for Embedded Developers
To wait for all the work items to finish, you can call flush_workqueue(). When you
are done with a workqueue, you should destroy it with destroy_workqueue(). Both
these options can be seen in the following code:
```c
void flush_workqueue(struct worksqueue_struct * queue);
void destroy_workqueue(structure workqueque_struct *queue);
```
While you’re waiting for any pending work to execute, the _sync variant functions sleep,
which means they can only be called from a process context.
## The kernel shared queue
In most situations, your code does not necessarily need to have the performance of its
own dedicated set of threads, and because create_workqueue() creates one worker
thread for each CPU, it may be a bad idea to use it on very large multi-CPU systems.
## In this situation, you may want to use the kernel shared queue, which has its own set of
kernel threads preallocated (early during boot, via the workqueue_init_early()
function) for running works.
This global kernel workqueue is the so-called system_wq, and is defined in kernel/
workqueue.c. There is one instance per CPU, with each backed by a dedicated thread
named events/n, where n is the processor number that the thread is bound to. You can
queue work to the system’s default workqueue using one of the following functions:
```c
int schedule_work(struct work_struct *work);
int schedule_delayed_work(struct delayed_work *dwork,
unsigned long delay);
int schedule_work_on(int cpu, struct work_struct *work);
int schedule_delayed_work_on(int cpu,
struct delayed_work *dwork,
unsigned long delay);
```
schedule_work() immediately schedules the work that will be executed as soon as
possible after the worker thread on the current processor wakes up. With schedule_
delayed_work(), the work will be put in the queue in the future, after the delay timer
has ticked. The _on variants are used to schedule the work on a specific CPU (this does
not need to be the current one). Each of these function queues work on the system’s
shared workqueue, system_wq, which is defined in kernel/workqueue.c:
```c
struct workqueue_struct *system_wq __read_mostly;
EXPORT_SYMBOL(system_wq);
```
## Work deferring mechanisms 37
To flush the kernel-global workqueue – that is, to ensure the given batch of work is
completed – you can use flush_scheduled_work():
```c
void flush_scheduled_work(void);
```
flush_scheduled_work() is a wrapper that calls flush_workqueue() on
system_wq. Note that there may be work in system_wq that you have not submitted
and have no control over. Due to this, flushing this workqueue entirely is overkill. It is
recommended to use cancel_delayed_work_sync() or cancel_work_sync()
instead.
## Tip
Unless you have a strong reason to create a dedicated thread, the default
(kernel-global) thread is preferred.
Workqueues – a new generation
The original (now legacy) workqueue implementation used two kinds of workqueues:
those with a single thread system-wide, and those with a thread per-CPU. However, due
to the increasing number of CPUs, this led to some limitations:
• On very large systems, the kernel could run out of process IDs (defaulted to 32k)
just at boot, before the init was started.
• Multi-threaded workqueues provided poor concurrency management as their
threads competed for the CPU with other threads on the system. Since there
```c
were more CPU contenders, this introduced some overhead; that is, more context
```
switches than necessary.
• The consumption of much more resources than what was really needed.
## Moreover, subsystems that needed a dynamic or fine-grained level of concurrency had
to implement their own thread pools. As a result of this, a new workqueue API has
been designed and the legacy workqueue API (create_workqueue(), create_
singlethread_workqueue(), and create_freezable_workqueue()) has been
scheduled to be removed. However, these are actually wrappers around the new ones – the
so-called concurrency-managed workqueues. This is done using per-CPU worker pools
that are shared by all the workqueues in order to automatically provide a dynamic and
flexible level of concurrency, thus abstracting such details for API users.
38 Linux Kernel Concepts for Embedded Developers
## Concurrency-managed workqueues
The concurrency-managed workqueue is an upgrade of the workqueue API. Using this
new API implies that you must choose between two macros to create the workqueue:
alloc_workqueue() and alloc_ordered_workqueue(). These macros both
allocate a workqueue and return a pointer to it on success, and NULL on failure. The
returned workqueue can be freed using the destroy_workqueue() function:
```c
#define alloc_workqueue(fmt, flags, max_active, args...)
#define alloc_ordered_workqueue(fmt, flags, args...)
void destroy_workqueue(struct workqueue_struct *wq)
```
fmt is the printf format for the name of the workqueue, while args... are arguments
for fmt. destroy_workqueue() is to be called on the workqueue once you are done
with it. All work that’s currently pending will be completed first, before the kernel destroys
the workqueue. alloc_workqueue() creates a workqueue based on max_active,
which defines the concurrency level by limiting the number of work (tasks) that can be
executing (workers in a runnable sate) simultaneously from this workqueue on any given
CPU. For example, a max_active of 5 would mean that, at most, five work items on
this workqueue can be executing at the same time per CPU. On the other hand, alloc_
ordered_workqueue() creates a workqueue that processes each work item one by one
in the queued order (that is, FIFO order).
flags controls how and when work items are queued, assigned execution resources,
scheduled, and executed. Various flags are used in this new API. Let’s take a look at some
of them:
• WQ_UNBOUND: Legacy workqueues had a worker thread per CPU and were
designed to run tasks on the CPU where they were submitted. The kernel scheduler
had no choice but to always schedule a worker on the CPU that it was defined on.
## With this approach, even a single workqueue could prevent a CPU from idling and
being turned off, which leads to increased power consumption or poor scheduling
policies. WQ_UNBOUND turns off this behavior. Work is not bound to a CPU
anymore, hence the name unbound workqueues. There is no more locality, and the
scheduler can reschedule the worker on any CPU as it sees fit. The scheduler has
the last word now and can balance CPU load, especially for long and sometimes
CPU-intensive work.
## Work deferring mechanisms 39
• WQ_MEM_RECLAIM: This flag is to be set for workqueues that need to guarantee
forward progress during a memory reclaim path (when free memory is running
```c
dangerously low; here, the system is under memory pressure. In this case, GFP_
```
KERNEL allocations may block and deadlock the entire workqueue). The workqueue
is then guaranteed to have a ready-to-use worker thread, a so-called rescuer thread
reserved for it, regardless of memory pressure, so that it can progress. One rescuer
thread is allocated for each workqueue that has this flag set.
Let’s consider a situation where we have three work items (w1, w2, and w3) in our
workqueue, W. w1 does some work and then waits for w3 to complete (let’s say it depends
on the computation result of w3). Afterward, w2 (which is independent of the others)
does some kmalloc() allocation (GFP_KERNEL). Now, it seems like there is not enough
memory. While w2 is blocked, it still occupies the workqueue of W. This results in w3 not
being able to run, despite the fact that there is no dependency between w2 and w3. Since
there is not enough memory available, there is no way to allocate a new thread to run w3.
## A pre-allocated thread would definitely solve this problem, not by magically allocating
the memory for w2, but by running w3 so that w1 can continue its job, and so on. w2
will continue its progression as soon as possible, when there is enough available memory
to allocate. This pre-allocated thread is the so-called rescuer thread. You must set this
WQ_MEM_RECLAIM flag if you think the workqueue might be used in the memory reclaim
path. This flag replaces the old WQ_RESCUER flag as of the following commit: https:/
/git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/
commit/?id=493008a8e475771a2126e0ce95a73e35b371d277.
40 Linux Kernel Concepts for Embedded Developers
• WQ_FREEZABLE: This flag is used for power management purposes. A workqueue
with this flag set will be frozen when the system is suspended or hibernates. On
the freezing path, all current work(s) of the worker(s) will be processed. When the
freeze is complete, no new work items will be executed until the system is unfrozen.
## Filesystem-related workqueue(s) may use this flag to ensure that modifications that
are made to files are pushed to disk or create the hibernation image on the freezing
path and that no modifications are made on-disk after the hibernation image
has been created. In this situation, non-freezable items may do things differently
that could lead to filesystem corruption. As an example, all of the XFS internal
workqueues have this flag set (see fs/xfs/xfs_super.c) to ensure no further
changes are made on disk once the freezer infrastructure freezes the kernel threads
and creates the hibernation image. You should not set this flag if your workqueue
can run tasks as part of the hibernation/suspend/resume process of the system.
## More information on this topic can be found in Documentation/power/
freezing-of-tasks.txt, as well as by taking a look at the kernel’s internal
freeze_workqueues_begin() and thaw_workqueues() functions.
• WQ_HIGHPRI: Tasks that have this flag set run immediately and do not wait for the
CPU to become available. This flag is used for workqueues that queue work items
that require high priority for execution. Such workqueues have worker threads with
a high priority level (a lower nice value).
## In the early days of the CMWQ, high-priority work items were just queued at
the head of a global normal priority worklist so that they could immediately run.
## Nowadays, there is no interaction between normal priority and high-priority
workqueues as each has its own worklist and its own worker pool. The work items
of a high-priority workqueue are queued to the high-priority worker pool of the
target CPU. Tasks in this workqueue should not block much. Use this flag if you do
not want your work item competing for CPU with normal or lower-priority tasks.
Crypto and Block subsystems use this, for example.
• WQ_CPU_INTENSIVE: Work items that are part of a CPU-intensive workqueue
may burn a lot of CPU cycles and will not participate in the workqueue’s
concurrency management. Instead, their execution is regulated by the system
scheduler, just like any other task. This makes this flag useful for bound work
items that may hog CPU cycles. Though their execution is regulated by the system
scheduler, the start of their execution is still regulated by concurrency management,
and runnable non-CPU-intensive work items can delay the execution of
CPU-intensive work items. Actually, the crypto and dm-crypt subsystems use such
workqueues. To prevent such tasks from delaying the execution of other non-CPUintensive work items, they will not be taken into account when the workqueue code
determines whether the CPU is available.
## Work deferring mechanisms 41
In order to be compliant with the old workqueue API, the following mappings are made to
keep this API compatible with the original one:
• create_workqueue(name) is mapped to alloc_workqueue(name,
WQ_MEM_RECLAIM, 1).
• create_singlethread_workqueue(name) is mapped to
alloc_ordered_workqueue(name, WQ_MEM_RECLAIM).
• create_freezable_workqueue(name) is mapped to
alloc_workqueue(name,WQ_FREEZABLE | WQ_UNBOUND|WQ_MEM_
RECLAIM, 1).
To summarize, alloc_ordered_workqueue() actually replaces create_
freezable_workqueue() and create_singlethread_workqueue() (as per
the following commit: https://git.kernel.org/pub/scm/linux/kernel/
git/next/linux-next.git/commit/?id=81dcaf6516d8). Workqueues
allocated with alloc_ordered_workqueue() are unbound and have max_active
set to 1.
## When it comes to scheduled items in a workqueue, the work items that have been queued
to a specific CPU using queue_work_on() will execute on that CPU. Work items that
have been queued via queue_work() will prefer the queueing CPU, though this locality
is not guaranteed.
## Important Note
Note that schedule_work() is a wrapper that calls queue_work() on
the system workqueue (system_wq), while schedule_work_on() is a
wrapper around queue_work_on(). Also, keep in mind that system_
```c
wq = alloc_workqueue(“events”, 0, 0);. Take a look at the
```
workqueue_init_early() function in kernel/workqueue.c in
the kernel sources to see how other system-wide workqueues are created.
Memory reclaim is a Linux kernel mechanism on the memory allocation path.
## This consists of allocating memory after throwing the current content of that
memory somewhere else.
## With that, we have finished looking at workqueues and the concurrency-managed ones in
particular. Next, we’ll introduce Linux kernel interrupt management, which is where most
of the previous mechanisms will be solicited.
42 Linux Kernel Concepts for Embedded Developers
## Linux kernel interrupt management
Apart from servicing processes and user requests, another job of the Linux kernel is
managing and speaking with hardware. This is either from the CPU to the device or
from the device to the CPU. This is achieved by means of interrupts. An interrupt is a
signal that’s sent to the processor by an external hardware device requesting immediate
attention. Prior to an interrupt being visible to the CPU, this interrupt should be enabled
by the interrupt controller, which is a device on its own, and whose main job consists of
routing interrupts to CPUs.
An interrupt may have five states:
• Active: An interrupt that has been acknowledged by a processing element (PE) and
is being handled. While being handled, another assertion of the same interrupt is
not presented as an interrupt to a processing element, until the initial interrupt is no
longer active.
• Pending (asserted): An interrupt that is recognized as asserted in hardware, or
generated by software, and is waiting to be handled by the target PE. It is a common
behavior for most hardware devices not to generate other interrupts until their
“interrupt pending” bit has been cleared. A disabled interrupt can’t be pending as it
is never asserted, and it is immediately dropped by the interrupt controller.
• Active and pending: An interrupt that is active from one assertion of the interrupt
and is pending from a subsequent assertion.
• Inactive: An interrupt that is not active or pending. Deactivation clears the active
state of the interrupt, and thereby allows the interrupt, when it is pending, to be
taken again.
• Disabled/Deactivated: This is unknown to the CPU and not even seen by the
interrupt controller. This will never be asserted. Disabled interrupts are lost.
## Important note
There are interrupt controllers where disabling an interrupt means masking
that interrupt, or vice versa. In the remainder of this book, we will consider
disabling to be the same as masking, though this is not always true.
## Linux kernel interrupt management 43
Upon reset, the processor disables all the interrupts until they are enabled again by the
initialization code (this is the job of the Linux kernel in our case). The interrupts are
enabled/disabled by setting/clearing the bits in the processor status/control registers.
## Upon an interrupt assertion (an interrupt occurred), the processor will check whether the
interrupts are masked or not and will do nothing if they are masked. Once unmasked, the
processor will pick one pending interrupt, if any (the order does not matter since it will
do this for each pending interrupt until they are all serviced), and will execute a specially
purposed function called the Interrupt Service Routine (ISR) that is associated with
this interrupt. This ISR must be registered by the code (that is, our device driver, which
relies on the kernel irq core code) at a special location called the vector table. Right
before the processor starts executing this ISR, it does some context saving (including the
unmasked status of interrupts) and then masks the interrupts on the local CPU (interrupts
can be asserted and will be serviced once unmasked). Once the ISR is running, we can say
that the interrupt is being serviced.
The following is the complete IRQ handling flow on ARM Linux. This happens when an
interrupt occurs and the interrupts are enabled in the PSR:
1. The ARM core will disable further interrupts occurring on the local CPU.
2. The ARM core will then put the Current Program Status Register (CPSR) in the
## Saved Program Status Register (SPSR), put the current Program Counter (PC) in
the Link Register (LR), and then switch to IRQ mode.
3. Finally, the ARM processor will refer to the vector table and jumps to the exception
handler. In our case, it jumps to the exception handler of IRQ, which in the Linux
kernel corresponds to the vector_stub macro defined in arch/arm/kernel/
entry-armv.S.
These three steps are done by the ARM processor itself. Now, the kernel jumps into
action:
4. The vector_stub macro checks from what processor mode we used to get here
```c
– either kernel mode or user mode – and determines the macro to call accordingly;
```
either __irq_user or __irq_svc.
5. __irq_svc() will save the registers (from r0 to r12) on the kernel stack and
then call the irq_handler() macro, which either calls handle_arch_irq()
(present in arch/arm/include/asm/entry-macro-multi.S) if CONFIG_
MULTI_IRQ_HANDLER is defined, or arch_irq_handler_default()
otherwise, with handle_arch_irq being a global pointer to the function
that’s set in arch/arm/kernel/setup.c (from within the setup_arch()
function).
44 Linux Kernel Concepts for Embedded Developers
6. Now, we need to identify the hardware-IRQ number, which is what asm_do_
IRQ() does. It then calls handle_IRQ() on that hardware-IRQ, which in turn
calls __handle_domain_irq(), which will translate the hardware-irq into its
corresponding Linux IRQ number (irq = irq_find_mapping(domain,
hwirq)) and call generic_handle_irq() on the decoded Linux IRQ
(generic_handle_irq(irq)).
7. generic_handle_irq() will look for the IRQ descriptor structure (Linux’s view
of an interrupt) that corresponds to the decoded Linux IRQ (struct irq_desc
*desc = irq_to_desc(irq)) and calling generic_handle_irq_desc()
on this descriptor), which will result in desc->handle_irq(desc).
desc->handle_irq corresponding to the high-level IRQ handler that was set
using irq_set_chip_and_handler() during the mapping of this IRQ.
8. desc->handle_irq() may result in a call to handle_level_irq(),
handle_simple_irq(), handle_edge_irq(), and so on.
9. The high-level IRQ handler calls our ISR.
10. Once the ISR has been completed, irq_svc will return and restore the processor
state by restoring registers (r0-r12), the PC, and the CSPR.
## Important note
Going back to step 1, during an interrupt, the ARM core disables further IRQs
on the local CPU. It is worth mentioning that in the earlier Linux kernel days,
there were two families of interrupt handlers: those running with interrupts
disabled (that is, with the old IRQF_DISABLED flag set) and those running
with interrupts enabled: they were then interruptible. The former were called
fast handlers, while the latter were called slow handlers. For the latter,
interrupts were actually reenabled by the kernel prior to invoking the handler.
## Since the interrupt context has a really small stack size compared to the process
stack, it makes no sense that we may run into a stack overflow if we are in an
interrupt context (running a given IRQ handler) while other interrupts keep
occurring, even the one being serviced. This is confirmed by the commit at
https://git.kernel.org/pub/scm/linux/kernel/git/
next/linux-next.git/commit/?id=e58aa3d2d0cc, which
deprecated the fact of running interrupt handlers with IRQs enabled. As of this
patch, IRQs remain disabled (left untouched after ARM core disabled them
on the local CPU) during the execution of an IRQ handler. Additionally, the
aforementioned flags have been entirely removed by the commit at https:/
/git.kernel.org/pub/scm/linux/kernel/git/next/
linux-next.git/commit/?id=d8bf368d0631, since Linux v4.1.
## Linux kernel interrupt management 45
Designing an interrupt handler
Now that we’re familiar with the concept of bottom halves and deferring mechanisms, the
time for us to implement interrupt handlers has come. In this section, we’ll take care of
some specifics. Nowadays, the fact that interrupt handlers run with interrupts disabled
(on the local CPU) means that we need to respect certain constraints in the ISR design:
• Execution time: Since IRQ handlers run with interrupts disabled on the local CPU,
the code must be as short and as small as possible, as well as fast enough to ensure
the previously disabled CPU-local interrupts are reenabled quickly in so that other
IRQs are not missed. Time-consuming IRQ handlers may considerably alter the
real-time properties of the system and slow it down.
• Execution context: Since interrupt handlers are executed in an atomic context,
sleeping (or any other mechanism that may sleep, such as mutexes, copying data
from kernel to user space or vice versa, and so on) is forbidden. Any part of the
code that requires or involves sleeping must be deferred into another, safer context
(that is, a process context).
An IRQ handler needs to be given two arguments: the interrupt line to install the
handler for, and a unique device identifier of the peripheral (mostly used as a context
```c
data structure; that is, the pointer to the per-device or private structure of the associated
```
hardware device):
```c
typedef irqreturn_t (*irq_handler_t)(int, void *);
```
## The device driver that wants to enable a given interrupt and register an ISR for it should
call request_irq(), which is declared in <linux/interrupt.h>. This must be
included in the driver code:
```c
int request_irq(unsigned int irq,
```
irq_handler_t handler,
unsigned long flags,
const char *name,
```c
void *dev)
```
46 Linux Kernel Concepts for Embedded Developers
## While the aforementioned API would require the caller to free the IRQ when it is no
longer needed (that is, on driver detach), you can use the device managed variant, devm_
request_irq(), which contains internal logic that allows it to take care of releasing the
IRQ line automatically. It has the following prototype:
```c
int devm_request_irq(struct device *dev, unsigned int irq,
```
irq_handler_t handler,
unsigned long flags,
const char *name, void *dev)
## Except for the extra dev parameter (which is the device that requires the interrupt), both
devm_request_irq() and request_irq() expect the following arguments:
• irq, which is the interrupt line (that is, the interrupt number of the issuing device).
## Prior to validating the request, the kernel will make sure the requested interrupt
is valid and that it is not already assigned to another device, unless both devices
request that this IRQ line needs to be shared (with the help of flags).
• handler, which is a function pointer to the interrupt handler.
• flags, which represents the interrupt flags.
• name, an ASCII string representing the name of the device generating or claiming
this interrupt.
• dev should be unique to each registered handler. This cannot be NULL for shared
IRQs since it is used to identify the device via the kernel IRQ core. The most
common way of using it is to provide a pointer to the device structure or a pointer
to any per-device (that’s potentially useful to the handler) data structure. This is
because when an interrupt occurs, both the interrupt line (irq) and this parameter
will be passed to the registered handler, which can use this data as context data for
further processing.
flags mangle the state or behavior of the IRQ line or its handler by means of the
following masks, which can be OR’ed to form the final desired bit mask according to your
needs:
```c
#define IRQF_TRIGGER_RISING 0x00000001
#define IRQF_TRIGGER_FALLING 0x00000002
#define IRQF_TRIGGER_HIGH 0x00000004
#define IRQF_TRIGGER_LOW 0x00000008
#define IRQF_SHARED 0x00000080
```
## Linux kernel interrupt management 47
```c
#define IRQF_PROBE_SHARED 0x00000100
#define IRQF_NOBALANCING 0x00000800
#define IRQF_IRQPOLL 0x00001000
#define IRQF_ONESHOT 0x00002000
#define IRQF_NO_SUSPEND 0x00004000
#define IRQF_FORCE_RESUME 0x00008000
#define IRQF_NO_THREAD 0x00010000
#define IRQF_EARLY_RESUME 0x00020000
#define IRQF_COND_SUSPEND 0x00040000
```
Note that flags can also be zero. Let’s take a look at some important flags. I’ll leave the rest
for you to explore in include/linux/interrupt.h:
• IRQF_TRIGGER_HIGH and IRQF_TRIGGER_LOW flags are to be used for levelsensitive interrupts. The former is for interrupts triggered at high level and the latter
is for the low-level triggered interrupts. Level-sensitive interrupts are triggered as
long as the physical interrupt signal is high. If the interrupt source is not cleared by
the end of its interrupt handler in the kernel, the operating system will repeatedly
call that kernel interrupt handler, which may lead platform to hang. In other words,
when the handler services the interrupt and returns, if the IRQ line is still asserted,
the CPU will signal the interrupt again immediately. To prevent such a situation,
the interrupt must be acknowledged (that is, cleared or de-asserted) by the kernel
interrupt handler immediately when it is received.
## However, those flags are safe with regard to interrupt sharing because if several
devices pull the line active, an interrupt will be signaled (assuming the IRQ is
enabled or as soon as it becomes so) until all drivers have serviced their devices.
## The only drawback is that it may lead to lockup if a driver fails to clear its interrupt
source.
• IRQF_TRIGGER_RISING and IRQF_TRIGGER_FALLING concern edgetriggered interrupts, rising and falling edges respectively. Such interrupts are
signaled when the line changes from inactive to active state, but only once. To get a
new request the line must go back to inactive and then to active again. Most of the
time, no special action is required in software in order to acknowledge this type of
interrupt.
48 Linux Kernel Concepts for Embedded Developers
## When using edge-triggered interrupts however, interrupts may be lost, especially
in the context of a shared interrupt line: if one device pulls the line active for too
long a time, when another device pulls the line active, no edge will be generated, the
second request will not be seen by the processor and then will be ignored. With a
shared edge-triggered interrupts, if a hardware does not de-assert the IRQ line, no
other interrupt will be notified for either shared device.
## Important note
As a quick reminder, you can just remember that level triggered interrupts
signal a state, while edge triggered ones signal an event.
Moreover, when requesting an interrupt without specifying an IRQF_
## TRIGGER flag, the setting should be assumed to be as already configured,
which may be as per machine or firmware initialization. In such cases, you
can refer to the device tree (if specified in there) for example to see what this
assumed configuration is.
• IRQF_SHARED: This allows the interrupt line to be shared among several devices.
## However, each device driver that needs to share the given interrupt line must set
```c
this flag; otherwise, the registration will fail.
```
• IRQF_NOBALANCING: This excludes the interrupt from IRQ balancing, which is
a mechanism that consists of distributing/relocating interrupts across CPUs, with
the goal of increasing performance. This prevents the CPU affinity of this IRQ from
being changed. This flag can be used to provide a flexible setup for clocksources
in order to prevent the event from being misattributed to the wrong core. This
misattribution may result in the IRQ being disabled because if the CPU handling
the interrupt is not the one that triggered it, the handler will return IRQ_NONE.
This flag is only meaningful on multicore systems.
• IRQF_IRQPOLL: This flag allows the irqpoll mechanism to be used, which fixes
interrupt problems. This means that this handler should be added to the list of
known interrupt handlers that can be looked for when a given interrupt is not
handled.
• IRQF_ONESHOT: Normally, the actual interrupt line being serviced is enabled after
its hard-IRQ handler completes, whether it awakes a threaded handler or not. This
flag keeps the interrupt line disabled after the hard-IRQ handler completes. This
flag must be set on threaded interrupts (we will discuss this later) for which the
interrupt line must remain disabled until the threaded handler has completed. After
this, it will be enabled.
## Linux kernel interrupt management 49
• IRQF_NO_SUSPEND: This does not disable the IRQ during system hibernation/
suspension. This means that the interrupt is able to save the system from a
suspended state. Such IRQs may be timer interrupts, which may trigger and need
to be handled while the system is suspended. The whole IRQ line is affected by this
flag in that if the IRQ is shared, every registered handler for this shared line will be
executed, not just the one who installed this flag. You should avoid using IRQF_
NO_SUSPEND and IRQF_SHARED at the same time as much as possible.
• IRQF_FORCE_RESUME: This enables the IRQ in the system resume path, even if
IRQF_NO_SUSPEND is set.
• IRQF_NO_THREAD: This prevents the interrupt handler from being threaded. This
flag overrides the threadirqs kernel (used on RT kernels, such as when applying
the PREEMPT_RT patch) command-line option, which forces every interrupt to
be threaded. This flag was introduced to address the non-threadability of some
interrupts (for example, timers, which cannot be threaded even when all the
interrupt handlers are forced to be threaded).
• IRQF_TIMER: This marks this handler as being specific to the system timer
interrupts. It helps not to disable the timer IRQ during system suspend to ensure
that it resumes normally and does not thread them when full preemption (see
PREEMPT_RT) is enabled. It is just an alias for IRQF_NO_SUSPEND | IRQF_NO_
THREAD.
• IRQF_EARLY_RESUME: This resumes IRQ early at the resume time of system core
(syscore) operations instead of at device resume time. Go to https://lkml.
org/lkml/2013/11/20/89 to see the commit introducing its support.
We must also consider the return type, irqreturn_t, of interrupt handlers since they
may involve further actions once the handler is returned:
• IRQ_NONE: On a shared interrupt line, once the interrupt occurs, the kernel
irqcore successively walks through the handlers that have been registered for this
line and executes them in the order they have been registered. The driver then has
the responsibility of checking whether it is their device that issued the interrupt. If
the interrupt does not come from its device, it must return IRQ_NONE in order to
instruct the kernel to call the next registered interrupt handler. This return value is
mostly used on shared interrupt lines since it informs the kernel that the interrupt
does not come from our device. However, if 99,900 of the previous 100,000
interrupts of a given IRQ line have not been handled, the kernel assumes that this
IRQ is stuck in some manner, drops a diagnostic, and tries to turn the IRQ off. For
more information on this, have a look at the __report_bad_irq() function in
the kernel source tree.
50 Linux Kernel Concepts for Embedded Developers
• IRQ_HANDLED: This value should be returned if the interrupt has been handled
successfully. On a threaded IRQ, this value acknowledges the interrupt without
waking the thread handler up.
• IRQ_WAKE_THREAD: On a thread IRQ handler, this value must be returned the by
hard-IRQ handler in order to wake the handler thread. In this case, IRQ_HANDLED
must only be returned by the threaded handler that was previously registered with
request_threaded_irq(). We will discuss this later in the Threaded IRQ
handlers section of this chapter.
## Important note
You must be very careful when reenabling interrupts in the handler. Actually,
you must never reenable IRQs from within your IRQ handler as this would
involve allowing “interrupts reentrancy”. In this case, it is your responsibility to
address this.
## In the unloading path of your driver (or once you think you do not need the IRQ line
anymore during your driver runtime life cycle, which is quite rare), you must release
your IRQ resource by unregistering your interrupt handler and potentially disabling the
interrupt line. The free_irq() interface does this for you:
```c
void free_irq(unsigned int irq, void *dev_id)
```
That being said, if an IRQ allocated with devm_request_irq() needs to be freed
separately, devm_free_irq() must be used. It has the following prototype:
```c
void devm_free_irq(struct device *dev,
```
unsigned int irq,
```c
void *dev_id)
```
This function has an extra dev argument, which is the device to free the IRQ for. This
is usually the same as the one that the IRQ has been registered for. Except for dev, this
function takes the same arguments and performs the same function as free_irq().
However, instead of free_irq(), it should be used to manually free IRQs that have
been allocated with devm_request_irq().
Both devm_request_irq() and free_irq() remove the handler (identified by
dev_id when it comes to shared interrupts) and disable the line. If the interrupt line
is shared, the handler is simply removed from the list of handlers for this IRQ, and the
interrupt line is disabled in the future when the last handler is removed. Moreover, if
possible, your code must ensure the interrupt is really disabled on the card it drives before
calling this function, since omitting this may leads to spurious IRQs.
## Linux kernel interrupt management 51
There are few things that are worth mentioning here about interrupts that you should
never forget:
• Since interrupt handlers in Linux run with IRQs disabled on the local CPU and
the current line is masked in all other cores, they don’t need to be reentrant, since
the same interrupt will never be received until the current handler has completed.
## However, all other interrupts (on other cores) remain enabled (or should we say
untouched), so other interrupts keep being serviced, even though the current line
is always disabled, as well as further interrupts on the local CPU. Consequently, the
same interrupt handler is never invoked concurrently to service a nested interrupt.
This greatly simplifies writing your interrupt handler.
• Critical regions that need to run with interrupts disabled should be limited as
much as possible. To remember this, tell yourselves that your interrupt handler has
interrupted other code and needs to give CPU back.
• Interrupt handlers cannot block as they do not run in a process context.
• They may not transfer data to/from user space since this may block.
• They may not sleep or rely on code that may lead to sleep, such as invoking wait_
event(), memory allocation with anything other than GFP_ATOMIC, or using a
mutex/semaphore. The threaded handler can handle this.
• They may not trigger nor call schedule().
• Only one interrupt on a given line can be pending (its interrupt flag bits get set
when its interrupt condition occurs, regardless of the state of its corresponding
enabled bit or the global enabled bit). Any further interrupt of this line is lost. For
example, if you are processing an RX interrupt while five more packets are received
at the same time, you should not expect five times more interrupts to appear
sequentially. You’ll only be notified once. If the processor doesn’t service the ISR
first, there’s no way to check how many RX interrupts will occur later. This means
that if the device generates another interrupt before the handler function returns
IRQ_HANDLED, the interrupt controller will be notified of the pending interrupt
flag and the handler will get called again (only once), so you may miss some
interrupts if you are not fast enough. Multiple interrupts will happen while you are
still handling the first one.
52 Linux Kernel Concepts for Embedded Developers
## Important note
If an interrupt occurs while it is disabled (or masked), it will not be processed
at all (masked in the flow handler), but will be recognized as asserted and will
remain pending so that it will be processed when enabled (or unmasked).
The interrupt context has its own (fixed and quite low) stack size. Therefore, it
totally makes sense to disable IRQs while running an ISR as reentrancy could
cause stack overflow if too many preemptions happen.
## The concept of non-reentrancy for an interrupt means that if an interrupt is
already in an active state, it cannot enter it again until the active status
is cleared.
## The concept of top and bottom halves
External devices send interrupt requests to the CPU either to signal a particular event or
to request a service. As stated in the previous section, bad interrupt management may
considerably increase system latency and decrease its real-time quality. We also stated that
interrupt processing – that is, the hard-IRQ handler – must be very fast, not only to keep
the system responsive, but also so that it doesn’t miss other interrupt events.
Take a look at the following diagram:
Figure 1.2 – Interrupt splitting flow
## Linux kernel interrupt management 53
The basic idea is that you split the interrupt handler into two parts. The first part is a
function) that will run in a so-called hard-IRQ context, with interrupts disabled, and
perform the minimum required work (such as doing some quick sanity checks, timesensitive tasks, read/write hardware registers, and processing this data and acknowledging
the interrupt on the device that raised it). This first part is the so-called top-half on Linux
systems. The top-half then schedules a (sometimes threaded) handler, which then runs
a so-called bottom-half function, with interrupts re-enabled. This is the second part of
the interrupt. The bottom-half may then perform time-consuming tasks (such as buffer
processing) – tasks that may sleep, depending on the deferring mechanism.
This splitting would considerably increase the system’s responsiveness as the time spent
with IRQs disabled is reduced to its minimum. When the bottom halves are run in kernel
threads, they compete for the CPU with other processes on the runqueue. Moreover,
they may have their real-time properties set. The top half is actually the handler that’s
registered using request_irq(). When using request_threaded_irq(), as we
will see in the next section, the top half is the first handler that’s given to the function.
As we described previously, a bottom half represents any task (or work) that’s scheduled
from within an interrupt handler. Bottom halves are designed using a work-deferring
mechanism, which we have seen previously. Depending on which one you choose, it
may run in a (software) interrupt context or in a process context. This includes SoftIRQs,
tasklets, workqueues, and threaded IRQs.
## Important note
Tasklets and SoftIRQs do not actually fit into the so-called “thread interrupts”
mechanism since they run in their own special contexts.
## Since softIRQ handlers run at a high priority with scheduler preemption disabled, they
do not relinquish the CPU to processes/threads until they complete, so care must be
taken while using them for bottom-half delegation. Nowadays, since the quantum that’s
allocated for a particular process may vary, there is no strict rule regarding how long the
softIRQ handler should take to complete so that it doesn’t slow the system down as the
kernel would not be able to give CPU time to other processes. I would say that this should
be no longer than a half of jiffy.
54 Linux Kernel Concepts for Embedded Developers
## The hard-IRQ handler (the top half) has to be as fast as possible, and most of time, it
should just be reading and writing in I/O memory. Any other computation should be
deferred to the bottom half, whose main goal is to perform any time-consuming and
minimal interrupt-related work that’s not performed by the top half. There are no clear
guidelines on repartitioning work between the top and bottom halves. The following is
some advice:
• Hardware-related work and time-sensitive work should be performed in the
top half.
• If the work doesn’t need to be interrupted, perform it in the top half.
• From my point of view, everything else can be deferred – that is, performed in
the bottom half – so that it runs with interrupts enabled and when the system is
less busy.
• If the hard-IRQ handler is fast enough to process and acknowledge interrupts
consistently within a few microseconds, then there is absolutely no need to use
bottom-half delegations at all.
Next, we will look at threaded IRQ handlers.
## Threaded IRQ handlers
Threaded interrupt handlers were introduced to reduce the time spent in the interrupt
handler and deferring the rest of the work (that is, processing) out to kernel threads. So,
the top half (hard-IRQ handler) would consist of quick sanity checks such as ensuring that
the interrupt comes from its device and waking the bottom half accordingly. A threaded
interrupt handler runs in its own thread, either in the thread of their parent (if they have
one) or in a separate kernel thread. Moreover, the dedicated kernel thread can have its
real-time priority set, though it runs at normal real-time priority (that is, MAX_USER_
RT_PRIO/2 as shown in the setup_irq_thread() function in kernel/irq/
manage.c).
The general rule behind threaded interrupts is simple: keep the hard-IRQ handler as
minimal as possible and defer as much work to the kernel thread as possible (preferably
all work). You should use request_threaded_irq() (defined in kernel/irq/
manage.c) if you want to request a threaded interrupt handler:
int
request_threaded_irq(unsigned int irq,
irq_handler_t handler,
irq_handler_t thread_fn,
unsigned long irqflags,
## Linux kernel interrupt management 55
const char *devname,
```c
void *dev_id)
```
This function accepts two special parameters handler and thread_fn. The other
parameters are the same as they are for request_irq():
• handler immediately runs when the interrupt occurs in the interrupt context, and
acts as a hard-IRQ handler. Its job usually consists of reading the interrupt cause
(in the device’s status register) to determine whether or how to handle the interrupt
(this is frequent on MMIO devices). If the interrupt does not come from its device,
this function should return IRQ_NONE. This return value usually only makes sense
on shared interrupt lines. In the other case, if this hard-IRQ handler can finish
interrupt processing fast enough (this is not a universal rule, but let’s say no longer
than half a jiffy – that is, no longer than 500 µs if CONFIG_HZ, which defines
the value of a jiffy, is set to 1,000) for a set of interrupt causes, it should return
IRQ_HANDLED after processing in order to acknowledge the interrupts. Interrupt
processing that does not fall into this time lapse should be deferred to the threaded
IRQ handler. In this case, the hard-IRQ handler should return IRQ_WAKE_T
HREAD in order to awake the threaded handler. Returning IRQ_WAKE_THREAD
only makes sense when the thread_fn handler is also registered.
• thread_fn is the threaded handler that’s added to the scheduler runqueue when
the hard-IRQ handler function returns IRQ_WAKE_THREAD. If thread_fn is
NULL while handler is set and it returns IRQ_WAKE_THREAD, nothing happens
at the return path of the hard-IRQ handler except for a simple warning message
being shown. Have a look at the __irq_wake_thread() function in the kernel
sources for more information. As thread_fn competes for the CPU with other
processes on the runqueue, it may be executed immediately or later in the future
when the system has less load. This function should return IRQ_HANDLED when
it has completed the interrupt handling process successfully. At this stage, the
associated kthread will be taken off the runqueue and put in a blocked state until it’s
woken up again by the hard-IRQ function.
56 Linux Kernel Concepts for Embedded Developers
## A default hard-IRQ handler will be installed by the kernel if handler is NULL and
thread_fn != NULL. This is the default primary handler. It is an almost empty
handler that simply returns IRQ_WAKE_THREAD in order to wake up the associated
kernel thread that will execute the thread_fn handler. This makes it possible to move
the execution of interrupt handlers entirely to the process context, thus preventing buggy
drivers (buggy IRQ handlers) from breaking the whole system and reducing interrupt
latency. A dedicated handler’s kthreads will be visible in ps ax:
/*
* Default primary interrupt handler for threaded interrupts is
* assigned as primary handler when request_threaded_irq is
* called with handler == NULL. Useful for one-shot interrupts.
*/
```c
static irqreturn_t irq_default_primary_handler(int irq,
void *dev_id)
{
return IRQ_WAKE_THREAD;
}
```
int
request_threaded_irq(unsigned int irq,
irq_handler_t handler,
irq_handler_t thread_fn,
unsigned long irqflags,
const char *devname,
```c
void *dev_id)
{
```
[...]
```c
if (!handler) {
if (!thread_fn)
return -EINVAL;
handler = irq_default_primary_handler;
}
```
[...]
```c
}
EXPORT_SYMBOL(request_threaded_irq);
```
## Linux kernel interrupt management 57
Important note
Nowadays, request_irq() is just a wrapper around request_
threaded_irq(), with the thread_fn parameter set to NULL.
## Note that the interrupt is acknowledged at the interrupt controller level when you return
from the hard-IRQ handler (whatever the return value is), thus allowing you to take other
interrupts into account. In such a situation, if the interrupt hasn’t been acknowledged
at the device level, the interrupt will fire again and again, resulting in stack overflows
(or being stuck in the hard-IRQ handler forever) for level-triggered interrupts since the
issuing device still has the interrupt line asserted. Before threaded IRQs were a thing,
when you needed to run the bottom-half in a thread, you would instruct the top half to
disable the IRQ at the device level, prior to waking the thread up. This way, even if the
controller is ready to accept another interrupt, it is not raised again by the device.
The IRQF_ONESHOT flag resolves this problem. It must be set when it comes to use a
```c
threaded interrupt (at the request_threaded_irq() call); otherwise, the request will
```
fail with the following error:
pr_err(
“Threaded irq requested with handler=NULL and !ONESHOT for irq
%d\n”,
```c
irq);
```
For more information on this, please have a look at the __setup_irq() function in the
kernel source tree.
The following is an excerpt from the message that introduced the IRQF_ONESHOT flag
and explains what it does (the entire message can be found at http://lkml.iu.edu/
hypermail/linux/kernel/0908.1/02114.html):
“It allows drivers to request that the interrupt is not unmasked (at the
controller level) after the hard interrupt context handler has been executed
and the thread has been woken. The interrupt line is unmasked after the
thread handler function has been executed.”
## Important note
If you omit the IRQF_ONESHOT flag, you’ll have to provide a hard-IRQ
```c
handler (in which you should disable the interrupt line); otherwise, the request
```
will fail.
58 Linux Kernel Concepts for Embedded Developers
An example of a thread-only IRQ is as follows:
```c
static irqreturn_t data_event_handler(int irq, void *dev_id)
{
struct big_structure *bs = dev_id;
process_data(bs->buffer);
return IRQ_HANDLED;
}
static int my_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
```
[...]
```c
if (client->irq > 0) {
```
ret = request_threaded_irq(client->irq,
## NULL,
&data_event_handler,
IRQF_TRIGGER_LOW | IRQF_ONESHOT,
id->name,
```c
private);
if (ret)
goto error_irq;
}
```
[...]
```c
return 0;
```
error_irq:
```c
do_cleanup();
return ret;
}
```
In the preceding example, our device sits on an I2C bus. Thus, accessing the available data
may cause it to sleep, so this should not be performed in the hard-IRQ handler. This is
why our handler parameter is NULL.
## Linux kernel interrupt management 59
Tip
## If the IRQ line where you need threaded ISR handling to be shared among
several devices (for example, some SoCs share the same interrupt among their
internal ADCs and the touchscreen module), you must implement the hardIRQ handler, which should check whether the interrupt has been raised by
your device or not. If the interrupt does come from your device, you should
disable the interrupt at the device level and return IRQ_WAKE_THREAD to
wake the threaded handler. The interrupt should be enabled back at the device
level in the return path of the threaded handler. If the interrupt does not come
from your device, you should return IRQ_NONE directly from the hard-IRQ
handler.
Moreover, if one driver has set either the IRQF_SHARED or IRQF_ONESHOT flag
on the line, every other driver sharing the line must set the same flags. The /proc/
interrupts file lists the IRQs and their processing per CPU, the IRQ name that was
given during the requesting step, and a comma-separated list of drivers that registered an
ISR for that interrupt.
## Threaded IRQs are the best choice for interrupt processing as they can hog too many CPU
cycles (exceeding a jiffy in most cases), such as bulk data processing. Threading IRQs
allow the priority and CPU affinity of their associated thread to be managed individually.
## Since this concept comes from the real-time kernel tree (from Thomas Gleixner), it fulfills
many requirements of a real-time system, such as allowing a fine-grained priority model
to be used and reducing interrupt latency in the kernel.
Take a look at /proc/irq/IRQ_NUMBER/smp_affinity, which can be used to get
or set the corresponding IRQ_NUMBER affinity. This file returns and accepts a bitmask
that represents which processors can handle ISRs that have been registered for this IRQ.
## This way, you can, for example, decide to set the affinity of a hard-IRQ to one CPU while
setting the affinity of the threaded handler to another CPU.
## Requesting a context IRQ
A driver requesting an IRQ must know the nature of the interrupt in advance and decide
whether its handler can run in the hard-IRQ context in order to call request_irq() or
request_threaded_irq() accordingly.
60 Linux Kernel Concepts for Embedded Developers
## There is a problem when it comes to request IRQ lines provided by discrete and
non-MMIO-based interrupt controllers, such as I2C/SPI gpio-expanders. Since accessing
those buses may cause them to sleep, it would be a disaster to run the handler of such
slow controllers in a hard-IRQ context. Since the driver does not contain any information
about the nature of the interrupt line/controller, the IRQ core provides the request_
any_context_irq() API. This function determines whether the interrupt controller/
line can sleep and calls the appropriate requesting function:
```c
int request_any_context_irq(unsigned int irq,
```
irq_handler_t handler,
unsigned long flags,
const char *name,
```c
void *dev_id)
```
request_any_context_irq() and request_irq() have the same interface
but different semantics. Depending on the underlying context (the hardware platform),
request_any_context_irq() selects either a hardIRQ handling method using
request_irq() or a threaded handling method using request_threaded_irq().
It returns a negative error value on failure, while on success, it returns either IRQC_IS_
HARDIRQ (meaning hardI-RQ handling is used) or IRQC_IS_NESTED (meaning the
threaded version is used). With this function, the behavior of the interrupt handler is
decided at runtime. For more information, take a look at the comment introducing it in
the kernel by following this link: https://git.kernel.org/pub/scm/linux/
kernel/git/next/linux-next.git/commit/?id=ae731f8d0785.
The advantage of using request_any_context_irq() is that you don’t need to
care about what can be done in the IRQ handler. This is because the context in which
the handler will run depends on the interrupt controller that provides the IRQ line. For
example, for a gpio-IRQ-based device driver, if the gpio belongs to a controller that seats
on an I2C or SPI bus (in which case gpio access may sleep), the handler will be threaded.
## Otherwise (the gpio access may not sleep and is memory mapped as it belongs to the
SoC), the handler will run in the hardIRQ context.
In the following example, the device expects an IRQ line mapped to a gpio. The driver
cannot assume that the given gpio line will be memory mapped since it’s coming from
the SoC. It may come from a discrete I2C or SPI gpio controller as well. A good practice
would be to use request_any_context_irq() here:
```c
static irqreturn_t packt_btn_interrupt(int irq, void *dev_id)
{
struct btn_data *priv = dev_id;
```
## Linux kernel interrupt management 61
input_report_key(priv->i_dev,
BTN_0,
```c
gpiod_get_value(priv->btn_gpiod) & 1);
input_sync(priv->i_dev);
return IRQ_HANDLED;
}
static int btn_probe(struct platform_device *pdev)
{
struct gpio_desc *gpiod;
int ret, irq;
gpiod = gpiod_get(&pdev->dev, “button”, GPIOD_IN);
if (IS_ERR(gpiod))
return -ENODEV;
priv->irq = gpiod_to_irq(priv->btn_gpiod);
priv->btn_gpiod = gpiod;
```
[...]
ret = request_any_context_irq(
priv->irq,
packt_btn_interrupt,
(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING),
“packt-input-button”,
```c
priv);
if (ret < 0)
goto err_btn;
return 0;
```
err_btn:
```c
do_cleanup();
return ret;
}
```
The preceding code is simple enough but is quite safe thanks to request_any_
context_irq(), which prevents us from mistaking the type of the underlying gpio.
62 Linux Kernel Concepts for Embedded Developers
## Using a workqueue to defer a bottom-half
Since we have already discussed the workqueue API, we will provide an example of
how to use it here. This example is not error-free and has not been tested. It is just
a demonstration that highlights the concept of bottom-half deferring by means of a
workqueue.
Let’s start by defining the data structure that will hold the elements we need for further
development:
```c
struct private_struct {
int counter;
struct work_struct my_work;
void __iomem *reg_base;
spinlock_t lock;
int irq;
```
/* Other fields */
[...]
```c
};
```
In the preceding data structure, our work structure is represented by the my_work
element. We aren’t using the pointer here because we will need to use the container_
of() macro to grab a pointer to the initial data structure. Next, we can define the method
that will be invoked in the worker thread:
```c
static void work_handler(struct work_struct *work)
{
int i;
unsigned long flags;
struct private_data *my_data =
container_of(work, struct private_data, my_work);
```
/*
* let’s proccessing at least half of MIN_REQUIRED_FIFO_SIZE
* prior to re-enabling the irq at device level, and so that
* buffer further data
*/
```c
for (i = 0, i < MIN_REQUIRED_FIFO_SIZE, i++) {
device_pop_and_process_data_buffer();
if (i == MIN_REQUIRED_FIFO_SIZE / 2)
enable_irq_at_device_level();
```
## Linux kernel interrupt management 63
```c
}
spin_lock_irqsave(&my_data->lock, flags);
my_data->buf_counter -= MIN_REQUIRED_FIFO_SIZE;
spin_unlock_irqrestore(&my_data->lock, flags);
}
```
In the preceding code, we start data processing when enough data has been buffered. Now,
we can provide our IRQ handler, which is responsible for scheduling our work, as follows:
/* This is our hard-IRQ handler.*/
```c
static irqreturn_t my_interrupt_handler(int irq, void *dev_id)
{
u32 status;
unsigned long flags;
struct private_struct *my_data = dev_id;
```
/* Let’s read the status register in order to determine how
* and what to do
*/
```c
status = readl(my_data->reg_base + REG_STATUS_OFFSET);
```
/*
* Let’s ack this irq at device level. Even if it raises
* another irq, we are safe since this irq remain disabled
* at controller level while we are in this handler
*/
writel(my_data->reg_base + REG_STATUS_OFFSET,
```c
status | MASK_IRQ_ACK);
```
/*
* Protecting the shared resource, since the worker also
* accesses this counter
*/
```c
spin_lock_irqsave(&my_data->lock, flags);
my_data->buf_counter++;
spin_unlock_irqrestore(&my_data->lock, flags);
```
/*
64 Linux Kernel Concepts for Embedded Developers
* Ok. Our device raised an interrupt in order to inform it
* has some new data in its fifo. But is it enough for us
* to be processed
*/
```c
if (my_data->buf_counter != MIN_REQUIRED_FIFO_SIZE)) {
```
/* ack and re-enable this irq at controller level */
```c
return IRQ_HANDLED;
} else {
```
/*
* Right. prior to schedule the worker and returning
* from this handler, we need to disable the irq at
* device level
*/
writel(my_data->reg_base + REG_STATUS_OFFSET,
```c
MASK_IRQ_DISABLE);
schedule_work(&my_work);
}
```
/* This will re-enable the irq at controller level */
```c
return IRQ_HANDLED;
};
```
The comments in the IRQ handler code are meaningful enough. schedule_work() is
the function that schedules our work. Finally, we can write our probe method, which will
request our IRQ and register the previous handler:
```c
static int foo_probe(struct platform_device *pdev)
{
struct resource *mem;
struct private_struct *my_data;
my_data = alloc_some_memory(sizeof(struct private_struct));
mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
```
my_data->reg_base =
```c
ioremap(ioremap(mem->start, resource_size(mem));
if (IS_ERR(my_data->reg_base))
return PTR_ERR(my_data->reg_base);
```
## Linux kernel interrupt management 65
/*
* work queue initialization. “work_handler” is the
* callback that will be executed when our work is
* scheduled.
*/
```c
INIT_WORK(&my_data->my_work, work_handler);
spin_lock_init(&my_data->lock);
my_data->irq = platform_get_irq(pdev, 0);
if (request_irq(my_data->irq, my_interrupt_handler,
```
0, pdev->name, my_data))
handler_this_error()
```c
return 0;
}
```
## The structure of the preceding probe method shows without a doubt that we are facing a
platform device driver. Generic IRQ and workqueue APIs have been used here to initialize
our workqueue and register our handler.
## Locking from within an interrupt handler
If a resource is shared between two or more use contexts (kthread, work, threaded IRQ,
and so on) and only with a threaded bottom-half (that is, they’re never accessed by the
hard-IRQ), then mutex locking is the way to go, as shown in the following example:
```c
static int my_probe(struct platform_device *pdev)
{
int irq;
int ret;
irq = platform_get_irq(pdev, i);
```
ret = devm_request_threaded_irq(dev, irq, NULL,
my_threaded_irq,
IRQF_ONESHOT, dev_
name(dev),
```c
my_data);
```
[...]
```c
return ret;
```
66 Linux Kernel Concepts for Embedded Developers
```c
}
static irqreturn_t my_threaded_irq(int irq, void *dev_id)
{
struct priv_struct *my_data = dev_id;
```
/* Save FIFO Underrun & Transfer Error status */
```c
mutex_lock(&my_data->fifo_lock);
```
/* accessing the device’s buffer through i2c */
[...]
```c
mutex_unlock(&ldev->fifo_lock);
return IRQ_HANDLED;
}
```
## In the preceding code, both the user task (kthread, work, and so on) and the threaded
bottom half must hold the mutex before accessing the resource.
The preceding case is the simplest one to exemplify. The following are some rules that will
help you lock between hard-IRQ contexts and others:
• If a resource is shared between a user context and a hard interrupt handler, you will
```c
want to use the spinlock variant, which disables interrupts; that is, the simple _irq
```
or _irqsave/_irq_restore variants. This ensures that the user context is never
preempted by this IRQ when it’s accessing the resource. This can be seen in the
following example:
```c
static int my_probe(struct platform_device *pdev)
{
int irq;
int ret;
```
[...]
```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
goto handle_get_irq_error;
```
ret = devm_request_threaded_irq(&pdev->dev,
irq,
## Linux kernel interrupt management 67
my_hardirq,
my_threaded_irq,
IRQF_ONESHOT,
dev_name(dev),
```c
my_data);
if (ret < 0)
goto err_cleanup_irq;
```
[...]
```c
return 0;
}
static irqreturn_t my_hardirq(int irq, void *dev_id)
{
struct priv_struct *my_data = dev_id;
unsigned long flags;
```
/* No need to protect the shared resource */
my_data->status = __raw_readl(
```c
my_data->mmio_base + my_data->foo.reg_offset);
```
/* Let us schedule the bottom-half */
```c
return IRQ_WAKE_THREAD;
}
static irqreturn_t my_threaded_irq(int irq, void *dev_id)
{
struct priv_struct *my_data = dev_id;
spin_lock_irqsave(&my_data->lock, flags);
```
/* Processing the status status */
```c
process_status(my_data->status);
spin_unlock_irqrestore(&my_data->lock, flags);
```
[...]
```c
return IRQ_HANDLED;
}
```
68 Linux Kernel Concepts for Embedded Developers
In the preceding code, the hard-IRQ handler doesn’t need to hold the spinlock
as it can never be preempted. Only the user context must be held. There is a case
where protection may not be necessary between the hard-IRQ and its threaded
```c
counterpart; that is, when the IRQF_ONESHOT flag is set while requesting the IRQ
```
line. This flag keeps the interrupt disabled after the hard-IRQ handler has finished.
## With this flag set, the IRQ line remains disabled until the threaded handler has
been run until its completion. This way, the hard-IRQ handler and its threaded
counterpart will never compete and a lock for a resource shared between the two
might not be necessary.
• When the resource is shared between user context and softIRQ, there are two
things you need to guard against: the fact the user context can be interrupted by
the softIRQ (remember, softIRQs run on the return path of hard-IRQ handlers)
and the fact that the critical region can be entered from another CPU (remember,
the same softIRQ may run concurrently on another CPU). In this case, you should
```c
use spinlock API variants that will disable softIRQs; that is, spin_lock_bh()
```
and spin_unlock_bh(). The _bh prefix means the bottom half. Because those
APIs have not been discussed in detail in this chapter, you can use the _irq or even
_irqsave variants, which disable hardware interrupts as well.
• The same applies to tasklets (because tasklets are built on top of softIRQs), with the
only difference that a tasklet never runs concurrently (it never runs on more than
```c
one CPU at once); a tasklet is exclusive by design.
```
• There are two things to guard against when it comes to locking between hard IRQ
and softIRQ: the softIRQ can be interrupted by the hard-IRQ and the critical region
can be entered (1 for either by another hard-IRQ if designed in this way, 2 by the
same softIRQ, or 3 by another softIRQ) from another CPU. Because the softIRQ
can never run when the hard-IRQ handler is running, hard-IRQ handlers only need
to use the spin_lock() and spin_unlock() APIs, which prevent concurrent
access by other hard handlers on another CPU. However, softIRQ needs to use the
locking API that actually disables interrupts – that is, the _irq() or irqsave()
variants – with a preference for the latter.
• Because softIRQs may run concurrently, locking may be necessary between two
different softIRQs, or even between a softIRQ and itself (running on another CPU).
In this case, spinlock()/spin_unlock() should be used. There’s no need to
disable hardware interrupts.
## At this point, we are done looking at interrupt locking, which means we have come to the
end of this chapter.
## Summary 69
Summary
## This chapter introduced some core kernel functionalities that will be used in the next
few chapters of this book. The concepts we covered concerned bit manipulation to Linux
kernel interrupt design and implementation, through locking helpers and work deferring
mechanisms. By now, you should be able to decide whether you should split your
interrupt handler into two parts or not, as well as know what locking primitive suits your
needs.
In the next chapter, we’ll cover Linux kernel managed resources, which is an interface
that’s used to offload allocated resource management to the kernel core