# Professional Linux Kernel Architecture : Introduction and Overview

## 1.1 Tasks of the Kernel
介绍Kernel 的三种角色

## 1.2 Implementation Strategies
Microkernels
Monolithic Kernels

## 1.3 Elements of the Kernel

#### 1.3.2 UNIX Processes
Linux provides the `clone` method to generate threads.


This fine-grained distribution of resources extends the classical thread
concept and allows for a more or less continuous transition between thread and processes.
> 其实没有绝对的区分。

#### 1.3.3 Address Spaces and Privilege Levels
Every user process in the system has its own virtual address range that extends from 0 to TASK_SIZE.
The area above (from TASK_SIZE to 2^32 or 2^64) is reserved exclusively for the kernel — and may not be
accessed by user processes.
> 什么代码保证的这样的地址空间 ? 是不是内核只能使用 TASK_SIZE to 2^32 or 2^64 的物理地址 ? 不是吧!
> 什么user processes 可以访问内核空间?

The key difference between the two(kernel mode and user mode) is that access to the memory area above
TASK_SIZE — that is, kernel space — is forbidden in user mode.

The switch from user to kernel mode is made by means of special transitions known as `system calls`

The main difference to running in process
context is that the userspace portion of the virtual address space must not be accessed

When operating in interrupt context, the kernel must be more
cautious than normal; for instance, it must not go to sleep. 
> 如果睡眠了，会如何?

Physical pages are often called `page frames`. In contrast, the term `page` is reserved for pages in virtual
address space

#### 1.3.4 Page Tables
As most areas of virtual address spaces are not used and are therefore not associated with page frames, a
far less memory-intensive model that fulfills the same purpose can be used: multilevel paging

The architecture-dependent code of the kernel for two- and three-level CPUs must therefore emulate the
missing levels by dummy page tables. Consequently, the remaining memory management code can be
implemented independently of the CPU used.
> 证据　in code ？

#### 1.3.5 Allocation of Physical Memory
The kernel can allocate only whole page frames. Dividing memory into smaller
portions is delegated to the standard library in userspace. This library splits the page frames received
from the kernel into smaller areas and allocates memory to the processes.
> 分配page frmae : buddy system 和 slab(用户层自行决定, 看glibc)

#### 1.3.6 Timing
A global variable named `jiffies_64` and its
32-bit counterpart `jiffies` are incremented periodically at constant time intervals

 The various timer
mechanisms of the underlying architectures are used to perform these updates — each computer architecture provides some means of executing periodic actions, usually in the form of **timer interrupts**.

Depending on architecture, jiffies is incremented with a frequency determined by the central constant
HZ of the kernel. This is usually on the range between 1,000 and 100; in other words, the value of jiffies
is incremented between 1,000 and 100 times per second

It is possible to make the periodic tick dynamic.



#### 1.3.8 Device Drivers, Block and Character Devices
#### 1.3.9 Networks
#### 1.3.10 Filesystems

#### 1.3.11 Modules and Hotplugging

#### 1.3.12 Caching
 Because the kernel
implements access to block devices by means of page memory mappings, caches are also organized into
pages, that is, whole pages are cached, thus giving rise to the name page cache.
> 此cache非that cache

#### 1.3.13 List Handling

#### 1.3.14 Object Management and Reference Counting
The generic kernel object mechanism can be used to perform the following operations on objects:
1. Reference counting
1. Management of lists (sets) of objects
1. Locking of sets
1. Exporting object properties into userspace (via the sysfs filesystem)

```
struct kobject {
	const char		*name;
	struct list_head	entry;
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct kernfs_node	*sd; /* sysfs directory entry */
	struct kref		kref;
#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
	struct delayed_work	release;
#endif
	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;
};
```

The meanings of the individual elements of struct kobject are as follows:
1. `k_name` is a text name exported to userspace using sysfs. Sysfs is a virtual filesystem that allows for exporting various properties of the system into userspace. Likewise sd supports this connection, and I will come back to this in Chapter 10.
1. `kref` holds the general type struct kref designed to simplify reference management. I discuss this below.
1. `entry` is a standard list element used to group several kobjects in a list (known as a set in this case).
1. `kset` is required when an object is grouped with other objects in a set.
1. `parent` is a pointer to the parent element and enables a hierarchical structure to be established between kobjects.
1. `ktype` provides more detailed information on the data structure in which a kobject is embedded. Of greatest importance is the destructor function that returns the resources of the embedding data structure.

| Function                 | Meaning                                                                                                                                                                        |
|--------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| kobject_get, kobject_put | Increments or decrements the reference counter of a kobject                                                                                                                    |
| `kobject_(un)register`   | Registers or removes obj from a hierarchy (the object is added to the  existing set (if any) of the parent element; a corresponding entry is created in the sysfs filesystem). |
| `kobject_init`           | Initializes a kobject; that is, it sets the reference counter to its initial value and initializes the list elements of the object.                                            |
| kobect_add               | Initializes a kernel object and makes it visible in sysfs                                                                                                                      |
| kobject_cleanup          | Releases the allocated resources when a kobject (and therefore the embedding object) is no longer needed                                                                       |


```

/**
 * struct kset - a set of kobjects of a specific type, belonging to a specific subsystem.
 *
 * A kset defines a group of kobjects.  They can be individually
 * different "types" but overall these kobjects all want to be grouped
 * together and operated on in the same manner.  ksets are used to
 * define the attribute callbacks and other common events that happen to
 * a kobject.
 *
 * @list: the list of all kobjects for this kset
 * @list_lock: a lock for iterating over the kobjects
 * @kobj: the embedded kobject for this kset (recursion, isn't it fun...)
 * @uevent_ops: the set of uevent operations for this kset.  These are
 * called whenever a kobject has something happen to it so that the kset
 * can add new environment variables, or filter out the uevents if so
 * desired.
 */
struct kset {
	struct list_head list;
	spinlock_t list_lock;
	struct kobject kobj;
	const struct kset_uevent_ops *uevent_ops;
};
```
`uevent_ops` provides several function pointers to methods that relay information about the state
of the set to userland. This mechanism is used by the core of the driver model, for instance, to
format messages that inform about the addition of new devices.
> `uevent_ops` 的含义让人一头污水

#### 1.3.15 Data Types
A particularity that does not occur in normal userspace programming is per-CPU variables. They are
declared with DEFINE_PER_CPU(name, type), where name is the variable name and type is the data type

On SMP systems with several CPUs, an instance of the variable is created for each CPU. The
instance for a particular CPU is selected with `get_cpu(name, cpu)`, where `smp_processor_id()`, which
returns the identifier of the active processor, is usually used as the argument for cpu

This(`__user` labeled pointer) is because memory is mapped via page tables into the
userspace portion of the virtual address space and not directly mapped by physical memory. Therefore
the kernel needs to ensure that the page frame in RAM that backs the destination is actually present
