# Part 1. Data Structures & Synchronization

This part covers essential kernel data structures and synchronization mechanisms that form the foundation of driver development.

---

## 3.1 Container and Data Structures

### Introduction

The Linux kernel is a standalone software that **does NOT use any C library**. It implements its own mechanisms for:

- Data structures (lists, trees, hash tables)
- String functions
- Memory management
- Compression algorithms
- And much more...

**Why kernel implements its own facilities:**

```
User Space (with libc)        Kernel Space (standalone)
├── Uses glibc/musl           ├── No external libraries
├── malloc/free               ├── kmalloc/kfree
├── printf                    ├── printk
├── Standard data structures  ├── Custom optimized structures
└── Threading (pthread)       └── Kernel threads & workqueues
```

---

## 3.1.1 Understanding container_of Macro

### The Problem

**Scenario:** You have a structure with multiple fields. You receive a pointer to ONE field, and you need to get the pointer to the ENTIRE structure.

```c
/* Example structure */
struct person {
    int age;
    int salary;
    char *name;
};

struct person somebody;
int *age_ptr = &somebody.age;  /* Pointer to age field */

/* QUESTION: How to get pointer to 'somebody' from 'age_ptr'? */
```

### The Solution: container_of

**Definition:**

```c
/* Defined in include/linux/kernel.h */
#define container_of(ptr, type, member) ({                \
    const typeof(((type *)0)->member) * __mptr = (ptr);   \
    (type *)((char *)__mptr - offsetof(type, member));    \
})
```

**Syntax:**

```c
container_of(pointer, container_type, container_field);
```

**Parameters:**

- `pointer`: Pointer to the field inside the structure
- `container_type`: Type of the structure that contains the field
- `container_field`: Name of the field that `pointer` points to

### How It Works Internally

**Memory layout of struct person:**

```
Memory Address          Field        Offset
┌────────────────────────────────────────────┐
│ 0x1000                age           0      │
├────────────────────────────────────────────┤
│ 0x1004                salary        4      │
├────────────────────────────────────────────┤
│ 0x1008                name          8      │
└────────────────────────────────────────────┘
        ↑
    &somebody (container address)
```

**The magic formula:**

```
container_address = field_pointer - field_offset

If age_ptr = 0x1004 (pointing to age)
And offset of age = 4 bytes from start
Then: container_address = 0x1004 - 4 = 0x1000
```

**The `offsetof` macro:**

```c
/* Defined in include/linux/stddef.h */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
```

**How offsetof works:**

1. Cast 0 (NULL) to pointer of TYPE → `(TYPE *)0`
2. Access MEMBER from this null pointer → `((TYPE *)0)->MEMBER`
3. Get address of MEMBER → `&((TYPE *)0)->MEMBER`
4. Result is offset from base (0) to MEMBER

### Basic Example

```c
#include <linux/kernel.h>
#include <linux/module.h>

struct person {
    int age;
    int salary;
    char *name;
};

static int __init test_container_of(void)
{
    struct person somebody;
    struct person *person_ptr;
    int *age_ptr;

    /* Initialize */
    somebody.age = 30;
    somebody.salary = 50000;
    somebody.name = "John Doe";

    /* Get pointer to age field */
    age_ptr = &somebody.age;

    pr_info("Original structure address: %p\n", &somebody);
    pr_info("Age field address: %p\n", age_ptr);

    /* Use container_of to get back structure pointer */
    person_ptr = container_of(age_ptr, struct person, age);

    pr_info("Retrieved structure address: %p\n", person_ptr);
    pr_info("Age: %d\n", person_ptr->age);
    pr_info("Salary: %d\n", person_ptr->salary);
    pr_info("Name: %s\n", person_ptr->name);

    return 0;
}

/* Output:
 * Original structure address: 0xffff8880xxxxx
 * Age field address: 0xffff8880xxxxx  (same as above)
 * Retrieved structure address: 0xffff8880xxxxx (matches!)
 * Age: 30
 * Salary: 50000
 * Name: John Doe
 */
```

### Real-World Driver Example

**Common pattern in Linux drivers:**

```c
/* Device-specific structure */
struct mcp23016 {
    struct i2c_client *client;   /* I2C client */
    struct gpio_chip chip;       /* GPIO chip (substructure) */
    struct mutex lock;           /* Mutex for protection */
    u16 reg_output;              /* Register cache */
    u16 reg_direction;
};

/* Helper function using container_of */
static inline struct mcp23016 *to_mcp23016(struct gpio_chip *gc)
{
    /* Get mcp23016 structure from embedded gpio_chip */
    return container_of(gc, struct mcp23016, chip);
}

/* GPIO chip callback - receives gpio_chip pointer */
static int mcp23016_get_value(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp;
    u16 value;

    /* Convert gpio_chip pointer to mcp23016 pointer */
    mcp = to_mcp23016(gc);

    /* Now we can access device-specific data */
    mutex_lock(&mcp->lock);
    value = (mcp->reg_output >> offset) & 0x1;
    mutex_unlock(&mcp->lock);

    return value;
}

/* Probe function - initializes everything */
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret;

    /* Allocate device structure */
    mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
    if (!mcp)
        return -ENOMEM;

    /* Initialize fields */
    mcp->client = client;
    mutex_init(&mcp->lock);

    /* Setup GPIO chip */
    mcp->chip.label = "mcp23016";
    mcp->chip.get = mcp23016_get_value;  /* Our callback */
    mcp->chip.ngpio = 16;

    /* Register GPIO chip */
    ret = gpiochip_add(&mcp->chip);
    if (ret) {
        dev_err(&client->dev, "Failed to register GPIO chip\n");
        return ret;
    }

    /* When kernel calls mcp23016_get_value():
     * - Kernel passes gpio_chip pointer (mcp->chip)
     * - to_mcp23016() uses container_of to get mcp pointer
     * - Now we can access mcp->lock, mcp->client, etc.
     */

    return 0;
}
```

**Visual representation:**

```
Kernel GPIO subsystem
    ↓
    Calls: mcp23016_get_value(&mcp->chip)
    ↓
    Function receives: struct gpio_chip *gc
    ↓
    Uses: mcp = container_of(gc, struct mcp23016, chip)
    ↓
    Now has access to full structure:

    ┌──────────────────────────────┐
    │ struct mcp23016              │
    ├──────────────────────────────┤
    │ i2c_client *client           │← Can access this
    │ gpio_chip chip        ←(gc)  │← Received this
    │ mutex lock                   │← Can access this
    │ u16 reg_output               │← Can access this
    └──────────────────────────────┘
```

### Important Restrictions

**❌ container_of DOES NOT WORK with:**

```c
struct bad_example {
    char *name;        /* Pointer to char */
    char buffer[100];  /* Array */
    int age;
};

/* WRONG! Cannot use pointer field */
char *name_ptr = bad_ex.name;
struct bad_example *ptr = container_of(name_ptr,
                                       struct bad_example,
                                       name);  /* FAILS! */

/* WRONG! Cannot use array field */
char *buf_ptr = bad_ex.buffer;
struct bad_example *ptr = container_of(buf_ptr,
                                       struct bad_example,
                                       buffer);  /* FAILS! */

/* CORRECT! Use non-pointer, non-array field */
int *age_ptr = &bad_ex.age;
struct bad_example *ptr = container_of(age_ptr,
                                       struct bad_example,
                                       age);  /* WORKS! */
```

**✅ container_of WORKS with:**

- Non-pointer fields (int, struct, etc.)
- Embedded structures (struct inside struct)
- Non-array fields

### Pointer to Pointer Case

**Special case: Retrieving container when you have pointer to pointer:**

```c
struct family {
    struct person *father;  /* Pointer field */
    struct person *mother;  /* Pointer field */
    int number_of_sons;
    int family_id;
};

struct family f;

/* If you have pointer to father (struct person *dad) */
/* container_of WILL NOT WORK directly */

/* But if you have POINTER TO POINTER (struct person **dad_ptr) */
struct person **dad_ptr = &f.father;

/* NOW container_of works! */
struct family *fam_ptr = container_of(dad_ptr,
                                      struct family,
                                      father);
```

**Why?** Because `dad_ptr` points to the actual `father` field inside the structure, not to the person structure that `father` points to.

---

## 3.1.2 Linked Lists

### Why Kernel Implements Its Own Lists

**Problem with standard approaches:**

- Array: Fixed size, inefficient insertion/deletion
- User-space libraries: Not available in kernel
- Need efficient, dynamic data structure

**Solution: Circular doubly linked list**

**Why doubly + circular:**

- ✅ Insert at head or tail in O(1)
- ✅ Delete from anywhere in O(1)
- ✅ Traverse forward or backward
- ✅ Implement both FIFO and LIFO
- ✅ No NULL pointer checks needed (always points to valid node)

### The Core Structure: list_head

**Defined in `<linux/list.h>`:**

```c
struct list_head {
    struct list_head *next;  /* Points to next node */
    struct list_head *prev;  /* Points to previous node */
};
```

**Key insight:** `list_head` is embedded in YOUR data structure!

```
Traditional approach:        Kernel approach:
struct node {                struct car {
    void *data;                  int doors;
    struct node *next;           char *color;
    struct node *prev;           struct list_head list;  ← Embedded!
};                           };
```

### Creating Your First Linked List

**Step 1: Define your data structure**

```c
struct car {
    int door_number;
    char *color;
    char *model;
    struct list_head list;  /* Kernel's list structure */
};
```

**Step 2: Create list head**

```c
/* Method 1: Static initialization */
static LIST_HEAD(carlist);

/* Expands to: */
struct list_head carlist = { &carlist, &carlist };

/* Method 2: Dynamic initialization */
struct list_head carlist;
INIT_LIST_HEAD(&carlist);
```

**Step 3: Create and add nodes**

```c
#include <linux/list.h>
#include <linux/slab.h>

/* Allocate nodes */
struct car *redcar = kmalloc(sizeof(*redcar), GFP_KERNEL);
struct car *bluecar = kmalloc(sizeof(*bluecar), GFP_KERNEL);

/* Initialize embedded list_head */
INIT_LIST_HEAD(&redcar->list);
INIT_LIST_HEAD(&bluecar->list);

/* Fill data */
redcar->door_number = 4;
redcar->color = "red";
redcar->model = "Toyota";

bluecar->door_number = 2;
bluecar->color = "blue";
bluecar->model = "Honda";

/* Add to list */
list_add(&redcar->list, &carlist);   /* Add at head */
list_add(&bluecar->list, &carlist);  /* Add at head */
```

**Visual representation after insertions:**

```
After list_add(&redcar->list, &carlist):
carlist → redcar → carlist (circular)

After list_add(&bluecar->list, &carlist):
carlist → bluecar → redcar → carlist (circular)
         (head)              (tail)
```

### List API Functions

**Initialization:**

```c
/* Static - declare and initialize */
LIST_HEAD(name);

/* Dynamic - initialize existing list_head */
INIT_LIST_HEAD(struct list_head *list);
```

**Adding nodes:**

```c
/* Add at head (LIFO - stack behavior) */
void list_add(struct list_head *new, struct list_head *head);

/* Add at tail (FIFO - queue behavior) */
void list_add_tail(struct list_head *new, struct list_head *head);
```

**Example:**

```c
LIST_HEAD(my_list);

struct car *car1 = kmalloc(...);
struct car *car2 = kmalloc(...);
struct car *car3 = kmalloc(...);

INIT_LIST_HEAD(&car1->list);
INIT_LIST_HEAD(&car2->list);
INIT_LIST_HEAD(&car3->list);

/* Stack behavior (LIFO) */
list_add(&car1->list, &my_list);  /* my_list→car1 */
list_add(&car2->list, &my_list);  /* my_list→car2→car1 */
list_add(&car3->list, &my_list);  /* my_list→car3→car2→car1 */

/* Queue behavior (FIFO) */
LIST_HEAD(queue);
list_add_tail(&car1->list, &queue);  /* queue→car1 */
list_add_tail(&car2->list, &queue);  /* queue→car1→car2 */
list_add_tail(&car3->list, &queue);  /* queue→car1→car2→car3 */
```

**Deleting nodes:**

```c
/* Remove node from list */
void list_del(struct list_head *entry);

/* Remove and reinitialize node */
void list_del_init(struct list_head *entry);
```

**Important:** `list_del()` does NOT free memory!

```c
struct car *to_delete = ...;

/* Step 1: Remove from list */
list_del(&to_delete->list);

/* Step 2: Free memory manually */
kfree(to_delete->color);   /* Free allocated strings */
kfree(to_delete->model);
kfree(to_delete);           /* Free structure itself */
```

**Checking list status:**

```c
/* Check if list is empty */
int list_empty(const struct list_head *head);

/* Check if node is last in list */
int list_is_last(const struct list_head *list,
                 const struct list_head *head);

/* Check if node is head of list */
int list_is_head(struct list_head *list,
                 const struct list_head *head);
```

**Counting nodes:**

```c
/* Get number of nodes in list */
int list_count_nodes(struct list_head *head);
```

### List Traversal

**The power of list_for_each_entry:**

```c
list_for_each_entry(pos, head, member)
```

**Parameters:**

- `pos`: Loop cursor (your structure type)
- `head`: List head
- `member`: Name of list_head field in your structure

**Example:**

```c
struct car *acar;  /* Loop cursor */
int blue_car_count = 0;

/* Traverse all cars */
list_for_each_entry(acar, &carlist, list) {
    pr_info("Car: %s %s, Doors: %d\n",
            acar->color, acar->model, acar->door_number);

    if (strcmp(acar->color, "blue") == 0)
        blue_car_count++;
}

pr_info("Found %d blue cars\n", blue_car_count);
```

**Safe traversal (when deleting nodes):**

```c
list_for_each_entry_safe(pos, n, head, member)
```

**Why "safe"?** Allows deleting current node during iteration.

```c
struct car *acar, *tmp;

/* Delete all red cars */
list_for_each_entry_safe(acar, tmp, &carlist, list) {
    if (strcmp(acar->color, "red") == 0) {
        pr_info("Deleting red car: %s\n", acar->model);
        list_del(&acar->list);  /* Safe to delete here! */
        kfree(acar);
    }
}
```

**Reverse traversal:**

```c
list_for_each_entry_reverse(pos, head, member)
```

### How list_for_each_entry Works

**Macro expansion:**

```c
#define list_for_each_entry(pos, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                            \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
```

**It uses container_of internally!**

```
Step 1: Start with head->next (first real node)
Step 2: Use container_of to get struct car* from list_head*
Step 3: Check if we're back at head (end of loop)
Step 4: Move to next node using pos->member.next
Step 5: Repeat from step 2
```

### Complete Working Example

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>

/* Car structure with embedded list */
struct car {
    int door_number;
    char *color;
    char *model;
    struct list_head list;
};

/* List head */
static LIST_HEAD(carlist);

static int __init list_example_init(void)
{
    struct car *redcar, *bluecar, *greencar;
    struct car *iterator, *tmp;
    int count = 0;

    pr_info("=== Creating Cars ===\n");

    /* Create red car */
    redcar = kmalloc(sizeof(*redcar), GFP_KERNEL);
    redcar->door_number = 4;
    redcar->color = "red";
    redcar->model = "Toyota Camry";
    INIT_LIST_HEAD(&redcar->list);
    list_add(&redcar->list, &carlist);

    /* Create blue car */
    bluecar = kmalloc(sizeof(*bluecar), GFP_KERNEL);
    bluecar->door_number = 2;
    bluecar->color = "blue";
    bluecar->model = "Honda Civic";
    INIT_LIST_HEAD(&bluecar->list);
    list_add_tail(&bluecar->list, &carlist);

    /* Create green car */
    greencar = kmalloc(sizeof(*greencar), GFP_KERNEL);
    greencar->door_number = 4;
    greencar->color = "green";
    greencar->model = "Ford Focus";
    INIT_LIST_HEAD(&greencar->list);
    list_add_tail(&greencar->list, &carlist);

    pr_info("=== List Contents ===\n");

    /* Traverse and print */
    list_for_each_entry(iterator, &carlist, list) {
        pr_info("Car #%d: %s %s (%d doors)\n",
                ++count, iterator->color, iterator->model,
                iterator->door_number);
    }

    pr_info("Total cars: %d\n", count);

    /* Count blue cars */
    count = 0;
    list_for_each_entry(iterator, &carlist, list) {
        if (strcmp(iterator->color, "blue") == 0)
            count++;
    }
    pr_info("Blue cars: %d\n", count);

    /* Delete red car */
    pr_info("=== Deleting red cars ===\n");
    list_for_each_entry_safe(iterator, tmp, &carlist, list) {
        if (strcmp(iterator->color, "red") == 0) {
            pr_info("Removing: %s %s\n",
                    iterator->color, iterator->model);
            list_del(&iterator->list);
            kfree(iterator);
        }
    }

    /* Print remaining */
    pr_info("=== Remaining Cars ===\n");
    count = 0;
    list_for_each_entry(iterator, &carlist, list) {
        pr_info("Car #%d: %s %s\n",
                ++count, iterator->color, iterator->model);
    }

    return 0;
}

static void __exit list_example_exit(void)
{
    struct car *iterator, *tmp;

    pr_info("=== Cleanup: Removing all cars ===\n");

    /* Clean up all remaining nodes */
    list_for_each_entry_safe(iterator, tmp, &carlist, list) {
        pr_info("Freeing: %s %s\n",
                iterator->color, iterator->model);
        list_del(&iterator->list);
        kfree(iterator);
    }

    pr_info("Module unloaded\n");
}

module_init(list_example_init);
module_exit(list_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linked List Example");
```

**Output:**

```
=== Creating Cars ===
=== List Contents ===
Car #1: red Toyota Camry (4 doors)
Car #2: blue Honda Civic (2 doors)
Car #3: green Ford Focus (4 doors)
Total cars: 3
Blue cars: 1
=== Deleting red cars ===
Removing: red Toyota Camry
=== Remaining Cars ===
Car #1: blue Honda Civic
Car #2: green Ford Focus
=== Cleanup: Removing all cars ===
Freeing: blue Honda Civic
Freeing: green Ford Focus
Module unloaded
```

### Best Practices

**DO:**
✅ Always initialize list_head with INIT_LIST_HEAD() or LIST_HEAD()

✅ Use list_for_each_entry_safe() when deleting nodes

✅ Manually kfree() nodes after list_del()

✅ Check list_empty() before operations

✅ Use appropriate add function (list_add vs list_add_tail)

**DON'T:**
❌ Forget to initialize embedded list_head

❌ Use list_for_each_entry() when deleting nodes

❌ Assume list_del() frees memory

❌ Access freed nodes

❌ Modify list while iterating (unless using _safe variant)