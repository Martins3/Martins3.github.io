# Linux Device Driver : The Linux Device Model
The demands of newer
systems,with their more complicated topologies and need to support features such
as power management,made it clear,however,that a general abstraction describing
the structure of the system was needed
> however

1. Power management and system shutdown
2. Communications with user space
3. Hotpluggable devices
4. Object lifecycles
5. Device classes
## 14.1 Kobjects, Ksets, and Subsystems
The tasks handled by `struct kobject` and its supporting code now include:
1. Reference counting of objects
2. Sysfs representation
3. Data structure glue
4. Hotplug event handling
> Wooooc, 看内核的时候怎么不知道有这么多功能啊


#### 14.1.1 Kobject Basics

**Embedding kobjects**

**Kobject initialization**


**Reference count manipulation**

**Release functions and kobject types**

#### 14.1.2 Kobject Hierarchies, Ksets, and Subsystems
**Ksets**

**Operations on ksets**

**Subsystems**

## 14.2 Low-Level Sysfs Operations
Kobjects are the mechanism behind the sysfs virtual filesystem. For every directory
found in sysfs,there is a kobject lurking somewhere within the kernel. Every kobject
of interest also exports one or more attributes,which appear in that kobject’s sysfs
directory as files containing kernel-generated information.
> Wooooqu, kobject and sysfs !

Getting a kobject to show up in sysfs is simply a matter of calling `kobject_add`

There are a couple of things worth knowing about how
the sysfs entry is created:
• Sysfs entries for kobjects are always directories,so a call to kobject_add results in
the creation of a directory in sysfs. Usually that directory contains one or more
attributes; we see how attributes are specified shortly.
• The name assigned to the kobject (with kobject_set_name) is the name used for
the sysfs directory. Thus,kobjects that appear in the same part of the sysfs hierarchy must have unique names. Names assigned to kobjects should also be reasonable file names: they cannot contain the slash character,and the use of white
space is strongly discouraged.
• The sysfs entry is located in the directory corresponding to the kobject’s parent
pointer. If parent is NULL when kobject_add is called,it is set to the kobject
embedded in the new kobject’s kset; thus,the sysfs hierarchy usually matches
the internal hierarchy created with ksets. If both parent and kset are NULL,the
sysfs directory is created at the top level,which is almost certainly not what you
want


```c
struct kobj_type {
	void (*release)(struct kobject *kobj);
	const struct sysfs_ops *sysfs_ops;
	struct attribute **default_attrs;
	const struct kobj_ns_type_operations *(*child_ns_type)(struct kobject *kobj);
	const void *(*namespace)(struct kobject *kobj);
};

struct attribute {
	const char		*name;
	umode_t			mode;
};


struct sysfs_ops {
	ssize_t	(*show)(struct kobject *, struct attribute *, char *);
	ssize_t	(*store)(struct kobject *, struct attribute *, const char *, size_t);
};
```
The `default_attrs` array says what the attributes are.
The `sysfs_ops` tell sysfs how to actually implement those attributes.
> 本节后面的没有看

#### 14.2.1 Nondefault Attributes
#### 14.2.2 Binary Attributes
#### 14.2.3 Symbolic Links

## 14.3 Hotplug Event Generation
## 14.4 Buses, Devices, and Drivers
## 14.5 Classes
## 14.6 Putting It All Together
## 14.7 Hotplug
## 14.8 Dealing with Firmware
