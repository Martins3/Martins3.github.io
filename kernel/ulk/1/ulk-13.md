# Understand Linux Kernel : I/O Architecture and Device Drivers

## KeyNote
1. 当一个 device driver 被注册之后，bus_type::match 以及 device_driver::probe 用于将 hardware 管理起来
2. port , interface, device controller
3. While accessing I/O ports is simple, detecting which I/O ports have been assigned to I/O devices may not be easy
4. 利用 kobject kset 构建 device device_driver bus_type class 之间关系，表现在 sysfs 上
5. *This*(foo_read() foo_interrupt()) is a typical case in which it is preferable to implement the driver using the interrupt mode
6. 物理地址为什么会出现空洞 : 出现空洞的含义是什么 ?
    1. 这是寻址控制器(虽然不知道具体的名字，也许其实内存控制器的作用吧)的角度确定的 ?
    2. 内存控制器在硬件上就会确定特定的区间为 ISA 或者 CPI, 那些依赖于 PCI 总线维持生活的设备然后在 resources 上进一步分配吗 ?

7. In the following discussion, we assume that `PAGE_OFFSET` is equal to 0xc0000000—that is, that the kernel linear addresses are in the fourth gigabyte.

- In the section “I/O Architecture,” we give a brief survey of the 80x86 I/O architecture.
- In the section “The Device Driver Model,” we introduce the Linux device driver model.
- Next, in the section “Device Files,” we show how the VFS associates a special file called “device file” with each different hardware device, so that application
- programs can use all kinds of devices in the same way.
- We then introduce in the section “Device Drivers” some common characteristics of device drivers.
- Finally, in the section “Character Device Drivers,” we illustrate the overall organization of character device drivers in Linux. We’ll defer the discussion of block device drivers to the next chapters


## 1. I/O Architecture

Any computer has a system bus that connects most of the internal hardware devices.
A typical system bus is the **PCI** (Peripheral Component Interconnect) bus. Several
other types of buses, such as ISA, EISA, MCA, SCSI, and USB, are currently in use.
Typically, the same computer includes several buses of different types, linked
together by hardware devices called bridges. Two high-speed buses are dedicated to
the data transfers to and from the memory chips: the **frontside** bus connects the CPUs
to the RAM controller, while the **backside** bus connects the CPUs directly to the
external hardware cache. The **host bridge** links together the **system bus** and the **frontside bus**.
> @todo system bus, frontside, backside, host bridge 都是他自己给的定义吗 ?

**Any I/O device is hosted by one, and only one, bus.** The bus type affects the internal
design of the I/O device, as well as how the device has to be handled by the kernel.
In this section, we discuss the functional characteristics common to all PC architectures, without giving details about a specific bus type.

The data path that connects a CPU to an I/O device is generically called an I/O bus.
The 80 × 86 microprocessors use 16 of their address pins to address I/O devices and
8, 16, or 32 of their data pins to transfer data. The I/O bus, in turn, is connected to
each I/O device by means of a hierarchy of hardware components including up to
three elements: **I/O ports**, **interfaces**, and **device controllers.**


#### 1.1 I/O Ports

I/O ports may also be mapped into addresses of the physical address space. The processor is then able to communicate with an I/O device by issuing assembly language
instructions that operate directly on memory (for instance, mov, and, or, and so on).
Modern hardware devices are more suited to mapped I/O, because it is faster and
can be combined with DMA.
> 80x86 汇编课程的时候，都是使用 io 设备的端口都是固定的，此时将 io 端口映射 ?


* ***Accessing I/O ports***

io.h
```c
#define BUILDIO(bwl, bw, type)						\
static inline void out##bwl(unsigned type value, int port)		\
{									\
	asm volatile("out" #bwl " %" #bw "0, %w1"			\
		     : : "a"(value), "Nd"(port));			\
}									\
```

> 并不神奇，只是封装一下 in out 指令而已

**While accessing I/O ports is simple, detecting which I/O ports have been assigned to
I/O devices may not be easy**, in particular, for systems based on an ISA bus. Often a
device driver must blindly write into some I/O port to probe the hardware device; if,
however, this I/O port is already used by some other hardware device, a system crash
could occur. To prevent such situations, the kernel keeps track of I/O ports assigned
to each hardware device by means of “resources.”
> @todo 很好，端口的使用又变成动态分配了。端口动态分配，需要修改 device controller 之类的 ?


#### 1.2 I/O Interfaces

**An I/O interface is a hardware circuit inserted between a group of I/O ports and the
corresponding device controller. It acts as an interpreter that translates the values in
the I/O ports into commands and data for the device.** In the opposite direction, it
detects changes in the device state and correspondingly updates the I/O port that
plays the role of status register. This circuit can also be connected through an IRQ
line to a Programmable Interrupt Controller, so that it issues interrupt requests on
behalf of the device.

There are two types of interfaces:
1. Custom I/O interfaces
2. General-purpose I/O interfaces

* ***General-purpose I/O interfaces***

> 分别列举了各种 interface, 但是@todo 并不知道为什么有的需要被划分为 custom ，有的需要被划分为 general purpose 的

#### 1.3 Device Controllers
A complex device may require a device controller to drive it. Essentially, the controller plays two important roles:
- It interprets the high-level commands received from the I/O interface and forces
the device to execute specific actions by sending proper sequences of electrical
signals to it.
- It converts and properly interprets the electrical signals received from the device
and modifies (through the I/O interface) the value of the status register
> 这样，从 CPU 到 device 的路径就很清新了，cpu -> io port -> interface -> device controller -> device

A typical device controller is the disk controller, which receives high-level commands
such as a “write this block of data” from the microprocessor (through the I/O interface) and converts them into low-level disk operations such as “position the disk
head on the right track” and “write the data inside the track.”
**Modern disk controllers are very sophisticated, because they can keep the disk data in on-board fast disk
caches and can reorder the CPU high-level requests optimized for the actual disk geometry.**
> device controller 好神奇吗！

Simpler devices do not have a device controller; examples include the *Programmable
Interrupt Controller* (see the section “Interrupts and Exceptions” in Chapter 4) and
the *Programmable Interval Timer* (see the section “Programmable Interval Timer
(PIT)” in Chapter 6).

Several hardware devices include their own memory, which is often called **I/O shared
memory**. For instance, all recent graphic cards include tens of megabytes of RAM in
the frame buffer, which is used to store the screen image to be displayed on the
monitor. We will discuss I/O shared memory in the section “Accessing the I/O
Shared Memory” later in this chapter.

## 2 The Device Driver Model
Drivers for such devices should typically take care of:
- Power management (handling of different voltage levels on the device’s power line)
- Plug and play (transparent allocation of resources when configuring the device)
- Hot-plugging (support for insertion and removal of the device while the system is running)

#### 2.1 The sysfs Filesystem
A goal of the sysfs filesystem is to expose the **hierarchical relationships among the
components of the device driver model**. The related top-level directories of this filesystem are:
> device driver models 是存在 hierarchical relationship 的，how ? why ?

| dir      | desc                                                                                                                                                                                      |
|----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| block    | The block devices, independently from the bus to which they are connected.
| devices  | All hardware devices recognized by the kernel, organized according to the bus in which they are connected.
| bus      | The buses in the system, which host the devices.
| drivers  | The device drivers registered in the kernel.
| class    | The types of devices in the system (audio cards, network cards, graphics cards, and so on); the same class may include devices hosted by different buses and driven by different drivers.
| power    | Files to handle the power states of some hardware devices.
| firmware | Files to handle the firmware of some hardware devices.

Relationships between components of the device driver models are expressed in the
sysfs filesystem as symbolic links between directories and files.
For example, the `/sys/block/sda/device` file can be a symbolic link to a subdirectory nested in `/sys/devices/pci0000:00` representing the SCSI controller connected to the PCI bus.
Moreover, the `/sys/block/sda/device/block` file is a symbolic link to `/sys/block/sda`, stating that this
PCI device is the controller of the SCSI disk
> @todo 这个设计有点让人震惊 !

**The main role of regular files in the sysfs filesystem is to represent attributes of drivers and devices.** For instance, the dev file in the `/sys/block/hda` directory contains the
major and minor numbers of the master disk in the first IDE chain.

#### 2.2 Kobjects
**The core data structure of the device driver model is a generic data structure named
kobject, which is inherently tied to the sysfs filesystem: each kobject corresponds to a
directory in that filesystem.**
> 所以 kobject 是如何形成文件系统的? (显然是通过 sysfs 啊)
> 但是为什么 可以，以及为什么需要形成 hierarchical structure

Kobjects are embedded inside larger objects—the so-called “containers”—that
describe the components of the device driver model. The descriptors of buses,
devices, and drivers are typical examples of containers; for instance, the descriptor of
the first partition in the first IDE disk corresponds to the `/sys/block/hda/hda1` directory.
Embedding a kobject inside a container allows the kernel to:
- Keep a reference counter for the container.
- Maintain hierarchical lists or sets of containers (for instance, a sysfs directory
associated with a block device includes a different subdirectory for each disk
partition)
- Provide a User Mode view for the attributes of the container

* ***Kobjects, ksets, and subsystems***

The kobjects can be organized in a hierarchical tree by means of ksets.
A kset is a collection of kobjects of the same type—that is, included in the same type of container.

Collections of ksets called subsystems also exist. A subsystem may include ksets of
different types.


Even the subsystem data structure can be embedded in a larger “container” object;
the reference counter of the container is thus the reference counter of the embedded
subsystem—that is, the reference counter of the kobject embedded in the kset
embedded in the subsystem. The `subsys_get()` and `subsys_put()` functions respectively increase and decrease this reference counter.

> 配合表格，详细讲解了 kobjects ksets 和 subsystems 的成员，三个关系是逐个包含的。
> 其中重点说明了 ksets::kobj 成员的妙用

Figure 13-3 illustrates an example of the device driver model hierarchy. The bus subsystem includes a pci subsystem, which, in turn, includes a drivers kset. This kset
contains a serial kobject—corresponding to the device driver for the serial port—
having a single new-id attribute.

> @todo 实际上 susbsys 已经不见了。

* ***Registering kobjects, ksets, and subsystems***

The `kobject_register()` function initializes a kobject and adds the corresponding
directory to the sysfs filesystem. Before invoking it, the caller should set the `kset` field
in the kobject so that it points to the parent kset, if any. The `kobject_unregister()`
function removes a kobject’s directory from the sysfs filesystem. To make life easier
for kernel developers, Linux also offers the `kset_register()` and `kset_unregister()`
functions, and the `subsystem_register()` and `subsystem_unregister()` functions, but
they are essentially wrapper functions around `kobject_register()` and `kobject_unregister()`.
> @todo 只有 kset_register 现在还存在，其余对应功能实现不知道移动到哪里了 !

As stated before, many kobject directories include regular files called **attributes**. The
`sysfs_create_file()` function receives as its parameters the addresses of a kobject
and an attribute descriptor, and creates the special file in the proper directory. Other
relationships between the objects represented in the sysfs filesystem are established
by means of symbolic links: the `sysfs_create_link()` function creates a symbolic link
for a given kobject in a directory associated with another kobject.

```c
static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}

/**
 *	sysfs_create_link - create symlink between two objects.
 *	@kobj:	object whose directory we're creating the link in.
 *	@target:	object we're pointing to.
 *	@name:		name of the symlink.
 */
int sysfs_create_link(struct kobject *kobj, struct kobject *target,
		      const char *name)
{
	return sysfs_do_create_link(kobj, target, name, 1);
}
EXPORT_SYMBOL_GPL(sysfs_create_link);
```


#### 2.3 Components of the Device Driver Model

* ***Devices***

Each device in the device driver model is represented by a device object.

The devices are organized hierarchically: a device is the “parent” of some “children”
devices if the children devices cannot work properly without the parent device.

The `parent` field of the
device object points to the descriptor of the parent device, the `children` field is the
head of the list of children devices, and the `node` field stores the pointers to the adjacent elements in the children list. The parenthood relationships between the kobjects embedded in the device objects reflect also the device hierarchy; thus, the
structure of the directories below `/sys/devices` matches the physical organization of
the hardware devices.
```c
struct device {
	struct kobject kobj;
	struct device		*parent;

	struct device_private	*p; // 实际上描述 children 的方法，并且提供各种封装函数。
```

The `device_register()` function inserts a new device object in the device driver
model, and automatically creates a new directory for it under /sys/devices. Conversely,
the device_unregister() function removes a device from the device driver model.

Usually, the device object is statically embedded in a larger descriptor. For instance,
PCI devices are described by pci_dev data structures; the dev field of this structure is
a device object, while the other fields are specific to the PCI bus.
The `device_register()` and `device_unregister()` functions are executed when the device is being
registered or de-registered in the PCI kernel layer.

```c
/**
 * device_register - register a device with the system.
 * @dev: pointer to the device structure
 *
 * This happens in two clean steps - initialize the device
 * and add it to the system. The two steps can be called
 * separately, but this is the easiest and most common.
 * I.e. you should only call the two helpers separately if
 * have a clearly defined need to use and refcount the device
 * before it is added to the hierarchy.
 *
 * For more information, see the kerneldoc for device_initialize()
 * and device_add().
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up the
 * reference initialized in this function instead.
 */
int device_register(struct device *dev)
{
	device_initialize(dev);
	return device_add(dev);
}
EXPORT_SYMBOL_GPL(device_register);
```

* ***Drivers***

Each driver in the device driver model is described by a `device_driver` object.


The `device_driver` object includes four methods for handling hot-plugging, plug and play, and power management

The `probe` method is invoked whenever a bus device
driver discovers a device that could possibly be handled by the driver; the corresponding function should probe the hardware to perform further checks on the
device. The remove method is invoked on a hot-pluggable device whenever it is
removed; it is also invoked on every device handled by the driver when the driver
itself is unloaded. The shutdown, suspend, and resume methods are invoked on a
a device when the kernel must change its power state.

The `driver_register()` function inserts a new device_driver object in the device
driver model, and automatically creates a new directory for it in the sysfs filesystem.
Conversely, the `driver_unregister()` function removes a driver from the device
driver model.
Usually, the device_driver object is statically embedded in a larger descriptor. For
instance, PCI device drivers are described by `pci_driver` data structures; the driver
field of this structure is a `device_driver` object, while the other fields are specific to
the PCI bus.
```c
/**
 * driver_register - register driver with bus
 * @drv: driver to register
 *
 * We pass off most of the work to the bus_add_driver() call,
 * since most of the things we have to do deal with the bus
 * structures.
 */
int driver_register(struct device_driver *drv)
{
	int ret;
	struct device_driver *other;

	if (!drv->bus->p) {
		pr_err("Driver '%s' was unable to register with bus_type '%s' because the bus was not initialized.\n",
			   drv->name, drv->bus->name);
		return -EINVAL;
	}

	if ((drv->bus->probe && drv->probe) ||
	    (drv->bus->remove && drv->remove) ||
	    (drv->bus->shutdown && drv->shutdown))
		printk(KERN_WARNING "Driver '%s' needs updating - please use "
			"bus_type methods\n", drv->name);

	other = driver_find(drv->name, drv->bus);
	if (other) {
		printk(KERN_ERR "Error: Driver '%s' is already registered, "
			"aborting...\n", drv->name);
		return -EBUSY;
	}

	ret = bus_add_driver(drv);
	if (ret)
		return ret;
	ret = driver_add_groups(drv, drv->groups);
	if (ret) {
		bus_remove_driver(drv);
		return ret;
	}
	kobject_uevent(&drv->p->kobj, KOBJ_ADD);

	return ret;
}
```

* ***Buses***

Each bus type supported by the kernel is described by a `bus_type` object

The per-bus subsystem typically includes only two ksets named drivers and devices (corresponding to the drivers and devices fields of the bus_type object, respectively).
The drivers kset contains the device_driver descriptors of all device drivers pertaining to the bus type, while the devices kset contains the device descriptors of all
devices of the given bus type. Because the directories of the devices’ kobjects already
appear in the sysfs filesystem under `/sys/devices`, the devices directory of the per-bus
subsystem stores symbolic links pointing to directories under `/sys/devices`.
The `bus_for_each_drv()` and `bus_for_each_dev()` functions iterate over the elements of the lists of drivers and devices, respectively.

The `match` method is executed when the kernel must check whether a given device
can be handled by a given driver. Even if each device’s identifier has a format specific to the bus that hosts the device, the function that implements the method is
usually simple, because it searches the device’s identifier in the driver’s table of supported identifiers. The hotplug method is executed when a device is being registered
in the device driver model; the implementing function should add bus-specific information to be passed as environment variables to a User Mode program that is notified about the new available device (see the later section “Device Driver
Registration”). Finally, the `suspend` and `resume` methods are executed when a device
on a bus of the given type must change its power state.



* ***Classes***
```c
/**
 * struct class - device classes
 * @name:	Name of the class.
 * @owner:	The module owner.
 * @class_groups: Default attributes of this class.
 * @dev_groups:	Default attributes of the devices that belong to the class.
 * @dev_kobj:	The kobject that represents this class and links it into the hierarchy.
 * @dev_uevent:	Called when a device is added, removed from this class, or a
 *		few other things that generate uevents to add the environment
 *		variables.
 * @devnode:	Callback to provide the devtmpfs.
 * @class_release: Called to release this class.
 * @dev_release: Called to release the device.
 * @shutdown_pre: Called at shut-down time before driver shutdown.
 * @ns_type:	Callbacks so sysfs can detemine namespaces.
 * @namespace:	Namespace of the device belongs to this class.
 * @get_ownership: Allows class to specify uid/gid of the sysfs directories
 *		for the devices belonging to the class. Usually tied to
 *		device's namespace.
 * @pm:		The default device power management operations of this class.
 * @p:		The private data of the driver core, no one other than the
 *		driver core can touch this.
 *
 * A class is a higher-level view of a device that abstracts out low-level
 * implementation details. Drivers may see a SCSI disk or an ATA disk, but,
 * at the class level, they are all simply disks. Classes allow user space
 * to work with devices based on what they do, rather than how they are
 * connected or how they work.
 */
struct class {
	const char		*name;
	struct module		*owner;

	const struct attribute_group	**class_groups;
	const struct attribute_group	**dev_groups;
	struct kobject			*dev_kobj;

	int (*dev_uevent)(struct device *dev, struct kobj_uevent_env *env);
	char *(*devnode)(struct device *dev, umode_t *mode);

	void (*class_release)(struct class *class);
	void (*dev_release)(struct device *dev);

	int (*shutdown_pre)(struct device *dev);

	const struct kobj_ns_type_operations *ns_type;
	const void *(*namespace)(struct device *dev);

	void (*get_ownership)(struct device *dev, kuid_t *uid, kgid_t *gid);

	const struct dev_pm_ops *pm;

	struct subsys_private *p;
};
```

Each class is described by a class object. All class objects belong to the class_subsys
subsystem associated with the /sys/class directory. Each class object, moreover,
includes an embedded subsystem; thus, for example, there exists a `/sys/class/input` directory associated with the input class of the device driver model.

> @todo 后面还有三段描述，但是有点看不懂

> @todo 总结一下 : 上面快速的描述了 driver device bus class 这些结构体，但是它们的用途，关系是什么 ? 为什么需要形成这样的抽象 ?

## 3 Device Files

Network cards are a notable exception to this schema, because they are hardware devices that are not directly associated with device files.

Traditionally, this identifier consists of the type of device file (character or block) and
a pair of numbers. The first number, called the major number, identifies the device
type. Traditionally, **all device files that have the same major number and the same
type share the same set of file operations**, because they are handled by the same
device driver. The second number, called the minor number, identifies a specific
device among a group of devices that share the same major number. For instance, a
group of disks managed by the same disk controller have the same major number
and different minor numbers.

#### 3.1 User Mode Handling of Device Files
> 说明了一下 major minor `dev_t`

The additional available device numbers are not being statically allocated in the official registry, because they should be used only when dealing with unusual demands
for device numbers. Actually, today’s preferred way to deal with device files is highly
dynamic, both in the device number assignment and in the device file creation.
> @todo 所以，当时，为什么就是需要将 major 固定的 ?

* ***Dynamic device number assignment***

*In this case, however, the device file cannot be created once and forever;* **it must be
created right after the device driver initialization with the proper major and minor
numbers.** Thus, there must be a standard way to export the device numbers used by
each driver to the User Mode applications. As we have seen in the earlier section
“Components of the Device Driver Model,” **the device driver model provides an elegant solution: the major and minor numbers are stored in the dev attributes contained in the subdirectories of `/sys/class`**
> @todo 所以这种方法为什么就解决了问题

* ***Dynamic device file creation***

A set of User Mode programs, collectively known as the `udev` toolset, must be installed in the system.
> 分析，在 /dev 下动态的创建文件
> @todo 所以 /dev 下的文件的作用到底是什么 ?

#### 3.2 VFS Handling of Device Files

The service routine of the `open()` system call also invokes the `dentry_open()` function, which allocates a new
file object and sets its `f_op` field to the address stored in `i_fop`—that is, to the address
of `def_blk_fops` or `def_chr_fops` once again.

> @todo `dentry_open` 的作用是什么 ?

## 4 Device Drivers

There are many types of device drivers. They mainly differ in the level of support that
they offer to the User Mode applications, as well as in their **buffering strategies** for
the data collected from the hardware devices. Because these choices greatly influence the internal structure of a device driver, we discuss them in the sections “Direct
Memory Access (DMA)” and “Buffering Strategies for Character Devices.”

#### 4.1 Device Driver Registration
> @todo device driver registration 一定会在 fs 中间创建出来文件，进而向用户空间提供接口吗 ?

If a device driver is statically compiled in the kernel, its registration is performed during the kernel initialization phase.
Conversely, if a device driver is compiled as a kernel module (see Appendix B), its registration is performed when the module is
loaded. In the latter case, the device driver can also unregister itself when the module is unloaded.

Let us consider, for instance, a generic PCI device. To properly handle it, its device
driver must allocate a descriptor of type `pci_driver`, which is used by the PCI kernel
layer to handle the device. After having initialized some fields of this descriptor, the
device driver invokes the `pci_register_driver()` function. Actually, the pci_driver
descriptor includes an embedded device_driver descriptor (see the earlier section
“Components of the Device Driver Model”); the `pci_register_function()` simply initializes the fields of the embedded driver descriptor and invokes driver_register()
to insert the driver in the data structures of the device driver model.
```c

/**
 * device_add - add device to device hierarchy.
 * @dev: device.
 *
 * This is part 2 of device_register(), though may be called
 * separately _iff_ device_initialize() has been called separately.
 *
 * This adds @dev to the kobject hierarchy via kobject_add(), adds it
 * to the global and sibling lists for the device, then
 * adds it to the other relevant subsystems of the driver model.
 *
 * Do not call this routine or device_register() more than once for
 * any device structure.  The driver model core is not designed to work
 * with devices that get unregistered and then spring back to life.
 * (Among other things, it's very hard to guarantee that all references
 * to the previous incarnation of @dev have been dropped.)  Allocate
 * and register a fresh new struct device instead.
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up your
 * reference instead.
 *
 * Rule of thumb is: if device_add() succeeds, you should call
 * device_del() when you want to get rid of it. If device_add() has
 * *not* succeeded, use *only* put_device() to drop the reference
 * count.
 */
int device_add(struct device *dev)
```
> @todo

When a device driver is being registered, the kernel looks for unsupported hardware
devices that could be possibly handled by the driver. To do this, it relies on the `match`
method of the relevant bus_type bus type descriptor, and on the probe method of the
`device_driver` object. If a hardware device that can be handled by the driver is discovered, the kernel allocates a device object and invokes device_register() to insert
the device in the device driver model.

#### 4.2 Device Driver Initialization

We already have seen an example in the section “I/O Interrupt Handling” in
Chapter 4: **the assignment of IRQs to devices is usually made dynamically**, right
before using them, because several devices may share the same IRQ line. *Other
resources that can be allocated at the last possible moment are page frames for DMA
transfer buffers and the DMA channel itself (for old non-PCI devices such as the
floppy disk driver).*
> 想要说明资源的推迟创建 ?

To make sure the resources are obtained when needed but are not requested in a
redundant manner when they have already been granted, device drivers usually
adopt the following schema:
- A usage counter keeps track of the number of processes that are currently accessing the device file. The counter is increased in the open method of the device file
and decreased in the release method.
- The `open` method checks the value of the usage counter before the increment. If
the counter is zero, the device driver must allocate the resources and enable
interrupts and DMA on the hardware device.
- The `release` method checks the value of the usage counter after the decrement.
If the counter is zero, no more processes are using the hardware device. If so, the
method disables interrupts and DMA on the I/O controller, and then releases
the allocated resources.

#### 4.3 Monitoring I/O Operations
This is a typical case in which it is preferable to implement the driver using the interrupt mode. Essentially, the driver includes two functions:
1. The `foo_read()` function that implements the read method of the file object.
2. The `foo_interrupt()` function that handles the interrupt.
> @todo 找到这两个函数真正的实现，有一说一，这两个例子很好。

#### 4.4 Accessing the I/O Shared Memory
--已经被整理--

#### 4.5 Direct Memory Access (DMA)

when the data transfer is completed, the DMA
issues an interrupt request. The conflicts that occur when CPUs and DMA circuits
need to access the same memory location at the same time are resolved by a hardware circuit called a *memory arbiter*.

The DMA is mostly used by disk drivers and other devices that transfer a large number of bytes at once. Because setup time for the DMA is relatively high, it is more efficient to directly use the CPU for the data transfer when the number of bytes is small.


* ***Synchronous and asynchronous DMA***

A device driver can use the DMA in two different ways called synchronous DMA and
asynchronous DMA. In the first case, the data transfers are triggered by processes; in
the second case the data transfers are triggered by hardware devices.
> synchronous 和 asynchronous 的含义是 interrupt 那种含义

* ***Helper functions for DMA transfers***

* ***Bus addresses***

Until now we have distinguished three kinds of memory addresses: logical and linear
addresses, which are used internally by the CPU, and physical addresses, which are
the memory addresses used by the CPU to physically drive the data bus. However,
there is a fourth kind of memory address: the so-called **bus address**. It corresponds to
the memory addresses used by all hardware devices except the CPU to drive the data
bus.
> @todo logical and linear address 的关系是什么来着 ?

Why should the kernel be concerned at all about bus addresses? Well, in a DMA
operation, the data transfer takes place without CPU intervention; the data bus is
driven directly by the I/O device and the DMA circuit. **Therefore, when the kernel
sets up a DMA operation, it must write the bus address of the memory buffer
involved in the proper I/O ports of the DMA or I/O device.**
> 当内核发起一个 DMA operation 的时候，它的写入操作比如进行在 DMA 或者 I/O device 合适的位置上。

**In the 80x86 architecture, bus addresses coincide with physical addresses.** However, other architectures such as Sun’s SPARC and Hewlett-Packard’s Alpha include
a hardware circuit called the I/O Memory Management Unit (IO-MMU), analog to
the paging unit of the microprocessor, which maps physical addresses into bus
addresses. All I/O drivers that make use of DMAs must set up properly the IO-MMU
before starting the data transfer.


> 后面介绍 dma_addr_t 和 pci_set_dma_mask
```c
/*
 * A dma_addr_t can hold any valid DMA address, i.e., any address returned
 * by the DMA API.
 *
 * If the DMA API only uses 32-bit addresses, dma_addr_t need only be 32
 * bits wide.  Bus addresses, e.g., PCI BARs, may be wider than 32 bits,
 * but drivers do memory-mapped I/O to ioremapped kernel virtual addresses,
 * so they don't care about the size of the actual bus addresses.
 */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif

#ifdef CONFIG_PCI
static inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_mask(&dev->dev, mask);
}

static inline int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_coherent_mask(&dev->dev, mask);
}
#else
static inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{ return -EIO; }
static inline int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask)
{ return -EIO; }
#endif
```

* ***Cache coherency***

Device driver developers may handle DMA buffers in two different ways by making
use of two different classes of helper functions. Using Linux terminology, the developer chooses between two different DMA mapping types:
- Coherent DMA mapping

  When using this mapping, the kernel ensures that there will be no cache coherency problems between the memory and the hardware device; this means that
  every write operation performed by the CPU on a RAM location is immediately
  visible to the hardware device, and vice versa. This type of mapping is also called
  “synchronous” or “consistent.”

- Streaming DMA mapping

  When using this mapping, the device driver must take care of cache coherency
  problems by using the proper synchronization helper functions. This type of
  mapping is also called “asynchronous” or “non-coherent.”

In the 80x86 architecture there are never cache coherency problems when using the
DMA, because the hardware devices themselves take care of “snooping” the accesses
to the hardware caches.


* ***Helper functions for coherent DMA mappings***

Usually, the device driver allocates the memory buffer and establishes the coherent
DMA mapping in the initialization phase; it releases the mapping and the buffer when
it is unloaded. To allocate a memory buffer and to establish a coherent DMA mapping, the kernel provides the architecture-dependent pci_alloc_consistent() and
dma_alloc_coherent() functions. They both return the linear address and the bus
address of the new buffer. In the 80 × 86 architecture, they return the linear address
and the physical address of the new buffer. To release the mapping and the buffer, the
kernel provides the pci_free_consistent() and the dma_free_coherent() functions.


```c
static inline void *dma_alloc_coherent(struct device *dev, size_t size,
		dma_addr_t *dma_handle, gfp_t gfp)
{

	return dma_alloc_attrs(dev, size, dma_handle, gfp,
			(gfp & __GFP_NOWARN) ? DMA_ATTR_NO_WARN : 0);
}

static inline void dma_free_coherent(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t dma_handle)
{
	return dma_free_attrs(dev, size, cpu_addr, dma_handle, 0);
}
```

* ***Helper functions for streaming DMA mappings***

> skip

## 4.6 Levels of Kernel Support
The Linux kernel does not fully support all possible existing I/O devices. Generally
speaking, in fact, there are three possible kinds of support for a hardware device:

- No support at all

  The application program interacts directly with the device’s I/O ports by issuing
  suitable in and out assembly language instructions.

- Minimal support

  The kernel does not recognize the hardware device, but does recognize its I/O
  interface. User programs are able to treat the interface as a sequential device
  capable of reading and/or writing sequences of characters.

- Extended support

  The kernel recognizes the hardware device and handles the I/O interface itself.
  In fact, there might not even be a device file for the device.

## 5 Character Device Drivers
> skip several pages, char dev, not interested, may be read later !
