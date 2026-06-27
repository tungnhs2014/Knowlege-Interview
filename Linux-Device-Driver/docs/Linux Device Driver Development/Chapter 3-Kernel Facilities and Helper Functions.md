```bash
# Chapter 3 - Kernel Facilities and Helper Functions 

The kernel is a standalone piece of software, as you'll see in this chapter, that does not make use of any C library. It implements any mechanism you may encounter in modern libraries,
```
and even more, such as compression, string functions, and so on. We will walk step by step through the most important aspects of such capabilities.
In this chapter, we will cover the following topics:
Introducing the kernel container data structure
Dealing with the kernel sleeping mechanism
Using timers
```c
Delving into the kernel locking mechanism (mutex, spinlock)
```
Deferring work using a dedicated kernel API
Using IRQs
## Understanding the container_of macro
When it comes to managing several data structures in code, you'll almost always need to embed one structure into another and retrieve them at any moment without being asked questions about memory offsets or boundaries. Let's say you have a struct person, as defined here:
```c
struct person {
int age;
int salary;
```
char *name;
```c
} p;
```
By only having a pointer on age or salary, you can retrieve the whole structure wrapping
(containing) that pointer. As the name says, the container_of macro is used to find the container of the given field of a structure. The macro is defined in include/linux/kernel.h and looks like the following:
```c
#define container_of(ptr, type, member) ({ \
```
const typeof(((type *)0)->member) * __mptr = (ptr); \
(type *)((char *)__mptr - offsetof(type, member)); })
Don't be afraid of the pointers; just see them as follows:
```c
container_of(pointer, container_type, container_field);
```
Here are the elements of the preceding code fragment:
pointer: This is the pointer to the field in the structure container_type: This is the type of structure wrapping (containing) the pointer container_field: This is the name of the field to which pointer points inside the structure
Let's consider the following container:
```c
struct person {
int age;
int salary;
```
char *name;
```c
};
```
Now, let's consider one of its instances, along with a pointer to the age member:
```c
struct person somebody;
```
[...]
```c
int *age_ptr = &somebody.age;
```
Along with a pointer to the name member (age_ptr),you can use the container_of macro in order to get a pointer to the whole structure (container) that wraps this member by using the following:
```c
struct person *the_person;
```
the_person = container_of(age_ptr, struct person, age);
```c
container_of takes the offset of age at the beginning of the struct into account to get the correct pointer location. If you subtract the offset of the field age from the pointer age_ptr,
```
you will get the correct location. This is what the macro's last line does:
(type *)( (char *)__mptr - offsetof(type,member) );
Applying this to a real example, gives the following:
```c
struct family {
struct person *father;
struct person *mother;
int number_of_sons;
int family_id;
} f;
```
/*
* Fill and initialise f somewhere */ [...]
/*
* pointer to a field of the structure
* (could be any (non-pointer) member in the structure)
*/
```c
int *fam_id_ptr = &f.family_id;
struct family *fam_ptr;
```
/* now let us retrieve back its family */
fam_ptr = container_of(fam_id_ptr, struct family, family_id);
The container_of macro won't work for char * or array members. It means the first member of container_of must not be a pointer to another pointer to char nor to array in the structure. In other words, in our first example with the struct person, it would have been wrong to use name field to retrieve the containing structure. Giving a pointer to father or mother as first parameter to container_of should be wrong, since those members are already pointer fields in the structure.
But what about retrieving the structure holding a pointer with container_of macro ? Let's have a look to our preceding example with the struct family structure. If you only have a pointer to father or mother (that is, if you just have struct person *dad or struct person *mom), you cannot use the container_of macro to retrieve the container struct family. To use it properly, you'll need a pointer to a pointer to struct person (that is, struct person **dad or struct person **mom) and use it like struct family *fam =
```c
container_of(dad, struct family, father);.
```
This is all you need to know about the container_of macro, and believe me, it is enough.
In real drivers that we'll develop later in the book, it looks like the following:
```c
struct mcp23016 {
struct i2c_client *client;
struct gpio_chip chip;
}
```
/* retrieve the mcp23016 struct given a pointer 'chip' field */
```c
static inline struct mcp23016 *to_mcp23016(struct gpio_chip *gc)
{
return container_of(gc, struct mcp23016, chip);
}
static int mcp23016_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
struct mcp23016 *mcp;
```
[...]
mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
```c
if (!mcp)
return -ENOMEM;
```
[...]
```c
}
```
The container_of macro is mainly used in generic containers in the kernel. In some examples in this book (starting from Chapter 5, Platform Device Drivers), you will encounter the container_of macro.
## Linked lists
Imagine you have a driver that manages more than one device, let's say five devices. You may need to keep a track of each of them in your driver. What you need here is a linked list.
Two types of linked list actually exist:
Simply linked list
Doubly linked list
Therefore, kernel developers only implement circular doubly linked lists because this structure allows you to implement FIFO and LIFO, and kernel developers take care to maintain a minimal set of code. The header to be added in the code in order to support lists is <linux/list.h>. The data structure at the core of list implementation in the kernel is the struct list_head structure, defined as the following:
```c
struct list_head {
struct list_head *next, *prev;
};
```
The struct list_head is used in both the head of the list and each node. In the world of the kernel, before a data structure can be represented as a linked list, that structure must embed a struct list_head field. For example, let's create a list of cars:
```c
struct car {
int door_number;
```
char *color;
char *model;
```c
};
```
Before we can create a list for the cars, we must change its structure in order to embed a struct list_head field. The structure becomes:
```c
struct car {
int door_number;
```
char *color;
char *model;
```c
struct list_head list; /* kernel's list structure */
};
```
First, we need to create a struct list_head variable that will always point to the head
(first element) of our list. This instance of list_head is not associated to any car and is special:
```c
static LIST_HEAD(carlist) ;
```
Now, we can create cars and add them to our list, carlist:
```c
#include <linux/list.h>
struct car *redcar = kmalloc(sizeof(*car), GFP_KERNEL);
struct car *bluecar = kmalloc(sizeof(*car), GFP_KERNEL);
```
/* Initialize each node's list entry */
```c
INIT_LIST_HEAD(&bluecar->list);
INIT_LIST_HEAD(&redcar->list);
```
/* allocate memory for color and model field and fill every field */
[...]
```c
list_add(&redcar->list, &carlist) ;
list_add(&bluecar->list, &carlist) ;
```
It is as simple as that. Now, carlist contains two elements. Let's get deeper into the linked list API.
## Creating and initializing a list
There are two ways to create and initialize a list.
## Dynamic method
The dynamic method consists of a struct list_head and initializes it with the
```c
INIT_LIST_HEAD macro:
struct list_head mylist;
INIT_LIST_HEAD(&mylist);
```
The following is the expansion of INIT_LIST_HEAD:
```c
static inline void INIT_LIST_HEAD(struct list_head *list)
{
```
list->next = list;
list->prev = list;
```c
}
```
## Static method
Static allocation is done through the LIST_HEAD macro:
```c
LIST_HEAD(mylist)
```
The LIST_HEAD definition is as follows:
```c
#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)
```
The following is its expansion:
```c
#define LIST_HEAD_INIT(name) { &(name), &(name) }
```
This assigns each pointer (prev and next) inside the name field to point to name itself (just like INIT_LIST_HEAD does).
## Creating a list node
To create new nodes, just create our data struct instances and initialize their embedded list_head field. Using the car example will give the following:
```c
struct car *blackcar = kzalloc(sizeof(struct car), GFP_KERNEL);
```
/* non static initialization, since it is the embedded list field*/
```c
INIT_LIST_HEAD(&blackcar->list);
```
As we said earlier, use INIT_LIST_HEAD, which is a dynamically allocated list and usually part of another structure.
## Adding a list node
The kernel provides list_add to add a new entry to the list, which is a wrapper around the internal function __list_add:
```c
void list_add(struct list_head *new, struct list_head *head);
static inline void list_add(struct list_head *new, struct list_head *head)
{
__list_add(new, head, head->next);
}
```
__list_add will take two known entries as a parameter, and inserts your elements between them. Its implementation in the kernel is quite easy:
```c
static inline void __list_add(struct list_head *new,
struct list_head *prev,
struct list_head *next)
{
```
next->prev = new;
new->next = next;
new->prev = prev;
prev->next = new;
```c
}
```
The following is an example of adding two cars in our list:
```c
list_add(&redcar->list, &carlist);
list_add(&blue->list, &carlist);
```
This mode can be used to implement a stack. The other function to add an entry into the list is:
```c
void list_add_tail(struct list_head *new, struct list_head *head);
```
This inserts the new entry at the end of the list. Given our previous example, we can use the following:
```c
list_add_tail(&redcar->list, &carlist);
list_add_tail(&blue->list, &carlist);
```
This mode can be used to implement a queue.
## Deleting a node from the list
List handling is an easy task in kernel code. Deleting a node is straightforward:
```c
void list_del(struct list_head *entry);
```
Following the preceding example, let's delete the red car:
```c
list_del(&redcar->list);
```
list_del disconnects the prev and next pointers of the given entry,
resulting in entry removal. The memory allocated for the node is not freed yet; you need to do that manually with kfree.
## Linked list traversal
We have the macro list_for_each_entry(pos, head, member) for list traversal:
head is the list's head node.
member is the name of the struct list_head list within our data struct (in our case, it is list).
pos is used for iteration. It is a loop cursor (just like i in for(i=0; i<foo;
i++)). head could be the head node of the linked list, or any entry, and we don't care since we are dealing with a doubly linked list:
```c
struct car *acar; /* loop counter */
int blue_car_num = 0;
```
/* 'list' is the name of the list_head struct in our data structure */
```c
list_for_each_entry(acar, carlist, list){
if(acar->color == "blue")
```
blue_car_num++;
```c
}
```
Why do we need the name of the list_head type field in our data structure? Look at the list_for_each_entry definition:
```c
#define list_for_each_entry(pos, head, member) \
for (pos = list_entry((head)->next, typeof(*pos), member); \
```
&pos->member != (head); \
pos = list_entry(pos->member.next, typeof(*pos), member))
```c
#define list_entry(ptr, type, member) \
container_of(ptr, type, member)
```
Given this, we can understand that it is all about the power of container_of. Also, bear in mind list_for_each_entry_safe(pos, n, head, member).
## The kernel sleeping mechanism
Sleeping is the mechanism by which a process relaxes a processor, with the possibility of handling another process. The reason why a processor sleeps could be for sensing data availability, or waiting for a resource to be free.
The kernel scheduler manages a list of tasks to run, known as a run queue. Sleeping processes are not scheduled anymore, since they are removed from that run queue. Unless its state changes (that is, it wakes up), a sleeping process will never be executed. You may relax a processor as soon as you are waiting for something (a resource or anything else),
and make sure a condition or something else will wake it up. That said, the Linux kernel simplifies the implementation of the sleeping mechanism by providing a set of functions and data structures.
## Wait queue
Wait queues are essentially used to process blocked I/O, to wait for particular conditions to be true, and to sense data or resource availability. To understand how they work, let's have a look at their structure in include/linux/wait.h:
```c
struct __wait_queue {
unsigned int flags;
#define WQ_FLAG_EXCLUSIVE 0x01
void *private;
wait_queue_func_t func;
struct list_head task_list;
};
```
Let's pay attention to the task_list field. As you can see, it is a list. Every process you want to put to sleep is queued in that list (hence the name wait queue) and put into a sleep state until a condition becomes true. The wait queue can be seen as nothing but a simple list of processes and a lock.
The functions you will always face when dealing with wait queues are:
Static declaration:
```c
DECLARE_WAIT_QUEUE_HEAD(name)
```
Dynamic declaration:
```c
wait_queue_head_t my_wait_queue;
init_waitqueue_head(&my_wait_queue);
```
Blocking:
/*
* block the current task (process) in the wait queue if
* CONDITION is false
*/
```c
int wait_event_interruptible(wait_queue_head_t q, CONDITION);
```
Unblocking:
/*
* wake up one process sleeping in the wait queue if
* CONDITION above has become true
*/
```c
void wake_up_interruptible(wait_queue_head_t *q);
wait_event_interruptible does not continuously poll, but simply evaluates the condition when it is called. If the condition is false, the process is put into a
```
TASK_INTERRUPTIBLE state and removed from the run queue. The condition is then only rechecked each time you call wake_up_interruptible in the wait queue. If the condition is true when wake_up_interruptible runs, a process in the wait queue will be awakened, and its state set to TASK_RUNNING. Processes are awakened in the order in which they are put to sleep. To awaken all processes waiting in the queue, you should use wake_up_interruptible_all.
In fact, the main functions are wait_event, wake_up, and wake_up_all.
They are used with processes in the queue in an exclusive
(uninterruptible) wait, since they can't be interrupted by the signal. They should be used only for critical tasks. Interruptible functions are just optional (but recommended). Since they can be interrupted by signals,
you should check their return value. A nonzero value means your sleep has been interrupted by some sort of signal, and the driver should return
ERESTARTSYS.
If something has called wake_up or wake_up_interruptible and the condition is still
FALSE, then nothing will happen. Without wake_up (or wake_up_interuptible),
processes will never be awakened. Here is an example of a wait queue:
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
printk("Waitqueue module handler %s\n", __FUNCTION__);
msleep(5000);
printk("Wake up the sleeping module\n");
```
condition = 1;
```c
wake_up_interruptible(&my_wq);
}
static int __init my_init(void)
{
printk("Wait queue example\n");
INIT_WORK(&wrk, work_handler);
schedule_work(&wrk);
printk("Going to sleep %s\n", __FUNCTION__);
wait_event_interruptible(my_wq, condition != 0);
pr_info("woken up by the work job\n");
return 0;
}
void my_exit(void)
{
printk("waitqueue example cleanup\n");
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("John Madieu <john.madieu@foobar.com>");
MODULE_LICENSE("GPL");
```
In the preceding example, the current process (actually insmod) will be put to sleep in the wait queue for 5 seconds and woken up by the work handler. The dmesg output is as follows:
[342081.385491] Wait queue example
[342081.385505] Going to sleep my_init
[342081.385515] Waitqueue module handler work_handler
[342086.387017] Wake up the sleeping module
[342086.387096] woken up by the work job
[342092.912033] waitqueue example cleanup
## Delay and timer management
Time is one of the most used resources, right after memory. It is used to do almost everything: defer work, sleep, scheduling, timeout, and many other tasks.
There are two categories of time. The kernel uses absolute time to know what time it is, that is, the date and time of the day, whereas relative time is used by, for example, the kernel scheduler. For absolute time, there is a hardware chip called the real-time clock (RTC). We will deal with such devices later in the book in Chapter 18, RTC Drivers. On the other hand,
to handle relative time, the kernel relies on a CPU feature (peripheral) called a timer, which,
from the kernel's point of view, is called a kernel timer. Kernel timers are what we will talk about in this section.
Kernel timers are classified into two different parts:
Standard timers, or system timers
High-resolution timers
## Standard timers
Standard timers are kernel timers operating on the granularity of jiffies.
## Jiffies and HZ
A jiffy is a kernel unit of time declared in <linux/jiffies.h>. To understand jiffies, we need to introduce a new constant, HZ, which is the number of times jiffies is incremented in one second. Each increment is called a tick. In other words, HZ represents the size of a jiffy. HZ depends on the hardware and on the kernel version, and also determines how frequently the clock interrupt fires. This is configurable on some architectures, fixed on other ones.
What it means is that jiffies is incremented HZ times every second. If HZ = 1,000, then it is incremented 1,000 times (that is, one tick every 1/1,000 seconds). Once defined, the programmable interrupt timer (PIT), which is a hardware component, is programmed with that value in order to increment jiffies when the PIT interrupt comes in.
Depending on the platform, jiffies can lead to overflow. On a 32-bit system, HZ = 1,000 will result in about 50 days, duration only, whereas the duration is about 600 million years on a
64-bit system. By storing jiffies in a 64-bit variable, the problem is solved. A second variable has then been introduced and defined in <linux/jiffies.h>:
```c
extern u64 jiffies_64;
```
In this manner, on 32-bit systems jiffies will point to low-order 32 bits, and jiffies_64
will point to high-order bits. On 64-bit platforms, jiffies = jiffies_64.
## The timer API
A timer is represented in the kernel as an instance of timer_list:
```c
#include <linux/timer.h>
struct timer_list {
struct list_head entry;
unsigned long expires;
struct tvec_t_base_s *base;
void (*function)(unsigned long);
unsigned long data;
```
);
expires is an absolute value in jiffies. entry is a doubly linked list and data is optional,
and passed to the callback function.
## Timer setup initialization
The following steps initialize timers:
1. Setting up the timer: Set up the timer, feeding the user-defined callback and data:
```c
void setup_timer( struct timer_list *timer, \
void (*function)(unsigned long), \
unsigned long data);
```
You can also use this:
```c
void init_timer(struct timer_list *timer);
```
setup_timer is a wrapper around init_timer.
2. Setting the expiration time: When the timer is initialized, we need to set its expiration before the callback gets fired:
```c
int mod_timer( struct timer_list *timer, unsigned long expires);
```
3. Releasing the timer: When you are done with the timer, it needs to be released:
```c
void del_timer(struct timer_list *timer);
int del_timer_sync(struct timer_list *timer);
```
del_timer returns void whether it has deactivated a pending timer or not. Its return value is 0 on an inactive timer, or 1 on an active one. The last,
del_timer_sync, waits for the handler to finish its execution, even if that happens on another CPU. You should not hold a lock preventing the handler's completion, otherwise it will result in a deadlock. You should release the timer in the module cleanup routine. You can independently check whether the timer is running or not:
```c
int timer_pending( const struct timer_list *timer);
```
This function checks whether there are any fired timer callbacks pending.
## Standard timer example
The standard timer example is as follows:
```c
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
static struct timer_list my_timer;
void my_timer_callback(unsigned long data)
{
printk("%s called (%ld).\n", __FUNCTION__, jiffies);
}
static int __init my_init(void)
{
int retval;
printk("Timer module loaded\n");
setup_timer(&my_timer, my_timer_callback, 0);
printk("Setup timer to fire in 300ms (%ld)\n", jiffies);
```
retval = mod_timer( &my_timer, jiffies + msecs_to_jiffies(300) );
```c
if (retval)
printk("Timer firing failed\n");
return 0;
}
static void my_exit(void)
{
int retval;
```
retval = del_timer(&my_timer);
/* Is timer still active (1) or no (0) */
```c
if (retval)
printk("The timer is still in use...\n");
pr_info("Timer module unloaded\n");
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Standard timer example");
MODULE_LICENSE("GPL");
```
## High-resolution timers (HRTs)
Standard timers are less accurate and do not suit real-time applications. HRTs, introduced in kernel v2.6.16 (and enabled by the CONFIG_HIGH_RES_TIMERS option in the kernel configuration) have a resolution in microseconds (up to nanoseconds, depending on the platform), compared to milliseconds on standard timers. The standard timer depends on
HZ (since they rely on jiffies), whereas the HRT implementation is based on ktime.
The kernel and hardware must support an HRT before being used on your system. In other words, there must be architecture-dependent code implemented to access your hardware
HRTs.
## HRT API
The required headers are:
```c
#include <linux/hrtimer.h>
```
An HRT is represented in the kernel as an instance of hrtimer:
```c
struct hrtimer {
struct timerqueue_node node;
```
ktime_t _softexpires;
enum hrtimer_restart (*function)(struct hrtimer *);
```c
struct hrtimer_clock_base *base;
```
u8 state;
u8 is_rel;
```c
};
```
## HRT setup initialization
The HRT setup initialization is done as follows:
1. Initializing the hrtimer: Before hrtimer initialization, you need to set up a ktime, which represents time duration. We will see how to achieve that in the following example:
```c
void hrtimer_init( struct hrtimer *time, clockid_t which_clock,
```
enum hrtimer_mode mode);
2. Starting hrtimer: hrtimer can be started as shown in the following example:
```c
int hrtimer_start( struct hrtimer *timer, ktime_t time,
```
const enum hrtimer_mode mode);
mode represents the expiry mode. It should be HRTIMER_MODE_ABS for an absolute time value, or HRTIMER_MODE_REL for a time value relative to now.
3. hrtimer cancellation: You can either cancel the timer or see whether it is possible to cancel it:
```c
int hrtimer_cancel( struct hrtimer *timer);
int hrtimer_try_to_cancel(struct hrtimer *timer);
```
Both return 0 when the timer is not active and 1 when the timer is active. The difference between these two functions is that hrtimer_try_to_cancel fails if the timer is active or its callback is running, returning -1, whereas hrtimer_cancel will wait until the callback finishes.
We can independently check whether the hrtimer's callback is still running with the following:
```c
int hrtimer_callback_running(struct hrtimer *timer);
```
Remember, hrtimer_try_to_cancel internally calls hrtimer_callback_running.
In order to prevent the timer from automatically restarting, the hrtimer callback function must return HRTIMER_NORESTART.
You can check whether HRTs are available on your system by doing the following:
By looking in the kernel config file, which should contain something like
CONFIG_HIGH_RES_TIMERS=y: zcat /proc/configs.gz | grep
CONFIG_HIGH_RES_TIMERS.
By looking at the cat /proc/timer_list or cat /proc/timer_list | grep resolution result. The .resolution entry must show 1 nsecs and the event_handler must show hrtimer_interrupts.
By using the clock_getres system call.
From within the kernel code, by using #ifdef CONFIG_HIGH_RES_TIMERS.
With HRTs enabled on your system, the accuracy of sleep and timer system calls does not depend on jiffies anymore, but they are still as accurate as HRTs are. It is the reason why some systems do not support nanosleep(), for example.
## Dynamic tick/tickless kernel
With the previous HZ options, the kernel is interrupted HZ times per second in order to reschedule tasks, even in an idle state. If HZ is set to 1,000, there will be 1,000 kernel interruptions per second, preventing the CPU from being idle for a long time, thus affecting
CPU power consumption.
Now, let's look at a kernel with no fixed or predefined ticks, where the ticks are disabled until some task needs to be performed. We call such a kernel a tickless kernel. In fact, tick activation is scheduled, based on the next action. The right name should be dynamic tick kernel. The kernel is responsible for task scheduling, and maintains a list of runnable tasks
(the run queue) in the system. When there is no task to schedule, the scheduler switches to the idle thread, which enables dynamic tick by disabling periodic tick until the next timer expires (a new task is queued for processing).
Under the hood, the kernel also maintains a list of the task timeouts (it then knows when and how long it has to sleep). In an idle state, if the next tick is further away than the lowest timeout in the tasks list timeout, the kernel programs the timer with that timeout value.
When the timer expires, the kernel re-enables periodic ticks and invokes the scheduler,
which then schedules the task associated with the timeout. This is how the tickless kernel removes periodic ticks and saves power when idle.
## Delays and sleep in the kernel
Without going into the details, there are two types of delays, depending on the context your code runs in: atomic or nonatomic. The mandatory header to handle delays in the kernel is
```c
#include <linux/delay>.
```
## Atomic context
Tasks in the atomic context (such as ISR) can't sleep, and can't be scheduled; it is the reason why busy-wait loops are used for delaying purposes in an atomic context. The kernel exposes the Xdelay family of functions that will spend time in a busy loop, long enough(based on jiffies) to achieve the desired delay:
```c
ndelay(unsigned long nsecs)
udelay(unsigned long usecs)
mdelay(unsigned long msecs)
```
You should always use udelay() since ndelay() precision depends on how accurate your hardware timer is (not always the case on an embedded SOC). Use of mdelay() is also discouraged.
Timer handlers (callbacks) are executed in an atomic context, meaning that sleeping is not allowed at all. By sleeping, I mean any function that may result in sending the caller to sleep,
such as allocating memory, locking a mutex, an explicit call to the sleep() function, and so on.
## Nonatomic context
In a nonatomic context, the kernel provides the sleep[_range] family of functions and which function to use depends on how long you need to delay by:
udelay(unsigned long usecs): Busy-wait loop-based. You should use this function if you need to sleep for a few µsecs ( < ~10 us ).
usleep_range(unsigned long min, unsigned long max): Relies on hrtimers, and it is recommended to let this sleep for a few ~µsecs or small msecs
(10 us - 20 ms), avoiding the busy-wait loop of udelay().
msleep(unsigned long msecs): Backed by jiffies/legacy_timers. You should use this for larger, msecs sleep (10 ms+).
Sleep and delay topics are well explained in
Documentation/timers/timers-howto.txt in the kernel source.
## Kernel locking mechanism
Locking is a mechanism that helps shares resources between different threads or processes.
A shared resource is data or a device that can be accessed by at least two users,
simultaneously or not. Locking mechanisms prevent abusive access, for example, a process writing data when another one is reading in the same place, or two processes accessing the same device (the same GPIO, for example). The kernel provides several locking mechanisms. The most important are:
## Mutex
Semaphore
## Spinlock
We will only learn about mutexes and spinlocks, since they are widely used in device drivers.
## Mutex
Mutual exclusion (mutex) is the de facto, most-used locking mechanism. To understand how it works, let's see what its structure looks like in include/linux/mutex.h:
```c
struct mutex {
```
/* 1: unlocked, 0: locked, negative: locked, possible waiters */
atomic_t count;
spinlock_t wait_lock;
```c
struct list_head wait_list;
```
[...]
```c
};
```
As we have seen in the wait queue section, there is also a list type field in the structure:
```c
wait_list. The principle of sleeping is the same.
```
Contenders are removed from the scheduler run queue and put onto the wait list
(wait_list) in a sleep state. The kernel then schedules and executes other tasks. When the lock is released, a waiter in the wait queue is woken, moved off the wait_list, and scheduled back.
## Mutex API
Using a mutex requires only a few basic functions.
Declare
Statically:
```c
DEFINE_MUTEX(my_mutex);
```
Dynamically:
```c
struct mutex my_mutex;
mutex_init(&my_mutex);
```
## Acquire and release
Lock:
```c
void mutex_lock(struct mutex *lock);
int mutex_lock_interruptible(struct mutex *lock);
int mutex_lock_killable(struct mutex *lock);
```
Unlock:
```c
void mutex_unlock(struct mutex *lock);
```
Sometimes, you may only need to check whether a mutex is locked or not. For that purpose, you can use the int mutex_is_locked(struct mutex *lock) function:
```c
int mutex_is_locked(struct mutex *lock);
```
What this function does is just check whether the mutex's owner is empty (NULL) or not.
There is also mutex_trylock, which acquires the mutex if it is not already locked and returns 1; otherwise, it returns 0:
```c
int mutex_trylock(struct mutex *lock);
```
As with the wait queue's interruptible family function, mutex_lock_interruptible(),
which is recommended, will result in the driver being able to be interrupted by any signal,
whereas, with mutex_lock_killable(), only signals killing the process can interrupt the driver.
You should be very careful with mutex_lock(), and use it when you can guarantee that the mutex will be released, whatever happens. In the user context, it is recommended you always use mutex_lock_interruptible() to acquire the mutex, since mutex_lock()
will not return if a signal is received (even with a Ctrl + C).
Here is an example of a mutex implementation:
```c
struct mutex my_mutex;
mutex_init(&my_mutex);
```
/* inside a work or a thread */
```c
mutex_lock(&my_mutex);
access_shared_memory();
mutex_unlock(&my_mutex);
```
Please have a look at include/linux/mutex.h in the kernel source to see the strict rules you must respect with mutexes. The following are some of them:
Only one task can hold the mutex at a time; this is actually not a rule, but a fact
Multiple unlocks are not permitted
They must be initialized through the API
A task holding the mutex may not exit, since the mutex will remain locked, and possible contenders will wait (sleep) forever
Memory areas where held locks reside must not be freed
Held mutexes must not be reinitialized
Since they involve rescheduling, mutexes may not be used in atomic contexts,
such as tasklets and timers
As with wait_queue, there is no polling mechanism with mutexes. Every time that mutex_unlock is called on a mutex, the kernel checks for waiters in wait_list. If any one (and only one) of them is awakened and scheduled, they are woken in the same order in which they were put to sleep.
## Spinlock
Like mutex, spinlock is a mutual exclusion mechanism; it only has two states:
```c
locked (acquired)
unlocked (released)
```
Any thread that needs to acquire the spinlock will active-loop until the lock is acquired,
which breaks out of the loop. This is the point where mutex and spinlock differ. Since spinlock heavily consumes the CPU while looping, it should be used for very quick acquires, especially when the time to hold the spinlock is less than the time to reschedule.
Spinlock should be released as soon as the critical task is done.
In order to avoid wasting CPU time by scheduling a thread that may probably spin, trying to acquire a lock held by another thread moved off the run queue, the kernel disables preemption whenever code holding a spinlock is running. With preemption disabled, we prevent the spinlock holder from being moved off the run queue, which could lead waiting processes to spin for a long time and consume CPU.
As long as you hold a spinlock, other tasks may be spinning while waiting on it. By using spinlock, you assert and guarantee that it will not be held for a long time. You can say it is better to spin in a loop, wasting CPU time, than the cost of sleeping your thread, contextshifting to another thread or process, and being woken up afterward. Spinning on a processor means no other task can run on that processor; it then makes no sense to use spinlock on a single core machine. In the best case, you will slow down the system; in the worst case, you will deadlock, as with mutexes. For this reason, the kernel just disables preemption in response to the spin_lock(spinlock_t *lock) function on a single processor. On a single processor (core) system, you should use spin_lock_irqsave()
and spin_unlock_irqrestore(), which will respectively disable the interrupts on the
CPU, preventing interrupt concurrency.
Since you do not know in advance what system you will write the driver for, it is recommended you acquire a spinlock using spin_lock_irqsave(spinlock_t *lock,
```c
unsigned long flags), which disables interrupts on the current processor (the processor where it is called) before taking the spinlock. spin_lock_irqsave internally calls local_irq_save(flags);, an architecture-dependent function to save the IRQ status,
```
and preempt_disable() to disable preemption on the relevant CPU. You should then release the lock with spin_unlock_irqrestore(), which does the reverse operations that we previously enumerated. This is code that does lock acquisition and release. It is an IRQ
handler, but let's just focus on the lock aspect. We will discuss IRQ handlers more in the next section:
/* some where */
spinlock_t my_spinlock;
```c
spin_lock_init(my_spinlock);
static irqreturn_t my_irq_handler(int irq, void *data)
{
unsigned long status, flags;
spin_lock_irqsave(&my_spinlock, flags);
```
status = access_shared_resources();
```c
spin_unlock_irqrestore(&gpio->slock, flags);
return IRQ_HANDLED;
}
```
## Spinlock versus mutexes
Used for concurrency in the kernel, spinlocks and mutexes both have their own objectives:
Mutexes protect the process's critical resources, whereas spinlocks protect the
IRQ handler's critical sections
Mutexes put contenders to sleep until the lock is acquired, whereas spinlocks infinitely spin in a loop (consuming CPU) until the lock is acquired
Because of the previous point, you can't hold spinlocks for a long time, since waiters will waste CPU time waiting for the lock, whereas a mutex can be held as long as the resource needs to be protected, since contenders are put to sleep in a wait queue
When dealing with spinlocks, please keep in mind that preemption is disabled only for threads holding spinlocks, not for spinning waiters.
## Work deferring mechanism
Deferring is a method by which you schedule a piece of work to be executed in the future.
It's a way to report an action later. Obviously, the kernel provides facilities to implement such a mechanism; it allows you to defer functions, whatever their type, to be called and executed later. There are three of them in the kernel:
SoftIRQs: Executed in an atomic context
Tasklets: Executed in an atomic context
Workqueues: Executed in a process context
## Softirqs and ksoftirqd
A software IRQ (softirq), or software interrupt is a deferring mechanism used only for very fast processing, since it runs with a disabled scheduler (in an interrupt context). You'll rarely (almost never) want to deal with softirq directly. There are only networks and block device subsystems using softirq. Tasklets are an instantiation of softirqs, and will be sufficient in almost every case when you feel the need to use softirqs.
## ksoftirqd
In most cases, softirqs are scheduled in hardware interrupts, which may arrive very quickly, faster than they can be serviced. They are then queued by the kernel in order to be processed later. Ksoftirqds are responsible for late execution (process context this time). A
ksoftirqd is a per-CPU kernel thread raised to handle unserved software interrupts:
In the preceding top sample from my personal computer, you can see ksoftirqd/n entries, where n is the CPU number that the ksoftirqds runs on. CPU-consuming ksoftirqd may indicate an overloaded system or a system under interrupts storm, which is never good. You can have a look at kernel/softirq.c to see how ksoftirqds are designed.
## Tasklets
Tasklets are a bottom-half (we will see what this means later) mechanism built on top of softirqs. They are represented in the kernel as instances of the struct tasklet_struct:
```c
struct tasklet_struct
{
struct tasklet_struct *next;
unsigned long state;
```
atomic_t count;
```c
void (*func)(unsigned long);
unsigned long data;
};
```
Tasklets are not re-entrant by nature. Code is called reentrant if it can be interrupted anywhere in the middle of its execution, and then be safely called again. Tasklets are designed such that a tasklet can run on one and only one CPU simultaneously (even on an
SMP system), which is the CPU it was scheduled on, but different tasklets may be run simultaneously on different CPUs. The tasklet API is quite basic and intuitive.
## Declaring a tasklet
Dynamically:
```c
void tasklet_init(struct tasklet_struct *t,
void (*func)(unsigned long), unsigned long data);
```
Statically:
```c
DECLARE_TASKLET( tasklet_example, tasklet_function, tasklet_data );
DECLARE_TASKLET_DISABLED(name, func, data);
```
There is one difference between the two functions: the former creates a tasklet already enabled and ready to be scheduled without any other function call, done by setting the count field to 0, whereas the latter creates a tasklet disabled (done by setting count to 1),
on which you have to call tasklet_enable() before the tasklet can be schedulable:
```c
#define DECLARE_TASKLET(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(0), func, data }
#define DECLARE_TASKLET_DISABLED(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(1), func, data }
```
Globally, setting the count field to 0 means that the tasklet is disabled and cannot be executed, whereas a nonzero value means the opposite.
## Enabling and disabling a tasklet
There is one function to enable a tasklet:
```c
void tasklet_enable(struct tasklet_struct *);
tasklet_enable simply enables the tasklet. In older kernel versions, you may find void tasklet_hi_enable(struct tasklet_struct *) is used, but those two functions do exactly the same thing. To disable a tasklet, call:
void tasklet_disable(struct tasklet_struct *);
```
You can also call:
```c
void tasklet_disable_nosync(struct tasklet_struct *);
tasklet_disable will disable the tasklet and return only when the tasklet has terminated its execution (if it was running), whereas tasklet_disable_nosync returns immediately,
```
even if the termination has not occurred.
## Tasklet scheduling
There are two scheduling functions for a tasklet, depending on whether your tasklet has normal or higher priority:
```c
void tasklet_schedule(struct tasklet_struct *t);
void tasklet_hi_schedule(struct tasklet_struct *t);
```
The kernel maintains normal priority and high priority tasklets in two different lists.
```c
tasklet_schedule adds the tasklet into the normal priority list, scheduling the associated softirq with a TASKLET_SOFTIRQ flag. With tasklet_hi_schedule, the tasklet is added into the high priority list, scheduling the associated softirq with a HI_SOFTIRQ flag. High priority tasklets are meant to be used for soft interrupt handlers with low latency requirements. There are some properties associated with tasklets you should know:
```
Calling tasklet_schedule on a tasklet already scheduled, but whose execution has not started, will do nothing, resulting in the tasklet being executed only once.
```c
tasklet_schedule can be called in a tasklet, meaning that a tasklet can reschedule itself.
```
High priority tasklets are always executed before normal ones. Abusive use of high priority tasks will increase system latency. Only use them for really quick stuff.
You can stop a tasklet using the tasklet_kill function, which will prevent the tasklet from running again or wait for its completion before killing it if the tasklet is currently scheduled to run:
```c
void tasklet_kill(struct tasklet_struct *t);
```
Let's check. Look at the following example:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h> /* for tasklets API */
```
char tasklet_data[]="We use a string; but it could be pointer to a structure";
/* Tasklet handler, that just print the data */
```c
void tasklet_work(unsigned long data)
{
printk("%s\n", (char *)data);
}
DECLARE_TASKLET(my_tasklet, tasklet_function, (unsigned long)
tasklet_data);
static int __init my_init(void)
{
```
/*
* Schedule the handler.
* Tasklet are also scheduled from interrupt handler
*/
```c
tasklet_schedule(&my_tasklet);
return 0;
}
void my_exit(void)
{
tasklet_kill(&my_tasklet);
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
## Work queues
Added since Linux kernel 2.6, the most used and simple deferring mechanism is the work queue. It is the last one we will talk about in this chapter. As a deferring mechanism, it takes an opposite approach to the others we've seen, running only in a pre-emptible context. It is the only choice when you need to sleep in your bottom half (I will explain what a bottom half is later in the next section). By sleep, I mean process I/O data, hold mutexes, delay, and all the other tasks that may lead to sleep or move the task off the run queue.
Keep in mind that work queues are built on top of kernel threads, and this is the reason why I decided not to talk about the kernel thread as a deferring mechanism at all. However,
there are two ways to deal with work queues in the kernel. First, there is a default shared work queue, handled by a set of kernel threads, each running on a CPU. Once you have work to schedule, you queue that work into the global work queue, which will be executed at the appropriate moment. The other method is to run the work queue in a dedicated kernel thread. It means whenever your work queue handler needs to be executed, your kernel thread is woken up to handle it, instead of one of the default predefined threads.
Structures and functions to call are different, depending on whether you chose a shared work queue or dedicated ones.
## Kernel-global work queue – the shared queue
Unless you have no choice, you need critical performance, or you need to control everything from work queue initialization to work scheduling, and if you only submit tasks occasionally, you should use the shared work queue provided by the kernel. With that queue being shared over the system, you should be nice, and should not monopolize the queue for a long time.
Since the execution of the pending task on the queue is serialized on each CPU, you should not sleep for a long time because no other task on the queue will run until you wake up.
You won't even know who you share the work queue with, so don't be surprised if your task takes longer to get the CPU. Work in shared work queues is executed in a per-CPU
thread called events/n, created by the kernel.
In this case, the work must also be initialized with the INIT_WORK macro. Since we are going to use the shared work queue, there is no need to create a work queue structure. We only need the work_struct structure that will be passed as an argument. There are three functions to schedule work on the shared work queue:
The version that ties the work on the current CPU:
```c
int schedule_work(struct work_struct *work);
```
The same, but delayed, function:
```c
static inline bool schedule_delayed_work(struct delayed_work
```
*dwork,unsigned long delay)
The function that actually schedules the work on a given CPU:
```c
int schedule_work_on(int cpu, struct work_struct *work);
```
The same as shown previously, but with a delay:
```c
int scheduled_delayed_work_on(int cpu, struct delayed_work *dwork,
unsigned long delay);
```
All of these functions schedule the work given as an argument on the system's shared work queue, system_wq, defined in kernel/workqueue.c:
```c
struct workqueue_struct *system_wq __read_mostly;
EXPORT_SYMBOL(system_wq);
```
Work already submitted to the shared queue can be canceled with the cancel_delayed_work function. You can flush the shared work queue with:
```c
void flush_scheduled_work(void);
```
Since the queue is shared over the system, you can't really know how long flush_scheduled_work() may last before it returns:
```c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h> /* for sleep */
#include <linux/wait.h> /* for wait queue */
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/slab.h> /* for kmalloc() */
#include <linux/workqueue.h>
```
//static DECLARE_WAIT_QUEUE_HEAD(my_wq);
```c
static int sleep = 0;
struct work_data {
struct work_struct my_work;
wait_queue_head_t my_wq;
int the_data;
};
static void work_handler(struct work_struct *work)
{
struct work_data *my_data = container_of(work, \
struct work_data, my_work);
printk("Work queue module handler: %s, data is %d\n", __FUNCTION__,
```
my_data->the_data);
```c
msleep(2000);
wake_up_interruptible(&my_data->my_wq);
kfree(my_data);
}
static int __init my_init(void)
{
struct work_data * my_data;
```
my_data = kmalloc(sizeof(struct work_data), GFP_KERNEL);
my_data->the_data = 34;
```c
INIT_WORK(&my_data->my_work, work_handler);
init_waitqueue_head(&my_data->my_wq);
schedule_work(&my_data->my_work);
printk("I'm going to sleep ...\n");
wait_event_interruptible(my_data->my_wq, sleep != 0);
printk("I am Waked up...\n");
return 0;
}
static void __exit my_exit(void)
{
printk("Work queue module exit: %s %d\n", __FUNCTION__, __LINE__);
}
module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com> ");
MODULE_DESCRIPTION("Shared workqueue");
```
In order to pass data to my work queue handler, you may have noticed that, in both examples, I've embedded my work_struct structure inside my custom data structure and used container_of to retrieve it. This is a common way to pass data to the work queue handler.
## Dedicated work queue
Here, the work queue is represented as an instance of struct workqueue_struct. The work to be queued into the work queue is represented as an instance of struct work_struct. There are four steps involved prior to scheduling your work in your own kernel thread:
1. Declare/initialize a struct workqueue_struct
2. Create your work function
3. Create a struct work_struct so that your work function will be embedded into it
4. Embed your work function in the work_struct
## Programming syntax
The following functions are defined in include/linux/workqueue.h:
Declare the work and work queue:
```c
struct workqueue_struct *myqueue;
struct work_struct thework;
```
Define the worker function (the handler):
```c
void dowork(void *data) { /* Code goes here */ };
```
Initialize our work queue and embed our work into it:
myqueue = create_singlethread_workqueue( "mywork" );
```c
INIT_WORK( &thework, dowork, <data-pointer> );
```
We could have also created our work queues through a macro called create_workqueue. The difference between create_workqueue and create_singlethread_workqueue is that the former will create a work queue that in turn will create a separate kernel thread on each and every processor available.
Scheduling work:
```c
queue_work(myqueue, &thework);
```
Queue after the given delay to the given worker thread:
```c
queue_dalayed_work(myqueue, &thework, <delay>);
```
These functions return false if the work was already on a queue and true if otherwise. delay represents the number of jiffies to wait before queueing. You may use the helper function msecs_to_jiffies in order to convert the standard ms delay into jiffies. For example, to queue work after 5 ms, you can use queue_delayed_work(myqueue, &thework, msecs_to_jiffies(5));.
Wait on all pending work on the given work queue:
```c
void flush_workqueue(struct workqueue_struct *wq)
```
flush_workqueue sleeps until all queued work has finished its execution. New incoming (enqueued) work does not affect the sleep. You may typically use this in driver shutdown handlers.
Cleanup: Use cancel_work_sync() or cancel_delayed_work_sync for synchronous cancellation, which will cancel the work if it is not already running,
or block until the work has completed. The work will be cancelled even if it requeues itself. You must also ensure that the work queue on which the work was last queued can't be destroyed before the handler returns. These functions are to be used for nondelayed or delayed work, respectively:
```c
int cancel_work_sync(struct work_struct *work);
int cancel_delayed_work_sync(struct delayed_work *dwork);
```
Since Linux kernel v4.8, it has been possible to use cancel_work or cancel_delayed_work, which are asynchronous forms of cancellation. You must check whether the function returns true or not, and makes sure the work does not requeue itself.
You must then explicitly flush the work queue:
```c
if ( !cancel_delayed_work( &thework) ){
flush_workqueue(myqueue);
destroy_workqueue(myqueue);
}
```
The other is a different version of the same method and will create only a single thread for all the processors. If you need a delay before the work is enqueued, feel free to use the following work initialization macro:
```c
INIT_DELAYED_WORK(_work, _func);
INIT_DELAYED_WORK_DEFERRABLE(_work, _func);
```
Using the preceding macros would imply that you should use the following functions to queue or schedule the work in the work queue:
```c
int queue_delayed_work(struct workqueue_struct *wq,
struct delayed_work *dwork, unsigned long delay)
queue_work ties the work to the current CPU. You can specify the CPU on which the handler should run using the queue_work_on function:
int queue_work_on(int cpu, struct workqueue_struct *wq,
struct work_struct *work);
```
For delayed work, you can use:
```c
int queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
struct delayed_work *dwork, unsigned long delay);
```
The following is an example of using a dedicated work queue:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h> /* for work queue */
#include <linux/slab.h> /* for kmalloc() */
struct workqueue_struct *wq;
struct work_data {
struct work_struct my_work;
int the_data;
};
static void work_handler(struct work_struct *work)
{
struct work_data * my_data = container_of(work,
struct work_data, my_work);
printk("Work queue module handler: %s, data is %d\n",
```
__FUNCTION__, my_data->the_data);
```c
kfree(my_data);
}
static int __init my_init(void)
{
struct work_data * my_data;
printk("Work queue module init: %s %d\n",
```
__FUNCTION__, __LINE__);
wq = create_singlethread_workqueue("my_single_thread");
my_data = kmalloc(sizeof(struct work_data), GFP_KERNEL);
my_data->the_data = 34;
```c
INIT_WORK(&my_data->my_work, work_handler);
queue_work(wq, &my_data->my_work);
return 0;
}
static void __exit my_exit(void)
{
flush_workqueue(wq);
destroy_workqueue(wq);
printk("Work queue module exit: %s %d\n",
```
__FUNCTION__, __LINE__);
```c
}
module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
```
Predefined (shared) workqueue and standard workqueue functions
The predefined work queue is defined in kernel/workqueue.c as follows:
```c
struct workqueue_struct *system_wq __read_mostly;
```
This is just standard work for which the kernel provides a custom API that simply wraps around the standard one.
Comparisons between kernel predefined work queue functions and standard work queue functions are shown as follows:
Predefined work queue function Equivalent standard work queue function schedule_work(w) queue_work(keventd_wq,w)
```c
schedule_delayed_work(w,d) queue_delayed_work(keventd_wq,w,d)(on any CPU)
schedule_delayed_work_on(cpu,w,d) queue_delayed_work(keventd_wq,w,d) (on a given CPU)
```
flush_scheduled_work() flush_workqueue(keventd_wq)
## Kernel threads
Work queues run on top of kernel threads. You already use kernel threads when you use work queues. This is why I have decided not to talk about the kernel thread API.
## Kernel interruption mechanism
An interrupt is the way a device halts the kernel, telling it that something interesting or important has happened. These are called IRQs on Linux systems. The main advantage interrupts offer is to avoid device polling. It is up to the device to announce any change in its state; it is not up to us to poll it.
In order to get notified when an interrupt occurs, you need to register to that IRQ,
providing a function called an interrupt handler that will be called every time that interrupt is raised.
## Registering an interrupt handler
You can register a callback to be run when the interruption (or interrupt line) you are interested in gets fired. You can achieve that with the request_irq() function, declared in <linux/interrupt.h>:
```c
int request_irq(unsigned int irq, irq_handler_t handler,
unsigned long flags, const char *name, void *dev)
request_irq() may fail, and returns 0 on success. Other elements of the preceding code are outlined in detail as follows:
```
flags: These should be a bitmask of the masks defined in
<linux/interrupt.h>. The most used are:
IRQF_TIMER: Informs the kernel that this handler was originated by a system timer interrupt.
IRQF_SHARED: Used for interrupt lines that can be shared by two or more devices. Each device sharing the same line must have this flag set. If omitted, only one handler can be registered for the specified IRQ line.
IRQF_ONESHOT: Used essentially in the threaded IRQ. It instructs the kernel not to re-enable the interrupt when the hardirq handler has finished. It will remain disabled until the threaded handler has been run.
In older kernel versions (until v2.6.35), there were IRQF_DISABLED
flags, which asked the kernel to disable all interrupts when the handler is running. This flag is no longer used.
name: This is used by the kernel to identify your driver in /proc/interrupts and /proc/irq.
dev: Its primary goal is to be passed as an argument to the handler. This should be unique to each registered handler, since it is used to identify the device. It can be NULL for nonshared IRQs, but not for shared ones. The common way of using it is to provide a device structure, since it is both unique and potentially useful to the handler. That said, a pointer to any per-device data structure is sufficient:
```c
struct my_data {
struct input_dev *idev;
struct i2c_client *client;
```
char name[64];
char phys[32];
```c
};
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
struct my_data *md = dev_id;
unsigned char nextstate = read_state(lp);
```
/* Check whether my device raised the irq or no */
[...]
```c
return IRQ_HANDLED;
}
```
/* some where in the code, in the probe function */
```c
int ret;
struct my_data *md;
```
md = kzalloc(sizeof(*md), GFP_KERNEL);
ret = request_irq(client->irq, my_irq_handler,
IRQF_TRIGGER_LOW | IRQF_ONESHOT,
DRV_NAME, md);
/* far in the release function */
```c
free_irq(client->irq, md);
```
handler: This field is of type irq_handler_t, defined in include/linux/interrupt.h as: typedef irqreturn_t
(*irq_handler_t)(int, void *). It corresponds to a pointer to a function return an irqreturn_t, and taking an int and a void * as parameters. An interrupt handler prototype should then look like:
```c
static irqreturn_t my_irq_handler(int irq, void *dev)
```
This contains the following code elements:
irq: The numeric value of the IRQ (the same as used in request_irq)
dev: The same as used in request_irq
Both parameters are given to your handler by the kernel. There are only two values the handler can return, depending on whether your device originated the IRQ or not:
IRQ_NONE: Your device is not the originator of that interrupt (it especially happens on shared IRQ lines)
IRQ_HANDLED: Your device caused the interrupt
Depending on the processing, you may use the IRQ_RETVAL(val) macro, which will return IRQ_HANDLED if the value is nonzero, or IRQ_NONE otherwise.
When writing the interrupt handler, you don't have to worry about reentrance, since the IRQ line serviced is disabled on all processors by the kernel in order to avoid recursive interrupts.
The associated function to free the previously registered handler is:
```c
void free_irq(unsigned int irq, void *dev)
```
If the specified IRQ is not shared, free_irq will not only remove the handler, but will also disable the line. If it is shared, only the handler identified through dev (which should be the same as that used in request_irq) is removed, but the interrupt line still remains, and will be disabled only when the last handler is removed. free_irq will block until any executing interrupts for the specified IRQ have completed. You must then avoid both request_irq and free_irq in the interrupt context.
## Interrupt handler and lock
It goes without saying that you are in an atomic context and must only use spinlock for concurrency. Whenever there is global data accessible by both user code (the user task, that is, the system call) and interrupt code, this shared data should be protected by spin_lock_irqsave() in the user code. Let's see why we can't just use spin_lock. An interrupt handler will always have priority on the user task, even if that task is holding a spinlock. Simply disabling IRQ is not sufficient. An interrupt may happen on another CPU.
It would be a disaster if a user task updating the data were interrupted by an interrupt handler trying to access the same data. Using spin_lock_irqsave() will disable all interrupts on the local CPU, preventing the system call from being interrupted by any kind of interrupt:
ssize_t my_read(struct file *filp, char __user *buf, size_t count,
loff_t *f_pos)
```c
{
unsigned long flags;
```
/* some stuff */
[...]
```c
unsigned long flags;
spin_lock_irqsave(&my_lock, flags);
```
data++;
```c
spin_unlock_irqrestore(&my_lock, flags)
```
[...]
```c
}
static irqreturn_t my_interrupt_handler(int irq, void *p)
{
```
/*
* preemption is disabled when running interrupt handler
* also, the serviced irq line is disabled until the handler has completed
* no need then to disable all other irq. We just use spin_lock and
* spin_unlock
*/
```c
spin_lock(&my_lock);
```
/* process data */
[...]
```c
spin_unlock(&my_lock);
return IRQ_HANDLED;
}
```
When sharing data between different interrupt handlers (that is, the same driver managing two or more devices, with each having its own IRQ line), you should also protect that data with spin_lock_irqsave() in those handlers, in order to prevent the other IRQs from being triggered and uselessly spinning.
## Concept of bottom halves
Bottom halves are mechanisms by which you split interrupt handlers into two parts. This introduces another term, which is top half. Before discussing each of them, let's talk about their origin, and what problem they solve.
## The problem – interrupt handler design limitations
Whether an interrupt handler holds a spinlock or not, preemption is disabled on the CPU
running that handler. The more you waste time in the handler, the less CPU time is granted to the other task, which may considerably increase the latency of other interrupts and so increase the latency of the whole system. The challenge is to acknowledge the device that raised the interrupt as quickly as possible in order to keep the system responsive.
On Linux systems (actually on all OSes, by hardware design), any interrupt handler runs with its current interrupt line disabled on all processors, and sometimes you may need to disable all interrupts on the CPU actually running the handler, but you definitely don't want to miss an interrupt. To meet this need, the concept of halves has been introduced.
## The solution – bottom halves
This idea consists of splitting the handler into two parts:
The first part, called the top half or hard-IRQ, which is the registered function using request_irq() that will eventually mask/hide interrupts (on the current
CPU, except the one being serviced since it is already disabled by the kernel before running the handler) depending on the needs, performs quick and fast operations (essentially time-sensitive tasks, read/write hardware registers, and fast processing of this data), schedules the second and next part, and then acknowledges the line. All interrupts that are disabled must have been reenabled just before exiting the bottom half.
The second part, called the bottom half, will process time-consuming stuff, and run with the interrupt re-enabled. This way, you have the chance not to miss an interrupt.
Bottom halves are designed using a work-deferring mechanism, which we have seen previously. Depending on which one you choose, it may run in a (software) interrupt context, or in a process context. Bottom half mechanisms are:
Softirqs
## Tasklets
Workqueues
## Threaded IRQs
Softirqs and tasklets execute in a (software) interrupt context (meaning that preemption is disabled), Workqueues and threaded IRQs are executed in a process (or simply task)
context, and can be preempted, but nothing prevents you from changing their real-time properties to fit your needs and their preemption behavior (see CONFIG_PREEMPT or
CONFIG_PREEMPT_VOLUNTARY; this also impacts the whole system). Bottom halves are not always possible. But when possible, they are certainly the best thing to do.
## Tasklets as bottom halves
The tasklet deferring mechanism is most used in DMA, network, and block device drivers.
Just try the following command in the kernel source:
```bash
grep -rn tasklet_schedule
```
Now, let's see how to implement such a mechanism in our interrupt handler:
```c
struct my_data {
int my_int_var;
struct tasklet_struct the_tasklet;
int dma_request;
};
static void my_tasklet_work(unsigned long data)
{
```
/* Do what ever you want here */
```c
}
struct my_data *md = init_my_data;
```
/* somewhere in the probe or init function */
[...]
```c
tasklet_init(&md->the_tasklet, my_tasklet_work,
```
(unsigned long)md);
[...]
```c
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
struct my_data *md = dev_id;
```
/* Let's schedule our tasklet */
```c
tasklet_schedule(&md.dma_tasklet);
return IRQ_HANDLED;
}
```
In the preceding sample, our tasklet will execute the my_tasklet_work() function.
## Workqueue as bottom halves
Let's just start with a sample:
```c
static DECLARE_WAIT_QUEUE_HEAD(my_wq); /* declare and init the wait queue
```
*/
```c
static struct work_struct my_work;
```
/* some where in the probe function */
/*
* work queue initialization. "work_handler" is the call back that will be
* executed when our work is scheduled.
*/
```c
INIT_WORK(my_work, work_handler);
static irqreturn_t my_interrupt_handler(int irq, void *dev_id)
{
```
uint32_t val;
```c
struct my_data = dev_id;
```
val = readl(my_data->reg_base + REG_OFFSET);
```c
if (val == 0xFFCD45EE)) {
```
my_data->done = true;
```c
wake_up_interruptible(&my_wq);
} else {
schedule_work(&my_work);
}
return IRQ_HANDLED;
};
```
In the preceding sample, we used either a wait queue or a work queue in order to wake up a possibly sleeping process waiting for us, or schedule work depending on the value of a register. We have no shared data or resource, so there is no need to disable all other IRQs
(spin_lock_irq_disable).
## Softirqs as bottom half
As said at the beginning of this chapter, we will not discuss softirq. Tasklets will be enough wherever you feel the need to use softirqs. Anyway, let's talk about their defaults.
Softirqs run in a software interrupt context, with preemption disabled, holding the CPU
until they complete. Softirqs should be fast; otherwise they may slow the system down.
When, for any reason, a softirq prevents the kernel from scheduling other tasks, any new incoming softirq will be handled by ksoftirqd threads, running in a process context.
## Threaded IRQs
The main goal of threaded IRQs is to reduce the time spent with interrupts disabled to a bare minimum. With threaded IRQs, the way you register an interrupt handler is a bit simplified. You do not even have to schedule the bottom half yourself. The core does that for us. The bottom half is then executed in a dedicated kernel thread. We do not use request_irq() anymore, but request_threaded_irq():
```c
int request_threaded_irq(unsigned int irq, irq_handler_t handler,\
```
irq_handler_t thread_fn, \
```c
unsigned long irqflags, \
```
const char *devname, void *dev_id)
The request_threaded_irq() function accepts two functions in its parameters:
@handler function: This is the same function as the one registered with request_irq(). It represents the top-half function, which runs in an atomic context (or hard-IRQ). If it can process the interrupt faster so that you can get rid of the bottom half at all, it should return IRQ_HANDLED. But, if the interrupt processing needs more than 100 µs, as discussed previously, you should use the bottom half. In this case, it should return IRQ_WAKE_THREAD, which will result in scheduling the thread_fn function that must have been provided.
@thread_fn function: This represents the bottom half, as you would have scheduled in your top half. When the hard-IRQ handler (handler function)
function returns IRQ_WAKE_THREAD, the kthread associated with this bottom half will be scheduled, invoking the thread_fn function when it comes to run the ktread. The thread_fn function must return IRQ_HANDLED when complete.
After being executed, the kthread will not be rescheduled again until the IRQ is triggered again and the hard-IRQ returns IRQ_WAKE_THREAD.
Wherever you would have used the work queue to schedule the bottom half, threaded
IRQs can be used. handler and thread_fn must be defined in order to have a proper threaded IRQ. A default hard-IRQ handler will be installed by the kernel if handler is
NULL and thread_fn != NULL (see the following code), which will simply return
IRQ_WAKE_THREAD to schedule the bottom half. handler is always called in an interrupt context, whether it has been provided by you or by the kernel by default:
/*
* Default primary interrupt handler for threaded interrupts. Is
* assigned as primary handler when request_threaded_irq is called
* with handler == NULL. Useful for one shot interrupts.
*/
```c
static irqreturn_t irq_default_primary_handler(int irq, void *dev_id)
{
return IRQ_WAKE_THREAD;
}
request_threaded_irq(unsigned int irq, irq_handler_t handler,
```
irq_handler_t thread_fn, unsigned long irqflags,
const char *devname, void *dev_id)
```c
{
```
[...]
```c
if (!handler) {
if (!thread_fn)
return -EINVAL;
```
handler = irq_default_primary_handler;
```c
}
```
[...]
```c
}
EXPORT_SYMBOL(request_threaded_irq);
```
With threaded IRQs, the handler definition does not change, but the way it is registered changes a little bit:
```c
request_irq(unsigned int irq, irq_handler_t handler, \
unsigned long flags, const char *name, void *dev)
{
return request_threaded_irq(irq, handler, NULL, flags, \
```
name, dev);
```c
}
```
## Threaded bottom half
The following simple excerpt is a demonstration of how you can implement the threaded bottom half mechanism:
```c
static irqreturn_t pcf8574_kp_irq_handler(int irq, void *dev_id)
{
struct custom_data *lp = dev_id;
unsigned char nextstate = read_state(lp);
if (lp->laststate != nextstate) {
int key_down = nextstate < ARRAY_SIZE(lp->btncode);
unsigned short keycode = key_down ?
```
p->btncode[nextstate] : lp->btncode[lp->laststate];
```c
input_report_key(lp->idev, keycode, key_down);
input_sync(lp->idev);
```
lp->laststate = nextstate;
```c
}
return IRQ_HANDLED;
}
static int pcf8574_kp_probe(struct i2c_client *client, \
```
const struct i2c_device_id *id)
```c
{
struct custom_data *lp = init_custom_data();
```
[...]
/*
* @handler is NULL and @thread_fn != NULL
* the default primary handler is installed, which will
* return IRQ_WAKE_THREAD, that will schedule the thread
* associated to the bottom half. the bottom half must then
* return IRQ_HANDLED when finished
*/
ret = request_threaded_irq(client->irq, NULL, \
pcf8574_kp_irq_handler, \
IRQF_TRIGGER_LOW | IRQF_ONESHOT, \
DRV_NAME, lp);
```c
if (ret) {
```
dev_err(&client->dev, "IRQ %d is not free\n", \
client->irq);
goto fail_free_device;
```c
}
```
ret = input_register_device(idev);
[...]
```c
}
```
When an interrupt handler is executed, the serviced IRQ is always disabled on all CPUs, and re-enabled when the hard-IRQ (top half)
finishes. But if for any reason you need the IRQ line not to be re-enabled after the top half, and to remain disabled until the threaded handler has been run, you should request the threaded IRQ with the flag
IRQF_ONESHOT enabled (by just doing an OR operation as shown previously). The IRQ line will then be re-enabled after the bottom half has finished.
Invoking user space applications from the kernel
User-space applications are, most of the time, called from within the user space by other applications. Without going into the details, let's see an example:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h> /* for work queue */
#include <linux/kmod.h>
static struct delayed_work initiate_shutdown_work;
static void delayed_shutdown( void )
{
```
char *cmd = "/sbin/shutdown";
char *argv[] = {
cmd,
"-h",
"now",
NULL,
```c
};
```
char *envp[] = {
"HOME=/",
"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
NULL,
```c
};
call_usermodehelper(cmd, argv, envp, 0);
}
static int __init my_shutdown_init( void )
{
schedule_delayed_work(&delayed_shutdown, msecs_to_jiffies(200));
return 0;
}
static void __exit my_shutdown_exit( void )
{
```
return;
```c
}
module_init( my_shutdown_init );
module_exit( my_shutdown_exit );
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu", <john.madieu@gmail.com>);
MODULE_DESCRIPTION("Simple module that trigger a delayed shut down");
```
In the preceding example, the API used (call_usermodehelper) is a part of the
Usermode-helper API, with all functions defined in kernel/kmod.c. Its use is quite simple; just a look inside kmod.c will give you an idea. You may be wondering what this
API was defined for. It is used by the kernel, for example, for module (un)loading and cgroup management.
## Summary
In this chapter, we discussed the fundamental elements with which to start driver development, presenting every mechanism frequently used in drivers. This chapter is very important, since it discusses topics other chapters in this book rely on. The next chapter, for example, dealing with character devices, will use some elements discussed in this chapter