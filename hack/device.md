# linux 设备驱动


<!-- vim-markdown-toc GitLab -->

- [device model](#device-model)
    - [Hot Plug](#hot-plug)
    - [uevent](#uevent)
    - [kobject kset](#kobject-kset)
    - [attribute](#attribute)
    - [device](#device)
    - [driver](#driver)
    - [bus](#bus)
    - [class](#class)
    - [resource management](#resource-management)
- [device tree](#device-tree)
- [char device](#char-device)
- [block device](#block-device)
- [问题](#问题)
- [资源(整理成为 footnote)](#资源整理成为-footnote)

<!-- vim-markdown-toc -->

## device model
The simple idea is that the "devices" of the system show up in one single tree at /sys/devices. Devices of a common type have a "subsystem" symlink pointing back to the subsystem's directory. All devices of that subsystem are listed there.
- /sys/devices is the single unified hierarchy of all devices, used to express parent/child relationships
- /sys/{bus,class} is home of the subsystem, the grouping/listing of devices of this type, used to lookup devices by subsystem + name
The distinction of bus vs. class never made much sense. For that reason, udev internally merges the view of them and only exposes "subsystem", it refuses to distinguish "bus" and "class". [^3]




为什么需要设备模型，之前遇到的的问题是什么 ? 存在如下猜测
1. 需要 hot plug ? 所以构建出来一个 bus 类型，并且使用 device 和 device deriver
2. 处理 power 相关的 ? suspend 或者 resume 
2. 重复代码消除: 一个设备驱动可以处理不同设备控制器，甚至一个设备驱动可以支持多个类似的设备[^1]

设备模型 : platform bus 用于注销

Describing non-detectable devices　: device tree 或者 acpi
> 什么 device tree 的作用居然是这个

设备驱动在内核软件的架构:
- a framework that allows the driver to
expose the hardware features in a
generic way.
- a bus infrastructure, part of the device
model, to detect/communicate with the
hardware.

adapter driver are device deriver too !
> 图不错，但是还是理解不了呀
> 是不是存在多种 usb 驱动，那么是不是注册多个 adapter driver ? adapter deriver 的作用是什么 ?


> lld3 上分析的全部内容 :
1. linux 设备模型的核心要素 : bus device device_driver class
2. attribute sfsfs 
3. kobject kset **subsystem** 
4. hotplug

> wowtech 添加的内容

> 其中的 driver 的内容在哪里 ?

很多 device 不一定就是一个硬件设备，很多时候是作为一个接口，或者是为了使用 sfsfs


- For the same device, need to use the same device driver on
multiple CPU architectures (x86, ARM…), even though the
hardware controllers are different.
- Need for a single driver to support multiple devices of the
same kind.

#### Hot Plug


#### uevent
Uevent的机制是比较简单的(大概1000行)，设备模型中任何设备有事件需要上报时，
会触发Uevent提供的接口。Uevent模块准备好上报事件的格式后，可以通过两个途径把事件上报到用户空间：
一种是通过kmod模块，直接调用用户空间的可执行文件；另一种是通过netlink通信机制，将事件从内核空间传递给用户空间。

http://www.wowotech.net/device_model/uevent.html


#### kobject kset
主要参考文档 : Documentation/kobject.txt 介绍了几个 API ，其实也没有啥。

1. kset : A kset is a group of kobjects.  These kobjects can be of the same ktype
   or belong to different ktypes.  The kset is the basic container type for
   collections of kobjects. Ksets contain their own kobjects, but you can
   safely ignore that implementation detail as the kset core code handles
   this kobject automatically.
   When you see a sysfs directory full of other directories, generally each
   of those directories corresponds to a kobject in the same kset.
2. ktype 控制 kobject 的创建和销毁。(@todo 那么为什么叫做 type ，不叫 initializer 呀 ?)

kset内部本身也包含一个kobj对象，在sysfs中也表现为目录；所不同的是，kset要承担kobj状态变动消息的发送任务

- [ ] [kobject](https://www.kernel.org/doc/Documentation/kobject.txt)
- [ ] [The zen of kobjects](https://lwn.net/Articles/51437/)


```c
struct kobject {
	const char		*name; // 用于在 sysfs 中间显示 ?
	struct list_head	entry; // 这个用于 kset 将 kobject 连接起来
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct kernfs_node	*sd; /* sysfs directory entry */
	struct kref		kref; // 引用计数的作用 : bus 不能在挂到其上的设备还没有 release 的情况下就 release, 只有没有人设备以及驱动关联的时候，才可以
	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;
};
```
name : 同时也是sysfs中的目录名称。由于Kobject添加到Kernel时，需要根据名字注册到sysfs中，之后就不能再直接修改该字段。如果需要修改Kobject的名字，需要调用kobject_rename接口，该接口会主动处理sysfs的相关事宜


提供的一个 API :


```c
    void kobject_init(struct kobject *kobj, struct kobj_type *ktype);

    int kobject_rename(struct kobject *kobj, const char *new_name);
    const char *kobject_name(const struct kobject * kobj);
    int kobject_uevent(struct kobject *kobj, enum kobject_action action);


    int kobject_uevent(struct kobject *kobj, enum kobject_action action);
    // 应该没有什么特殊的，就是发送信息而已
```

```c

    struct kobj_type {
	    void (*release)(struct kobject *kobj);
	    const struct sysfs_ops *sysfs_ops; // TODO 和 attr 的关系是什么 ? 也许分析 sys 之后就可以知道了吧!
	    struct attribute **default_attrs;
	    const struct kobj_ns_type_operations *(*child_ns_type)(struct kobject *kobj);
	    const void *(*namespace)(struct kobject *kobj);
    };
```

This structure is used to describe a particular type of kobject (or, more
correctly, of containing object). Every kobject needs to have an associated
kobj_type structure; a pointer to that structure must be specified when you
call kobject_init() or kobject_init_and_add().

The release field in struct kobj_type is, of course, a pointer to the
release() method for this type of kobject. The other two fields (sysfs_ops
and default_attrs) control how objects of this type are represented in
sysfs; they are beyond the scope of this document.

The default_attrs pointer is a list of default attributes that will be
automatically created for any kobject that is registered with this ktype.



#### attribute
1. 属性的作用 : 每次定义一个 attribute，那么在 sys 中间存在对应的文件，并且规定了该文件的 IO 方法。

```c
struct attribute {
	const char		*name;
	umode_t			mode;
};
```

2. attribute 和 attribute group 说明 : attribute 的架构是什么样子的 ?
```c
/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name:	Optional: Attribute group name
 *		If specified, the attribute group will be created in
 *		a new subdirectory with this name.
 * @is_visible:	Optional: Function to return permissions associated with an
 *		attribute of the group. Will be called repeatedly for each
 *		non-binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if an attribute is not visible. The returned value
 *		will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *		Optional: Function to return permissions associated with a
 *		binary attribute of the group. Will be called repeatedly
 *		for each binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if a binary attribute is not visible. The returned
 *		value will replace static permissions defined in
 *		struct bin_attribute.
 * @attrs:	Pointer to NULL terminated list of attributes.
 * @bin_attrs:	Pointer to NULL terminated list of binary attributes.
 *		Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
	const char		*name;
	umode_t			(*is_visible)(struct kobject *,
					      struct attribute *, int);
	umode_t			(*is_bin_visible)(struct kobject *,
						  struct bin_attribute *, int);
	struct attribute	**attrs;
	struct bin_attribute	**bin_attrs;
};
```

3. attribute 可以关联到那些对象上 :

4. bin_attribute 其作用体现在 ?

```c
struct bin_attribute {
	struct attribute	attr;
	size_t			size;
	void			*private;
	ssize_t (*read)(struct file *, struct kobject *, struct bin_attribute *,
			char *, loff_t, size_t);
	ssize_t (*write)(struct file *, struct kobject *, struct bin_attribute *,
			 char *, loff_t, size_t);
	int (*mmap)(struct file *, struct kobject *, struct bin_attribute *attr,
		    struct vm_area_struct *vma);
};
```

5. 而且还存在几种进一步的封装 : 唯一的区别就是参数，其实就是提供一下
```c
struct bus_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct bus_type *bus, char *buf);
	ssize_t (*store)(struct bus_type *bus, const char *buf, size_t count);
};

/* interface for exporting device attributes */
struct device_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
};

struct driver_attribute {
	struct attribute attr;
	ssize_t (*show)(struct device_driver *driver, char *buf);
	ssize_t (*store)(struct device_driver *driver, const char *buf,
			 size_t count);
};
```

To add / delete an attribute within the bus structure, the `bus_create_file` and `bus_remove_file` functions are used

> TODO 可以处理一下
/home/shen/Core/linux/Documentation/kobject.txt

#### device
device_unregister 和 device_register 的作用 ?

To work with the attributes, we have structure atribute , DEVICE_ATTR macrodefine for definition, and device_create_file and device_remove_file functions to add the attribute to the device.
>  device_create_file 这种函数用于挂载 attr 到

The device_create and device_destroy functions are available for initialization / deinterlacing . 

```c
/*
 * The type of device, "struct device" is embedded in. A class
 * or bus can contain devices of different types
 * like "partitions" and "disks", "mouse" and "event".
 * This identifies the device type and carries type-specific
 * information, equivalent to the kobj_type of a kobject.
 * If "name" is specified, the uevent will contain it in
 * the DEVTYPE variable.
 */
struct device_type {
	const char *name;
	const struct attribute_group **groups;
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	char *(*devnode)(struct device *dev, umode_t *mode,
			 kuid_t *uid, kgid_t *gid);
	void (*release)(struct device *dev);

	const struct dev_pm_ops *pm; // 电源管理相关
};
```

#### driver
同样存在 driver_register 和 driver_unregister ?
To work with attributes, we have the driver_attribute structure , the macro definition of DRIVER_ATTR for definition, and the driver_create_file and driver_remove_file functions for adding the attribute to the device.


#### bus
bus_register 和 bus_unregister 的作用 ?
bus_create_file 

Other possible operations on a bus are browsing the drivers or devices attached to it.
Although we can not directly access them (lists of drives and devices being stored in the private data of the driver, the subsys_private * p field ), 
these can be scanned using the `bus_for_each_dev` and `bus_for_each_drv` macrodefines .

The `match` function is used when a new device or a new driver is added to the bus.
Its role is to make a comparison between the device ID and the driver ID.
The `uevent` function is called before generating a hotplug in user-space and has the role of adding environment variables.
> match 函数用于在 device 或者 driver 连接到 bus 上(device 和 device_driver 都是持有 bus_type 这个成员的，当其注册的时候，那么就是自动关联上对应的 bus )，但是 uevent 被 hotplug 触发

struct subsys_private中各个字段的解释：

subsys、devices_kset、drivers_kset是三个kset，kset是一个特殊的kobject，用来集合相似的kobject，它在sysfs中也会以目录的形式体现。其中subsys，代表了本bus（如/sys/bus/spi），它下面可以包含其它的kset或者其它的kobject；devices_kset和drivers_kset则是bus下面的两个kset（如/sys/bus/spi/devices和/sys/bus/spi/drivers），分别包括本bus下所有的device和device_driver。

interface是一个list head，用于保存该bus下所有的interface。有关interface的概念后面会详细介绍。

klist_devices和klist_drivers是两个链表，分别保存了本bus下所有的device和device_driver的指针，以方便查找。

drivers_autoprobe，用于控制该bus下的drivers或者device是否自动probe
bus和class指针，分别保存上层的bus或者class指针。

wowo : 分析过 bus_register bus_add_device bus_add_driver 的具体实现

#### class
The class_register and class_unregister functions for initialization / and cleanup :

class_create_file_ns

**两套接口原来大家都是存在的呀!**

sysfs中所有的目录以及目录下的文件都对应内核中的一个struct kobj实例。
bus目录中所有的子目录都对应内核中的一个struct bus_type实例。
class目录中所有的子目录对应内核中的一个struct class实例
devices目录中的所有目录以及子目录都对应一个struct device实例

#### resource management
内容在 : drivers/base/devres.c (wowo 还分析过具体的实现，暂时没有阅读的兴趣，至少等待KVM 和 namespace cgroup 看完再说吧) 提供一种机制，将系统中某个设备的所有资源，以链表的形式，组织起来，以便在driver detach的时候，自动释放。

a）power，供电。
b）clock，时钟。
c）memory，内存，在kernel中一般使用kzalloc分配。
d）GPIO，用户和CPU交换简单控制、状态等信息。
e）IRQ，触发中断。
f）DMA，无CPU参与情况下进行数据传输。
g）虚拟地址空间，一般使用ioremap、request_region等分配。
h）等等

```c
static inline void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
{
	return devm_kmalloc(dev, size, gfp | __GFP_ZERO);
}
```

## device tree
> 暂时看不懂，也没有兴趣分析

## char device

## block device

// TODO 感觉就像是以前从来接触过一样
```c
struct block_device_operations {
	int (*open) (struct block_device *, fmode_t);
	void (*release) (struct gendisk *, fmode_t);
	int (*rw_page)(struct block_device *, sector_t, struct page *, unsigned int);
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	unsigned int (*check_events) (struct gendisk *disk,
				      unsigned int clearing);
	/* ->media_changed() is DEPRECATED, use ->check_events() instead */
	int (*media_changed) (struct gendisk *);
	void (*unlock_native_capacity) (struct gendisk *);
	int (*revalidate_disk) (struct gendisk *);
	int (*getgeo)(struct block_device *, struct hd_geometry *);
	/* this callback is with swap_lock and sometimes page table lock held */
	void (*swap_slot_free_notify) (struct block_device *, unsigned long);
	int (*report_zones)(struct gendisk *, sector_t sector,
			unsigned int nr_zones, report_zones_cb cb, void *data);
	struct module *owner;
	const struct pr_ops *pr_ops;
};
```

## 问题
- [x] 到底一共存在多少总线类型 ? PCI PCIE I2C  ()
- [ ] major:minor ?
- [ ] 到底一共存在多少总线类型 ? PCI PCIE I2C 

这篇文章
A Hardware Architecture for Implementing Protection Rings
让我怀疑人生:
1. 这里描述的 gates 和 syscall 是什么关系 ?
2. syscall 可以使用 int 模拟实现吗 ?
3. interupt 和 exception 在架构实现上存在什么区别吗 ?


## 资源(整理成为 footnote)
1. https://www.linuxjournal.com/article/7353 : usb 驱动
2. https://www.linuxjournal.com/article/7136 : i2c 驱动
3. https://m.tb.cn/h.ViQNCvc?sm=09356c : 在实体机器上操作一下

https://opensource.com/article/18/11/udev : udev 的介绍
https://unix.stackexchange.com/questions/550037/udev-and-uevent-question : udev stackexchange

[^1]: https://events19.linuxfoundation.org/wp-content/uploads/2017/12/Introduction-to-Linux-Kernel-Driver-Programming-Michael-Opdenacker-Bootlin-.pdf
[^2]: https://lwn.net/Articles/645810/
[^3]: https://lwn.net/Articles/646514/
