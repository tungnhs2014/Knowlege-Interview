# Chapter 4 - Character Device Drivers 

Character devices transfer data to or from a user application by means of characters, in a stream manner (one character after another), like a serial port does. A character device driver exposes the properties and functionalities of a device by means of a special file in the
/dev directory, which you can use to exchange data between the device and user application, and which also allows you to control the real physical device. This is the basic concept of Linux that says everything is a file. A character device driver represents the most basic device driver in the kernel source. Character devices are represented in the kernel as instances of struct cdev, defined in include/linux/cdev.h:
```c
struct cdev {
struct kobject kobj;
struct module *owner;
```
const struct file_operations *ops;
```c
struct list_head list;
```
dev_t dev;
```c
unsigned int count;
};
```
This chapter will walk through the specifics of character device drivers; explain how they create, identify, and register the devices with the system; and also give a better overview of the device file methods, which are methods by which the kernel exposes the device capabilities to user space, accessible by using file-related system calls (read, write,
select, open, close, and so on), described in struct file_operations structures,
which you have certainly heard of before.
## The concept behind major and minor
Character devices are populated in the /dev directory. Do note that they are not only files present in that directory. A character device file is recognizable by its type, which we can display thanks to the ls -l command. Major and minor identify and tie the devices with the drivers. Let's see how it works by listing the content of the /dev directory (ls -l
/dev):
[...]
drwxr-xr-x 2 root root 160 Mar 21 08:57 input crw-r----- 1 root kmem 1, 2 Mar 21 08:57 kmem lrwxrwxrwx 1 root root 28 Mar 21 08:57 log -> /run/systemd/journal/dev-log crw-rw---- 1 root disk 10, 237 Mar 21 08:57 loop-control brw-rw---- 1 root disk 7, 0 Mar 21 08:57 loop0
brw-rw---- 1 root disk 7, 1 Mar 21 08:57 loop1
brw-rw---- 1 root disk 7, 2 Mar 21 08:57 loop2
brw-rw---- 1 root disk 7, 3 Mar 21 08:57 loop3
Given the preceding excerpt, the first character of the first column identifies the file type.
Possible values are:
c: This is for character device files b: This is for block device file l: This is for symbolic link d: This is for directory s: This is for socket p: This is for named pipe
For b and c file types, the fifth and sixth columns right before the date respect the <X, Y>
pattern. X represents the major, and Y is the minor. For example, the third line is <1, 2>
and the last one is <7, 3>. That is one of the classic methods for identifying a character device file from user space, as well as its major and minor.
The kernel holds the numbers that identify a device in dev_t type variables, which are simply u32 (32-bit unsigned long). The major is represented with only 12 bits, whereas the minor is coded on the 20 remaining bits.
As one can see in include/linux/kdev_t.h, given a dev_t type variable, you may need to extract the minor or the major. The kernel provides a macro for these purposes:
```c
MAJOR(dev_t dev);
MINOR(dev_t dev);
```
On the other hand, you may have a minor and a major, and need to build a dev_t. The macro you should use is MKDEV(int major, int minor);:
```c
#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)
#define MAJOR(dev) ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))
```
The device is registered with a major number that identifies the device, and a minor, which you may use as an array index to a local list of devices, since one instance of the same driver may handle several devices while different drivers may handle different devices of the same type.
## Device number allocation and freeing
Device numbers identify device files across the system. That means there are two ways to allocate these device numbers (actually major and minor):
Statically: Guessing a major not yet used by another driver using the register_chrdev_region() function. You should avoid using this as much as possible. Its prototype looks this:
```c
int register_chrdev_region(dev_t first, unsigned int count, \
```
char *name);
This method returns 0 on success, or a negative error code on failure. first is made of the major number that we need along with the first minor of the desired range. You should use MKDEV(ma,mi). count is the number of consecutive device numbers required, and name should be the name of the associated device or driver.
Dynamically: Letting the kernel do the job for us, using the alloc_chrdev_region() function. This is the recommended way to obtain a valid device number. Its prototype is as follows:
```c
int alloc_chrdev_region(dev_t *dev, unsigned int firstminor, \
unsigned int count, char *name);
```
This method returns 0 on success, or a negative error code on failure. dev is the only output parameter. It represents the first number the kernel assigned.
firstminor is the first of the requested range of minor numbers, count the number of minors you require, and name should be the name of the associated device or driver.
The difference between the two is that with the former, you should know in advance what number we need. This is registration: you tell the kernel what device numbers you want.
This may be used for pedagogic purposes, and works as long as the only user of the driver is you. When it comes to loading the driver on another machine, there is no guarantee the chosen number is free on that machine, and this will lead to conflicts and trouble. The second method is cleaner and much safer, since the kernel is responsible for guessing the right numbers for us. We do not even have to bother about what the behavior would be on loading the module on to another machine, since the kernel will adapt accordingly.
Anyway, the preceding functions are generally not called directly from the driver, but masked by the framework on which the driver relies (IIO framework, input framework,
RTC, and so on), by means of a dedicated API. These frameworks are all discussed in further chapters in this book.
## Introduction to device file operations
Operations that you can perform on files depend on the drivers that manage those files.
Such operations are defined in the kernel as instances of struct file_operations.
```c
struct file_operations exposes a set of callbacks that will handle any user space system call on a file. For example, if you want users to be able to perform a write on the file representing your device, you must implement the callback corresponding to that write function and add it into the struct file_operations that will be tied to your device. Let's fill in a file operations structure:
struct file_operations {
struct module *owner;
loff_t (*llseek) (struct file *, loff_t, int);
ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t (*write) (struct file *, const char __user *, size_t, loff_t
```
*);
```c
unsigned int (*poll) (struct file *, struct poll_table_struct *);
int (*mmap) (struct file *, struct vm_area_struct *);
int (*open) (struct inode *, struct file *);
long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
int (*release) (struct inode *, struct file *);
int (*fsync) (struct file *, loff_t, loff_t, int datasync);
int (*fasync) (int, struct file *, int);
int (*lock) (struct file *, int, struct file_lock *);
int (*flock) (struct file *, int, struct file_lock *);
```
[...]
```c
};
```
The preceding excerpt only lists important methods of the structure, especially the ones that are relevant for the needs of this book. You can find the full description in include/linux/fs.h in kernel sources. Each of these callbacks is linked with a system call, and none of them is mandatory. When a user code calls a file-related system call on a given file, the kernel looks for the driver responsible for that file (especially the one that created the file), locates its struct file_operations structure, and checks whether the method that matches the system call is defined or not. If yes, it simply runs it. If not, it returns an error code that varies depending on the system call. For example, an undefined
(*mmap) method will return -ENODEV to user, whereas an undefined (*write) method will return -EINVAL.
## File representation in the kernel
The kernel describes files as instances of the struct inode (not struct file) structure, defined in include/linux/fs.h:
```c
struct inode {
```
[...]
```c
struct pipe_inode_info *i_pipe; /* Set and used if this is a
```
*linux kernel pipe */
```c
struct block_device *i_bdev; /* Set and used if this is a
```
* a block device */
```c
struct cdev *i_cdev; /* Set and used if this is a
```
* character device */
[...]
```c
}
struct inode is a filesystem data structure holding information, which is only relevant to the OS, about a file (its type, character, block, pipe, and so on) or directory (yes!! from a kernel point of view, a directory is a file that on entry points to other files) on disk.
```
The struct file structure (also defined in include/linux/fs.h) is actually a higher level of file description that represents an open file in the kernel and that relies on the lower struct inode data structure:
```c
struct file {
```
[...]
```c
struct path f_path; /* Path to the file */
struct inode *f_inode; /* inode associated to this file */
```
const struct file_operations *f_op;/* operations that can be
* performed on this file
*/
```c
loff_t f_pos; /* Position of the cursor in
```
* this file */
/* needed for tty driver, and maybe others */
```c
void *private_data; /* private data that driver can set
```
* in order to share some data between file
* operations. This can point to any data
* structure.
*/
[...]
```c
}
```
The difference between struct inode and struct file is that an inode doesn't track the current position within the file or the current mode. It only contains stuff that helps the OS
find the contents of the underlying file structure (pipe, directory, regular disk file,
block/character device file, and so on). On the other hand, struct file is used as a generic structure (it actually holds a pointer to a struct inode structure) that represents and open file and provides a set of functions related to methods you can perform on the underlying file structure. Such methods are open, write, seek, read, select, and so on.
All this reinforces the philosophy of UNIX systems that says everything is a file.
In other words, a struct inode represents a file in the kernel, and a struct file describes it when it is actually open. There may be different file descriptors that represent the same file opened several times, but these will point to the same inode.
Allocating and registering a character device
Character devices are represented in the kernel as instances of struct cdev. When writing a character device driver, your goal is to finally create and register an instance of that structure associated with struct file_operations, exposing a set of operations
(functions) the user space can perform on the device. To reach that goal, there are some steps we must go through, which are as follows:
1. Reserve a major and a range of minors with alloc_chrdev_region().
2. Create a class for your devices with class_create(), visible in /sys/class/.
3. Set up a struct file_operation (to be given to cdev_init), and for each device you need to create, call cdev_init() and cdev_add() to register the device.
4. Then, create a device_create() for each device, with a proper name. It will result in your device being created in the /dev directory:
```c
#define EEP_NBANK 8
#define EEP_DEVICE_NAME "eep-mem"
#define EEP_CLASS "eep-class"
struct class *eep_class;
struct cdev eep_cdev[EEP_NBANK];
```
dev_t dev_num;
```c
static int __init my_init(void)
{
int i;
```
dev_t curr_dev;
/* Request the kernel for EEP_NBANK devices */
```c
alloc_chrdev_region(&dev_num, 0, EEP_NBANK, EEP_DEVICE_NAME);
```
/* Let's create our device's class, visible in /sys/class */
eep_class = class_create(THIS_MODULE, EEP_CLASS);
/* Each eeprom bank represented as a char device (cdev) */
```c
for (i = 0; i < EEP_NBANK; i++) {
```
/* Tie file_operations to the cdev */
```c
cdev_init(&my_cdev[i], &eep_fops);
```
eep_cdev[i].owner = THIS_MODULE;
/* Device number to use to add cdev to the core */
curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
/* Now make the device live for the users to access */
```c
cdev_add(&eep_cdev[i], curr_dev, 1);
```
/* create a device node each device /dev/eep-mem0, /dev/eep-mem1,
* With our class used here, devices can also be viewed under
* /sys/class/eep-class.
*/
```c
device_create(eep_class,
```
NULL, /* no parent device */
curr_dev,
NULL, /* no additional data */
EEP_DEVICE_NAME "%d", i); /* eep-mem[0-7] */
```c
}
return 0;
}
```
## Writing file operations
After introducing the preceding file operations, it is time to implement them in order to enhance the driver capabilities and expose the device's methods to the user space (by means of system calls of course). Each of these methods has its particularities, which we will highlight in this section.
Exchanging data between kernel space and user space
This section does not describe any driver file operation but instead introduces some kernel facilities that you may use to write these driver methods. The driver's write() method consists of reading data from user space to kernel space, and then processing that data from the kernel. Such processing could be something like pushing the data to the device, for example. On the other hand, the driver's read() method consists of copying data from the kernel to the user space. Both of these methods introduces new elements we need to discuss prior to jumping to their respective steps.
The first one is __user. __user is a cookie used by sparse (a semantic checker used by the kernel to find possible coding faults) to let the developer know they are actually about to use an untrusted pointer (or a pointer that may be invalid in the current virtual address mapping) improperly and that they should not dereference but instead use dedicated kernel functions to access the memory to which this pointer points.
This allows us to introduce different kernel functions needed to access such memory, either to read or write. These are copy_from_user() and copy_to_user() respectively to copy a buffer from user space to kernel space, and vice versa, to copy a buffer from kernel to user space:
```c
unsigned long copy_from_user(void *to, const void __user *from,
unsigned long n)
unsigned long copy_to_user(void __user *to, const void *from,
unsigned long n)
```
In both cases, pointers prefixed with __user point to user space (untrusted) memory. n represents the number of bytes to copy. from represents the source address, and to is the destination address. Each of these returns the number of bytes that could not be copied. On success, the return value should be 0.
Please do note that with copy_to_user(), if some data could not be copied, the function will pad the copied data to the requested size using zero bytes.
## A single value copy
When it comes to copying single and simple variables, such as char and int, but not larger data types, such as structures or arrays, the kernel offers dedicated macros in order to quickly perform the desired operation. These macros are put_user(x, ptr) and get_user(x, ptr), which are explained as follows:
put_user(x, ptr);: This macro copies a variable from kernel space to user space. x represents the value to copy to user space, and ptr is the destination address in user space. The macro returns 0 on success, or -EFAULT on error. x must be assignable to the result of dereferencing ptr. In other words, they must have (or point to) the same type.
get_user(x, ptr);: This macro copies a variable from user space to kernel space, and returns 0 on success or -EFAULT on error. Please do note that x is set to 0 on error. x represents the kernel variable to store the result, and ptr is the source address in user space. The result of dereferencing ptr must be assignable to x without a cast. Guess what it means.
## The open method open is the method called every time someone opens your device's file. Device opening will always be successful in cases where this method is not defined. You usually use this method to perform device and data structure initialization, and return a negative error code if something goes wrong, or 0. The prototype of the open method is defined as follows:
```c
int (*open)(struct inode *inode, struct file *filp);
```
## Per-device data
For each open performed on your character device, the callback function will be given a struct inode as a parameter, which is the kernel lower-level representation of the file.
That struct inode structure has a field named i_cdev, which points to the cdev we have allocated in the init function. By embedding the struct cdev in our device-specific data,
as in struct pcf2127 in the following example, we will be able to get a pointer on that specific data using the container_of macro. Here is an open method sample.
The following is our data structure:
```c
struct pcf2127 {
struct cdev cdev;
unsigned char *sram_data;
struct i2c_client *client;
int sram_size;
```
[...]
```c
};
```
Given this data structure, the open method would look like this:
```c
static unsigned int sram_major = 0;
static struct class *sram_class = NULL;
static int sram_open(struct inode *inode, struct file *filp)
{
unsigned int maj = imajor(inode);
unsigned int min = iminor(inode);
struct pcf2127 *pcf = NULL;
```
pcf = container_of(inode->i_cdev, struct pcf2127, cdev);
pcf->sram_size = SRAM_SIZE;
```c
if (maj != sram_major || min < 0 ){
pr_err ("device not found\n");
return -ENODEV; /* No such device */
}
```
/* prepare the buffer if the device is opened for the first time */
```c
if (pcf->sram_data == NULL) {
```
pcf->sram_data = kzalloc(pcf->sram_size, GFP_KERNEL);
```c
if (pcf->sram_data == NULL) {
pr_err("Open: memory allocation failed\n");
return -ENOMEM;
}
}
```
filp->private_data = pcf;
```c
return 0;
}
```
## The release method
The release method is called when the device gets closed, the reverse of the open method.
You must then undo everything you have done in the open task. What you have to do is roughly:
1. Free any private memory allocated during the open() step
2. Shut down the device (if supported) and discard every buffer on the last closing
(if the device supports multi opening, or if the driver can handle more than one device at a time)
The following is an excerpt from a release function:
```c
static int sram_release(struct inode *inode, struct file *filp)
{
struct pcf2127 *pcf = NULL;
```
pcf = container_of(inode->i_cdev, struct pcf2127, cdev);
```c
mutex_lock(&device_list_lock);
```
filp->private_data = NULL;
/* last close? */
pcf2127->users--;
```c
if (!pcf2127->users) {
kfree(tx_buffer);
kfree(rx_buffer);
```
tx_buffer = NULL;
rx_buffer = NULL;
[...]
```c
if (any_global_struct)
kfree(any_global_struct);
}
mutex_unlock(&device_list_lock);
return 0;
}
```
## The write method
The write() method is used to send data to the device; whenever a user app calls the write function on the device's file, the kernel implementation is called. Its prototype is as follows:
```c
ssize_t(*write)(struct file *filp, const char __user *buf, size_t count,
loff_t *pos);
```
The return value is the number of bytes (size) written
*buf represents the data buffer coming from the user space count is the size of the requested transfer
*pos indicates the start position from which data should be written in the file
## Steps to write
The following steps do not describe any standard nor universal method to implement the driver's write() method. They are just an overview of what kinds of operation you can perform in this method:
1. Check for bad or invalid requests coming from the user space. This step is relevant only if the device exposes its memory (eeprom, I/O memory, and so on),
which may have size limitations:
/* if trying to Write beyond the end of the file, return error.
* "filesize" here corresponds to the size of the device memory (if any)
*/
```c
if ( *pos >= filesize ) return -EINVAL;
```
2. Adjust count for the remaining bytes in order to not go beyond the file size. This step is not mandatory either, and is relevant in the same condition as step 1:
/* filesize coerresponds to the size of device memory */
```c
if (*pos + count > filesize)
```
count = filesize - *pos;
3. Find the location from which you will start to write. This step is relevant only if the device has a memory in which the write() method is supposed to write given data. As with steps 1 and 2, this step is not mandatory:
/* convert pos into valid address */
```c
void *from = pos_to_address( *pos );
```
4. Copy data from the user space and write it into the appropriate kernel space:
```c
if (copy_from_user(dev->buffer, buf, count) != 0){
```
retval = -EFAULT;
goto out;
```c
}
```
/* now move data from dev->buffer to physical device */
5. Write to the physical device and return an error on failure:
write_error = device_write(dev->buffer, count);
```c
if ( write_error )
return -EFAULT;
```
6. Increase the current position of the cursor in the file, according to the number of bytes written. Finally, return the number of bytes copied:
*pos += count;
```c
return count;
```
The following is an example of the write method. Once again, this is intended to give an overview:
```c
ssize_t eeprom_write(struct file *filp, const char __user *buf, size_t count,
loff_t *f_pos)
{
struct eeprom_dev *eep = filp->private_data;
ssize_t retval = 0;
```
/* step (1) */
```c
if (*f_pos >= eep->part_size)
```
/* Writing beyond the end of a partition is not allowed. */
```c
return -EINVAL;
```
/* step (2) */
```c
if (*pos + count >= eep->part_size)
```
count = eep->part_size - *pos;
/* step (3) */
```c
int part_origin = PART_SIZE * eep->part_index;
int register_address = part_origin + *pos;
```
/* step(4) */
/* Copy data from user space to kernel space */
```c
if (copy_from_user(eep->data, buf, count) != 0)
return -EFAULT;
```
/* step (5) */
/* perform the write to the device */
```c
if (write_to_device(register_address, buff, count) < 0){
pr_err("ee24lc512: i2c_transfer failed\n");
return -EFAULT;
}
```
/* step (6) */
*f_pos += count;
```c
return count;
}
```
## The read method
The prototype of the read() method is given as follows:
```c
ssize_t (*read) (struct file *filp, char __user *buf, size_t count, loff_t
```
*pos);
The return value is the size read. The rest of the method's elements are described here:
*buf is the buffer we receive from the user space count is the size of the requested transfer (size of the user buffer)
*pos indicates the start position from which data should be read in the file
## Steps to read
1. Prevent from reading beyond the file size, and return end-of-file:
```c
if (*pos >= filesize)
return 0; /* 0 means EOF */
```
2. The number of bytes read can't go beyond the file size. Adjust count appropriately:
```c
if (*pos + count > filesize)
```
count = filesize - (*pos);
3. Find the location from which you will start the read:
```c
void *from = pos_to_address (*pos); /* convert pos into valid address */
```
4. Copy the data into the user space buffer and return an error on failure:
sent = copy_to_user(buf, from, count);
```c
if (sent)
return -EFAULT;
```
5. Advance the file's current position according to the number of bytes read, and return the number of bytes copied:
*pos += count;
```c
return count;
```
The following is an example of a driver read() file operation, which is intended to give an overview of what can be done there:
```c
ssize_t eep_read(struct file *filp, char __user *buf, size_t count, loff_t
```
*f_pos)
```c
{
struct eeprom_dev *eep = filp->private_data;
if (*f_pos >= EEP_SIZE) /* EOF */
return 0;
if (*f_pos + count > EEP_SIZE)
```
count = EEP_SIZE - *f_pos;
/* Find location of next data bytes */
```c
int part_origin = PART_SIZE * eep->part_index;
int eep_reg_addr_start = part_origin + *pos;
```
/* perform the read from the device */
```c
if (read_from_device(eep_reg_addr_start, buff, count) < 0){
pr_err("ee24lc512: i2c_transfer failed\n");
return -EFAULT;
}
```
/* copy from kernel to user space */
if(copy_to_user(buf, dev->data, count) != 0)
```c
return -EIO;
```
*f_pos += count;
```c
return count;
}
```
## The llseek method
The llseek function is called when you move the cursor position within a file. The entry point of this method in user space is lseek(). You can refer to the man page in order to print the full description of either method from user space: man llseek and man lseek.
Its prototype looks as follows:
```c
loff_t(*llseek) (struct file *filp, loff_t offset, int whence);
```
The preceding command is explained as following:
The return value is the new position in the file.
```c
loff_t is an offset, relative to the current file position, which defines how much it will be changed.
```
whence defines where to seek from. Possible values are:
SEEK_SET: This puts the cursor into a position relative to the beginning of the file
SEEK_CUR: This puts the cursor into a position relative to the current file position
SEEK_END: This adjusts the cursor to a position relative to end of the file
## Steps to llseek
1. Use the switch statement to check every possible whence case, since they are limited, and adjust newpos accordingly:
```c
switch( whence ){
case SEEK_SET:/* relative from the beginning of file */
```
newpos = offset; /* offset become the new position */
break;
```c
case SEEK_CUR: /* relative to current file position */
```
newpos = file->f_pos + offset; /* just add offset to the current position */
break;
```c
case SEEK_END: /* relative to end of file */
```
newpos = filesize + offset;
break;
default:
```c
return -EINVAL;
}
```
2. Check whether newpos is valid:
```c
if ( newpos < 0 )
return -EINVAL;
```
3. Update f_pos with the new position:
filp->f_pos = newpos;
4. Return the new file-pointer position:
```c
return newpos;
```
The following is an example of a user program that successively reads and seeks into a file.
The underlying driver will then execute the llseek() file operation entry:
```c
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#define CHAR_DEVICE "toto.txt"
int main(int argc, char **argv)
{
int fd= 0;
```
char buf[20];
```c
if ((fd = open(CHAR_DEVICE, O_RDONLY)) < -1)
return 1;
```
/* Read 20 bytes */
```c
if (read(fd, buf, 20) != 20)
return 1;
printf("%s\n", buf);
```
/* Move the cursor to 10 time, relative to its actual position */
```c
if (lseek(fd, 10, SEEK_CUR) < 0)
return 1;
if (read(fd, buf, 20) != 20)
return 1;
printf("%s\n",buf);
```
/* Move the cursor ten time, relative from the beginning of the file */
```c
if (lseek(fd, 7, SEEK_SET) < 0)
return 1;
if (read(fd, buf, 20) != 20)
return 1;
printf("%s\n",buf);
close(fd);
return 0;
}
```
The code produces the following output:
jma@jma:~/work/tutos/sources$ cat toto.txt
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
jma@jma:~/work/tutos/sources$ ./seek
Lorem ipsum dolor si nsectetur adipiscing psum dolor sit amet,
jma@jma:~/work/tutos/sources$
## The poll method
If you need to implement a passive wait (not wasting CPU cycles while sensing the character device), you must implement the poll() function, which will be called whenever a user space program performs select() or poll() system calls on the file associated with the device:
```c
unsigned int (*poll) (struct file *, struct poll_table_struct *);
```
The kernel function at the heart of this method is poll_wait(), defined in
<linux/poll.h>, which is the header you should include in driver code:
```c
void poll_wait(struct file * filp, wait_queue_head_t * wait_address,
poll_table *p)
poll_wait() adds the device associated with a struct file structure (given as the first parameter) to a list of those that can wake up processes (which have been put to sleep in the struct wait_queue_head_t structure given as the second parameter), according to events registered in the struct poll_table structure given as the third parameter. A
```
user process can run poll(), select(), or epoll() system calls to add a set of files to a list on which it needs to wait, in order to be aware of the associated devices' readiness (if any). The kernel will then call the poll entry of the driver associated with each device file.
The poll method of each driver should then call poll_wait() in order to register events for which the process needs to be notified by the kernel, put that process to sleep until one of these events occurs, and register the driver as one of those that can wake the process up.
The usual way is to use a wait queue per event type (one for readability, another one for writability, and eventually one for exception if needed), according to events supported by the select() (or poll()) system call.
The return value of the (*poll) file operation must have POLLIN | POLLRDNORM set if there is data to read (at the moment, select or poll is called), POLLOUT | POLLWRNORM if the device is writable (at the moment, select or poll is called here as well), and 0 if there is no new data and the device is not yet writable. In the following example, we assume the device supports both blocking read and write. Of course, you may implement only one of these. If the driver does not define this method, the device will be considered as always readable and writable, so that poll() or select() system calls return immediately.
## Steps to poll
When you implement the poll function, either the read or write method may change:
1. Declare a wait queue for each event type (read, write, exception) you need to implement passive wait, to put tasks in when there is no data to read, or when the device is not yet writable:
```c
static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static DECLARE_WAIT_QUEUE_HEAD(my_rq);
```
2. Implement the poll function like this:
```c
#include <linux/poll.h>
static unsigned int eep_poll(struct file *file, poll_table *wait)
{
unsigned int reval_mask = 0;
poll_wait(file, &my_wq, wait);
poll_wait(file, &my_rq, wait);
if (new-data-is-ready)
```
reval_mask |= (POLLIN | POLLRDNORM);
```c
if (ready_to_be_written)
```
reval_mask |= (POLLOUT | POLLWRNORM);
```c
return reval_mask;
}
```
3. Notify the wait queue when there is new data or when the device is writable:
```c
wake_up_interruptible(&my_rq); /* Ready to read */
wake_up_interruptible(&my_wq); /* Ready to be written to */
```
You can notify readable events either from within the driver's write() method, meaning that the written data can be read back, or from within an IRQ handler, meaning that an external device sent some data that can be read back. On the other hand, you can notify writable events either from within the driver's read() method, meaning that the buffer is empty and can be filled again, or from within an IRQ handler, meaning that the device has completed a data-send operation and is ready to accept data again.
When using a sleepy input/output operation (blocked I/O), either the read or write method may change. The wait queue used in the poll must be used in read too. When the user needs to read, if there is data, that data will be sent immediately to the process and you must update the wait queue condition (set to false); if there is no data, the process is put to sleep in the wait queue.
If the write method is supposed to feed data, then in the write callback, you must fill the data buffer, update the wait queue condition (set to true), and wake up the reader (see the section wait queue). If it is an IRQ instead, these operations must be performed in their handler.
The following is an excerpt of code that uses select() on a given char device in order to sense data availability:
```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#define NUMBER_OF_BYTE 100
#define CHAR_DEVICE "/dev/packt_char"
```
char data[NUMBER_OF_BYTE];
```c
int main(int argc, char **argv)
{
int fd, retval;
ssize_t read_count;
```
fd_set readfds;
fd = open(CHAR_DEVICE, O_RDONLY);
```c
if(fd < 0)
```
/* Print a message and exit*/
[...]
```c
while(1){
FD_ZERO(&readfds);
FD_SET(fd, &readfds);
```
/*
* One needs to be notified of "read" events only, without timeout.
* This call will put the process to sleep until it is notified the
* event for which it registered itself
*/
ret = select(fd + 1, &readfds, NULL, NULL, NULL);
/* From this line, the process has been notified already */
```c
if (ret == -1) {
```
fprintf(stderr, "select call on %s: an error occurred",
CHAR_DEVICE);
break;
```c
}
```
/*
* file descriptor is now ready.
* This step assume we are interested in one file only.
*/
```c
if (FD_ISSET(fd, &readfds)) {
```
read_count = read(fd, data, NUMBER_OF_BYTE);
```c
if (read_count < 0 )
```
/* An error occurred. Handle this */
[...]
```c
if (read_count != NUMBER_OF_BYTE)
```
/* We have read less than need bytes */
[...] /* handle this */
else
/* Now we can process data we have read */
[...]
```c
}
}
close(fd);
return EXIT_SUCCESS;
}
```
## The ioctl method
A typical Linux system contains around 350 system calls (syscalls), but only a few of them are linked with file operations. Sometimes devices may need to implement specific commands that are not provided by system calls, and especially the ones associated with files and thus device files. In this case, the solution is to use input/output control(ioctl),
which is a method by which you extend a list of syscalls (actually commands) associated with a device. You can use it to send special commands to devices (reset, shutdown,
configure, and so on). If the driver does not define this method, the kernel will return an -ENOTTY error to any ioctl() system call.
In order to be valid and safe, an ioctl command needs to be identified by a number,
which should be unique to the system. The uniqueness of ioctl numbers across the system will prevent it from sending the right command to the wrong device, or passing the wrong argument to the right command (given a duplicated ioctl number). Linux provides four helper macros to create an ioctl identifier, depending on whether there is data transfer or not and on the direction of the transfer. Their respective prototypes are:
```c
_IO(MAGIC, SEQ_NO)
_IOW(MAGIC, SEQ_NO, TYPE)
_IOR(MAGIC, SEQ_NO, TYPE)
_IORW(MAGIC, SEQ_NO, TYPE)
```
Their descriptions are as follows:
_IO: The ioctl does not need data transfer
_IOW: The ioctl needs write parameters (copy_from_user or get_user)
_IOR: The ioctl needs read parameters (copy_to_user or put_user)
_IOWR: The ioctl needs both write and read parameters
What their parameters mean (in the order they are passed) is described here:
1. A number coded on 8 bits (0 to 255), called a magic number
2. A sequence number or command ID, also on 8 bits
3. A data type, if any, that will inform the kernel about the size to be copied
They are well documented in Documentation/ioctl/ioctl-decoding.txt in the kernel source, and existing ioctl are listed in Documentation/ioctl/ioctlnumber.txt, a good place to start when you need to create an ioctl command.
## Generating ioctl numbers (command)
You should generate their own ioctl number in a dedicated header file. It is not mandatory,
but it is recommended, since this header should be available in user space too. In other words, you should duplicate the ioctl header file so that there is one in the kernel and one in the user space, which you can include in user apps. Let's now generate ioctl numbers in a real example:
The eep_ioctl.h file is as follows:
#ifndef PACKT_IOCTL_H
```c
#define PACKT_IOCTL_H
```
/*
* We need to choose a magic number for our driver, and sequential numbers
* for each command:
*/
```c
#define EEP_MAGIC 'E'
#define ERASE_SEQ_NO 0x01
#define RENAME_SEQ_NO 0x02
#define ClEAR_BYTE_SEQ_NO 0x03
#define GET_SIZE 0x04
```
/*
* Partition name must be 32 byte max
*/
```c
#define MAX_PART_NAME 32
```
/*
* Now let's define our ioctl numbers:
*/
```c
#define EEP_ERASE _IO(EEP_MAGIC, ERASE_SEQ_NO)
#define EEP_RENAME_PART _IOW(EEP_MAGIC, RENAME_SEQ_NO, unsigned long)
#define EEP_GET_SIZE _IOR(EEP_MAGIC, GET_SIZE, int *)
```
#endif
## Steps for ioctl
First, let's have a look at its prototype. It looks as follows:
```c
long ioctl(struct file *f, unsigned int cmd, unsigned long arg);
```
There is only one step: use a switch ... case statement and return an -ENOTTY error when an undefined ioctl command is called. You can find more information at http://man7.org/linux/man-pages/man2/ioctl.2.html:
/*
* User space code also need to include the header file in which ioctls
* defined are defined. This is eep_ioctl.h in our case.
*/
```c
#include "eep_ioctl.h"
static long eep_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
int part;
```
char *buf = NULL;
```c
int size = 1300;
switch(cmd){
case EEP_ERASE:
erase_eeprom();
```
break;
```c
case EEP_RENAME_PART:
```
buf = kmalloc(MAX_PART_NAME, GFP_KERNEL);
```c
copy_from_user(buf, (char *)arg, MAX_PART_NAME);
rename_part(buf);
```
break;
```c
case EEP_GET_SIZE:
copy_to_user((int*)arg, &size, sizeof(int));
```
break;
default:
```c
return -ENOTTY;
}
return 0;
}
```
If you think your ioctl command will need more than one argument,
you should gather those arguments in a structure and just pass a pointer from the structure to ioctl.
Now, from the user space, you must use the same ioctl header as in the driver's code:
my_main.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "eep_ioctl.h" /* our ioctl header file */
int main()
{
int size = 0;
int fd;
```
char *new_name = "lorem_ipsum"; /* must not be longer than
MAX_PART_NAME */
fd = open("/dev/eep-mem1", O_RDWR);
```c
if (fd == -1){
printf("Error while opening the eeprom\n");
return -1;
}
ioctl(fd, EEP_ERASE); /* ioctl call to erase partition */
ioctl(fd, EEP_GET_SIZE, &size); /* ioctl call to get partition size */
ioctl(fd, EEP_RENAME_PART, new_name); /* ioctl call to rename partition */
close(fd);
return 0;
}
```
## Filling the file_operations structure
When writing kernel modules, it is better to use designated initializers when it comes to statically initialize structures with their parameters. This consists of naming the member you need to assign a value to. The form is .member-name to designate which member should be initialized. This allows, among other things, initializing the members in an undefined order, or leaving the fields that we do not want to modify unchanged.
Once we have defined our functions, we just have to fill the structure as follows:
```c
static const struct file_operations eep_fops = {
```
.owner = THIS_MODULE,
.read = eep_read,
.write = eep_write,
.open = eep_open,
.release = eep_release,
.llseek = eep_llseek,
.poll = eep_poll,
.unlocked_ioctl = eep_ioctl,
```c
};
```
Let's remember, the structure is given as a parameter to cdev_init in the init method.
## Summary
In this chapter, we have demystified character devices and we have seen how to let users interact with our driver through device files. We learned how to expose file operations to the user space and control their behavior from within the kernel. We went so far that you are even able to implement multi-device support. The next chapter is a bit hardwareoriented since it deals with platform drivers, which expose hardware device capabilities to the user space. The power of character drivers combined with platform drivers is just amazing. See you in the next chapter