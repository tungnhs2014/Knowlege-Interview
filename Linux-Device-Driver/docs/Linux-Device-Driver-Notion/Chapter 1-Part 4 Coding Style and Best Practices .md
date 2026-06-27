# Part 4. Coding Style and Best Practices

This section covers Linux kernel coding standards, style guidelines, memory allocation patterns, and object-oriented concepts used in kernel development.

---

## 1.18 Why Coding Style Matters

### The Importance of Consistency

The Linux kernel is one of the **largest collaborative software projects** in the world:

- **20,000+ files**, 30+ million lines of code
- **Thousands of contributors** from hundreds of companies
- Code must be **readable** and **maintainable** by anyone

**Benefits of consistent style:**

- **Faster code review** - Reviewers focus on logic, not formatting
- **Easier maintenance** - Consistent code is easier to understand
- **Prevents bugs** - Some style rules prevent common mistakes
- **Professional quality** - Shows attention to detail

**What happens without style compliance?**

- Patch rejected by maintainers
- Wasted review time
- Your code won't be merged into mainline

---

## 1.19 Essential Coding Style Rules

### Rule 1: Indentation (8-Character Tabs)

**Use TABS (not spaces), with 8-character width**

```c
/* CORRECT */
int my_function(void)
{
	if (condition) {
		do_something();
		do_another_thing();
	}
	return 0;
}

/* WRONG - Using spaces */
int my_function(void)
{
  if (condition) {    /* 2 spaces - WRONG! */
    do_something();
    do_another_thing();
  }
  return 0;
}

/* WRONG - 4 space tabs */
int my_function(void)
{
    if (condition) {  /* 4 spaces - WRONG! */
        do_something();
    }
    return 0;
}
```

**Why 8 characters?**

- **Visibility**: Makes deep nesting obvious (indicates bad design)
- **Readability**: Easy to see indentation levels on any screen
- **Tradition**: UNIX heritage since 1970s

**Configure your editor:**

```bash
# Vim
echo "set tabstop=8 shiftwidth=8 noexpandtab" >> ~/.vimrc

# Emacs
echo "(setq-default tab-width 8)" >> ~/.emacs
echo "(setq-default indent-tabs-mode t)" >> ~/.emacs
```

**Deep nesting example (BAD DESIGN):**

```c
/* If you need 3-4 levels of indentation, refactor! */
int bad_function(void)
{
	if (a) {
		if (b) {
			if (c) {
				if (d) {
					/* Too deep! */
					/* This is a sign to refactor */
				}
			}
		}
	}
}

/* Better: Extract functions */
int good_function(void)
{
	if (!a)
		return -EINVAL;

	return handle_case_a();
}

static int handle_case_a(void)
{
	if (!b)
		return -EINVAL;

	return handle_case_b();
}
```

### Rule 2: Line Length (80 Columns)

**Keep lines under 80 characters** (some flexibility to 100)

```c
/* GOOD - Fits in 80 columns */
int result = calculate_value(param1, param2, param3);

/* ACCEPTABLE - Long line broken properly */
int result = calculate_long_function_name_value(parameter1,
						parameter2,
						parameter3);

/* GOOD - Aligned continuation */
if (condition1 && condition2 &&
    condition3 && condition4) {
	do_work();
}

/* BAD - String broken (loses grep-ability) */
printk(KERN_ERR "This is a very long error message that "
       "continues here\n");  /* DON'T BREAK STRINGS! */

/* GOOD - Keep string intact even if > 80 chars */
printk(KERN_ERR "This is a very long error message that continues here\n");
```

**Why 80 columns?**

- **Terminal compatibility**: Standard terminal width
- **Side-by-side diffs**: See before/after in standard width
- **Multiple windows**: View multiple files simultaneously
- **Readability**: Forces concise, clear code

**Exception:** Strings should NOT be broken to preserve grep-ability

### Rule 3: Braces Placement

**Opening brace position depends on construct:**

**Functions: Opening brace on next line**

```c
/* CORRECT */
int my_function(int param)
{
	/* Function body */
	return 0;
}

/* WRONG */
int my_function(int param) {  /* Opening brace on same line - WRONG! */
	return 0;
}
```

**Structures: Opening brace on same line**

```c
/* CORRECT */
struct my_device {
	int id;
	void *data;
};

/* Also correct for inline initialization */
static struct file_operations fops = {
	.open = my_open,
	.release = my_release,
};
```

**Control statements: Opening brace on same line**

```c
/* CORRECT */
if (condition) {
	action();
} else {
	alternative();
}

while (count > 0) {
	process();
	count--;
}

/* Single statement: No braces needed */
if (error)
	return -EINVAL;

/* But braces for multi-line */
if (error) {
	cleanup_resources();
	return -EINVAL;
}
```

**Switch statements:**

```c
switch (value) {
case OPTION_A:
	handle_a();
	break;
case OPTION_B:
	handle_b();
	/* Fall through */
case OPTION_C:
	handle_c();
	break;
default:
	handle_default();
}
```

### Rule 4: Spaces and Parentheses

**Use spaces correctly around operators and keywords:**

```c
/* GOOD */
int result = a + b;
if (condition)
func(a, b, c);
ptr->member;
array[index];

/* BAD */
int result=a+b;           /* No spaces around = and + */
if(condition)             /* No space after if */
func (a,b,c);            /* Space before ( */
ptr -> member;           /* Spaces around -> */
array[ index ];          /* Spaces inside [] */
```

**Spaces after keywords (if, for, while, switch):**

```c
/* CORRECT */
if (x == y)
for (i = 0; i < n; i++)
while (condition)
switch (value)

/* WRONG */
if(x == y)         /* No space after if */
for(i = 0; i < n; i++)
```

**No spaces after function names:**

```c
/* CORRECT */
result = my_function(arg1, arg2);

/* WRONG */
result = my_function (arg1, arg2);  /* Space before ( */
```

**Spaces around most binary operators:**

```c
/* CORRECT */
y = x + 5;
mask = BIT(7) | BIT(8);
val = (a << 4) | b;

/* Exceptions: No space for * and & in pointers */
int *ptr;          /* NOT: int * ptr */
func(&variable);   /* NOT: func(& variable) */
```

### Rule 5: Naming Conventions

**Use descriptive, lowercase names with underscores:**

```c
/* GOOD variable names */
int device_count;
struct platform_device *pdev;
unsigned long timeout_ms;

/* BAD variable names */
int DeviceCount;      /* CamelCase - avoid in kernel */
int dc;              /* Too short, unclear */
int x;               /* Meaningless (except in loops) */
```

**Function names:**

```c
/* GOOD - Clear, descriptive */
static int gpio_chip_request(struct gpio_chip *chip, unsigned offset);
static void cleanup_device_resources(struct device *dev);

/* BAD - Unclear, generic */
static int request(int x);
static void cleanup(void *data);
```

**Macro names: ALL UPPERCASE**

```c
/* CORRECT */
#define MAX_BUFFER_SIZE  1024
#define DRIVER_NAME      "my_driver"

/* Function-like macros: Also uppercase (but use inline functions when possible) */
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
```

**Constants:**

```c
/* CORRECT */
#define DEVICE_ID_MAX    256
const int default_timeout = 1000;

/* WRONG */
#define deviceIdMax 256   /* CamelCase */
```

### Rule 6: Function Design

**Keep functions short and focused:**

```c
/* GOOD - Single, clear purpose */
static int validate_device_id(int id)
{
	if (id < 0 || id >= MAX_DEVICES)
		return -EINVAL;
	return 0;
}

/* GOOD - One task per function */
static int init_hardware(struct device *dev)
{
	enable_clock(dev);
	reset_device(dev);
	configure_pins(dev);
	return 0;
}

/* BAD - Function doing too much (should be split) */
static int do_everything(struct device *dev, int mode, void *data)
{
	/* 200 lines of mixed initialization, configuration,
	 * data processing, error handling...
	 * This function should be split into multiple functions
	 */
}
```

**Function length guideline:**

- **Ideal**: 20-40 lines
- **Acceptable**: Up to ~100 lines
- **Too long**: > 150 lines (consider refactoring)

**Limit function parameters:**

```c
/* GOOD - 3 parameters */
int configure_device(struct device *dev, int mode, int flags);

/* ACCEPTABLE - 5 parameters */
int configure_advanced(struct device *dev, int mode, int flags,
		       int timeout, int retries);

/* BAD - Too many parameters (use struct instead) */
int configure_everything(struct device *dev, int mode, int flags,
			int timeout, int retries, int speed,
			int voltage, int current);  /* TOO MANY! */

/* BETTER - Group related params in struct */
struct device_config {
	int mode;
	int flags;
	int timeout;
	int retries;
	int speed;
	int voltage;
	int current;
};

int configure_device(struct device *dev, struct device_config *config);
```

---

## 1.20 Comment Style

### Multi-line Comments

**Use kernel style for multi-line comments:**

```c
/*
 * This is the preferred style for multi-line comments
 * in the Linux kernel source code. Notice the opening
 * and closing markers.
 *
 * Each line starts with ' * ' (space-asterisk-space)
 */
```

**DON'T use C++ style:**

```c
// This is NOT the kernel style
// Even though it's valid C99
// Don't use it in kernel code
```

### Single-line Comments

```c
/* This is a single line comment */

int count;  /* Inline comment explaining the variable */
```

### Function Documentation Comments

**Use kernel-doc format for functions:**

```c
/**
 * gpio_request - request a GPIO pin
 * @gpio: GPIO number to request
 * @label: label for the requested GPIO (for debugging)
 *
 * This function requests a GPIO pin for exclusive use. The GPIO
 * must be freed with gpio_free() when no longer needed.
 *
 * Return: 0 on success, negative error code on failure
 */
int gpio_request(unsigned gpio, const char *label)
{
	/* Implementation */
}
```

**kernel-doc format:**

- Start with `/**`
- Function name and brief description
- `@param`: Parameter descriptions
- Detailed description (optional)
- `Return:` Return value description

### When to Comment

**DO comment:**

```c
/* Explain WHY, not WHAT */

/* Hardware requires 10ms delay after reset */
msleep(10);

/*
 * Workaround for silicon errata #1234:
 * Register must be written twice
 */
writel(val, reg);
writel(val, reg);

/* Lock ordering: Always acquire device_lock before irq_lock */
mutex_lock(&dev->device_lock);
spin_lock_irq(&dev->irq_lock);
```

**DON'T comment obvious code:**

```c
/* BAD - States the obvious */
i++;  /* Increment i */

/* BAD - Code is self-explanatory */
if (count > 0) {
	/* Process items if count is greater than zero */
	process_items();
}

/* GOOD - No comment needed, code is clear */
if (count > 0)
	process_items();
```

---

## 1.21 Code Organization

### Header Files

**Include guards:**

```c
/* my_driver.h */
#ifndef _MY_DRIVER_H_
#define _MY_DRIVER_H_

/* Header content */

#endif /* _MY_DRIVER_H_ */
```

**Include order:**

```c
/* my_driver.c */

/* 1. Linux kernel headers */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

/* 2. Architecture-specific headers */
#include <asm/io.h>

/* 3. Subsystem headers */
#include <linux/gpio.h>
#include <linux/i2c.h>

/* 4. Local headers */
#include "my_driver.h"
```

### Static Functions

**Use static for internal functions:**

```c
/* GOOD - Internal helper function */
static int init_hardware(struct device *dev)
{
	/* Only used in this file */
}

/* GOOD - External API function */
int my_driver_register(struct device *dev)
{
	/* Available to other modules */
}
EXPORT_SYMBOL(my_driver_register);
```

**Why use static?**

- **Namespace pollution prevention**: Avoids symbol conflicts
- **Compiler optimization**: Can inline aggressively
- **Clear intent**: Shows function is internal
- **Security**: Reduces attack surface

---

## 1.22 Using checkpatch.pl

### What is checkpatch.pl?

**Official kernel style checking tool** located at `scripts/checkpatch.pl`

**Checks for:**

- Coding style violations
- Common mistakes
- Best practice violations
- Documentation issues

### Basic Usage

```bash
cd ~/embedded-workspace/kernel/linux

# Check a single file
./scripts/checkpatch.pl --file drivers/mydriver.c

# Check a patch
git format-patch -1  # Create patch from last commit
./scripts/checkpatch.pl 0001-my-patch.patch

# Check staged changes
git diff --cached | ./scripts/checkpatch.pl -
```

### Example Output

```bash
$ ./scripts/checkpatch.pl --file mydriver.c

WARNING: line over 80 characters
#45: FILE: mydriver.c:45:
+	printk(KERN_INFO "This is a very long message that exceeds the 80 character limit\n");

ERROR: space prohibited before that ',' (ctx:WxW)
#67: FILE: mydriver.c:67:
+	func(a , b);
	     ^

ERROR: trailing whitespace
#89: FILE: mydriver.c:89:
+	int count; $

WARNING: missing space after return type
#102: FILE: mydriver.c:102:
+static int*get_ptr(void)

total: 2 errors, 2 warnings, 150 lines checked
```

### Common checkpatch Warnings/Errors

**ERROR: trailing whitespace**

```c
/* BAD */
int count;    /* Spaces after semicolon */

/* GOOD */
int count;
```

**WARNING: line over 80 characters**

```c
/* BAD */
printk(KERN_ERR "This is a very long error message that exceeds 80 characters\n");

/* GOOD */
printk(KERN_ERR "Long error message here\n");
/* Or use pr_err */
pr_err("Long error message here\n");
```

**ERROR: spaces required around operators**

```c
/* BAD */
result=a+b;

/* GOOD */
result = a + b;
```

**WARNING: Missing blank line after declarations**

```c
/* BAD */
int count;
count = get_count();

/* GOOD */
int count;

count = get_count();
```

### Strict Mode

```bash
# More pedantic checking
./scripts/checkpatch.pl --strict --file mydriver.c

# Additional checks in strict mode:
# - Alignment issues
# - Unnecessary braces
# - Prefer specific functions (usleep_range vs msleep)
```

### Fixing Style Issues

**Automated formatting (use with caution):**

```bash
# Install indent tool
sudo apt-get install indent

# Use kernel's Lindent script
./scripts/Lindent mydriver.c

# WARNING: Always review changes!
# Lindent doesn't handle everything correctly
```

---

## 1.23 Memory Allocation in Kernel

### Static vs Dynamic Allocation

**Static allocation:**

```c
/* Global/static variables - allocated at compile time */
static int device_count = 0;
static struct my_device devices[MAX_DEVICES];

/* Pros: Fast, no allocation overhead */
/* Cons: Fixed size, wastes memory if unused */
```

**Dynamic allocation:**

```c
/* Allocated at runtime using kmalloc/kzalloc */
struct my_device *dev;

dev = kzalloc(sizeof(*dev), GFP_KERNEL);
if (!dev)
	return -ENOMEM;

/* Use the device */

kfree(dev);  /* Must free when done */

/* Pros: Flexible, allocates only what's needed */
/* Cons: Can fail, overhead of allocation */
```

### Common Kernel Structures

**Prefer dynamic allocation for device structures:**

```c
/* BAD - Static allocation */
static struct platform_device my_device;

/* GOOD - Dynamic allocation */
static int my_probe(struct platform_device *pdev)
{
	struct my_device *dev;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, dev);
	return 0;
}
```

**Use managed allocation (devm_*) when possible:**

```c
/* Traditional allocation - manual cleanup needed */
struct resource *res;
void __iomem *base;

res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
base = ioremap(res->start, resource_size(res));
/* ... */
iounmap(base);  /* Must remember to free */

/* Managed allocation - automatic cleanup */
base = devm_ioremap_resource(&pdev->dev, res);
/* Automatically freed when device is removed */
```

---

## 1.24 Object-Oriented Concepts in Kernel

### Why OOP in C?

The kernel uses **object-oriented patterns** without C++ language features:

- **Encapsulation**: Data + operations in structures
- **Inheritance**: Embedding structures
- **Polymorphism**: Function pointers (ops structures)

### Operations Structures

**Common pattern: Group related function pointers:**

```c
/* Define operations interface */
struct file_operations {
	int (*open)(struct inode *, struct file *);
	int (*release)(struct inode *, struct file *);
	ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
};

/* Implementation */
static int my_open(struct inode *inode, struct file *file)
{
	/* Implementation */
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	/* Implementation */
	return 0;
}

/* Register operations */
static const struct file_operations my_fops = {
	.owner   = THIS_MODULE,
	.open    = my_open,
	.release = my_release,
	.read    = my_read,
	.write   = my_write,
};
```

**Benefits:**

- **Standard interface**: All drivers implement same operations
- **Polymorphism**: Core code calls ops without knowing specific driver
- **Maintainability**: Easy to add new operations

### kobject and Reference Counting

**Every kernel object has a reference count:**

```c
/* kobject is embedded in many kernel structures */
struct device {
	struct kobject kobj;
	/* ... */
};

/* Reference counting functions */
void get_device(struct device *dev)
{
	kobject_get(&dev->kobj);  /* Increment refcount */
}

void put_device(struct device *dev)
{
	kobject_put(&dev->kobj);  /* Decrement refcount */
	/* Object freed when refcount reaches 0 */
}
```

**Why reference counting?**

- **Prevents premature freeing**: Object stays alive while used
- **Thread-safe**: Atomic reference count operations
- **Automatic cleanup**: Object freed when last reference released

---

## 1.25 Best Practices Summary

### Code Quality Checklist

✅ **Before submitting code:**

1. **Run checkpatch.pl**
    
    ```bash
    ./scripts/checkpatch.pl --strict --file mydriver.c
    ```
    
2. **Compile without warnings**
    
    ```bash
    make W=1 drivers/mydriver.o
    ```
    
3. **Test on real hardware**
    - Load/unload module
    - Test all code paths
    - Check for memory leaks
4. **Review Documentation/CodingStyle**
    
    ```bash
    less Documentation/process/coding-style.rst
    ```
    
5. **Add proper attribution**
    
    ```c
    MODULE_AUTHOR("Your Name <your.email@example.com>");
    MODULE_DESCRIPTION("Driver description");
    MODULE_LICENSE("GPL v2");
    ```
    

### Common Mistakes to Avoid

❌ **Don't:**

- Mix tabs and spaces
- Use C++ style comments (//)
- Ignore checkpatch warnings
- Use CamelCase names
- Write functions > 150 lines
- Forget to free allocated memory
- Break strings across lines
- Use global variables unnecessarily

✅ **Do:**

- Use 8-character tabs
- Follow brace placement rules
- Keep lines under 80 characters
- Write descriptive commit messages
- Test thoroughly before submitting
- Read code from similar drivers
- Ask for review on mailing lists

### Learning Resources

**Official Documentation:**

```bash
# In kernel source tree
Documentation/process/coding-style.rst
Documentation/process/submitting-patches.rst
Documentation/driver-api/
```

**Online Resources:**

- Linux Kernel Mailing List (LKML)
- kernelnewbies.org
- LWN.net (Linux Weekly News)
- elixir.bootlin.com (source browser)

### Final Tips

**Start small:**

- Begin with simple character driver
- Study existing drivers in your subsystem
- Submit small patches initially

**Be patient:**

- Code review takes time
- Be ready to revise based on feedback
- Learn from experienced developers

**Stay updated:**

- Kernel style evolves slowly
- Follow mailing list discussions
- Read commit messages in git log

---

## Chapter 1 Summary

**You've learned:**

✅ Setting up development environment

✅ Getting and organizing kernel source

✅ Understanding kernel source tree structure

✅ Kernel configuration system

✅ Building kernel (native and cross-compilation)

✅ Kernel coding style and standards

✅ Using checkpatch.pl for style checking

✅ Memory allocation patterns

✅ Object-oriented concepts in kernel

**You're now ready to:**

- Set up kernel development environment
- Download and build Linux kernel
- Configure kernel for embedded targets
- Write code following kernel standards
- Navigate kernel source tree effectively