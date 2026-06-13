# sysfs kobject

## 重点分析一下 kernel 中的 device driver 等 obj module

## struct device 在 sysfs 的展示

device::devt
```c
	dev_t			devt;	/* dev_t, creates the sysfs "dev" */
	u32			id;	/* device instance */
```

和 set_dev_info 有关?

在 sysfs 中的确可以找到很多 dev 的文件，但是所有的设备都有这个吗?

sysfs 中的 dev 文件:
```c
static ssize_t dev_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return print_dev_t(buf, dev->devt);
}
static DEVICE_ATTR_RO(dev);
```

## /sys/devices 目录

- /sys/devices/virtual/misc/iommu
- /sys/devices/virtual/iommu

## 看看这个结构体
```c
struct bus_type {
	const char		*name;
	const char		*dev_name;
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, const struct device_driver *drv);
	int (*uevent)(const struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	void (*sync_state)(struct device *dev);
	void (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

	int (*num_vf)(struct device *dev);

	int (*dma_configure)(struct device *dev);
	void (*dma_cleanup)(struct device *dev);

	const struct dev_pm_ops *pm;

	bool need_parent_lock;
};
```

其中这个
```c
.num_vf		= pci_bus_num_vf,
```

真的是有趣的 nsim_bus 啊，也许是切入 bus 的好机会
```c
static const struct bus_type nsim_bus = {
	.name		= DRV_NAME,
	.dev_name	= DRV_NAME,
	.bus_groups	= nsim_bus_groups,
	.probe		= nsim_bus_probe,
	.remove		= nsim_bus_remove,
	.num_vf		= nsim_num_vf,
};
```

## 代码分析

## 关健结构体

### kobject
```c
struct kobject {
	const char		*name;
	struct list_head	entry;
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct kernfs_node	*sd; /* sysfs directory entry */
	struct kref		kref;
  // ...
};
```

The meanings of the individual elements of struct kobject are as follows:
1. `k_name` is a text name exported to userspace using sysfs. Sysfs is a virtual filesystem that allows for exporting various properties of the system into userspace. Likewise sd supports this connection, and I will come back to this in Chapter 10.
1. `kref` holds the general type struct kref designed to simplify reference management. I discuss this below.
1. `entry` is a standard list element used to group several kobjects in a list (known as a set in this case).
1. `kset` is required when an object is grouped with other objects in a set.
1. `parent` is a pointer to the parent element and enables a hierarchical structure to be established between kobjects.
1. `ktype` provides more detailed information on the data structure in which a kobject is embedded. Of greatest importance is the destructor function that returns the resources of the embedding data structure.

### kset
```c
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

1. kobjects are included in a hierarchic organization; most important, they can have a parent and
can be included in a kset. This determines where the kobject appears in the sysfs hierarchy: If
a parent exists, a new entry in the directory of the parent is created. Otherwise, it is placed in the
directory of the kobject that belongs to the kset the object is contained in (if both of these possibilities fail,
the entry for the kobject is located directly in the top level of the system hierarchy,
but this is obviously a rare case).
2. Every kobject is represented as a directory within sysfs. The files that appear in this directory
are the attributes of the object.
The operations used to export and set attribute values are provided by the subsystem (class, driver, etc.) to which the kobject belongs.
3. Buses, devices, drivers, and classes are the main kernel objects using the kobject mechanism;
they thus account for nearly all entries of sysfs

### /sys/class
```c
  static struct kset *class_kset;
```
具体的构建在 class_register 中，我们可以看到 /sys/class 下每一个目录中都是软连接
这都是通过 device_add_class_symlinks 来实现的

### sysfs 中的软链接如何形成的

/sys/block 下的软链接:
```c
	ret = sysfs_create_link(block_depr, &ddev->kobj,
				kobject_name(&ddev->kobj));
```

iommu_group_alloc_device 中:
```c
	ret = sysfs_create_link(&dev->kobj, &group->kobj, "iommu_group");
	if (ret)
		goto err_free_device;
```
/sys/devices/pci0000:00/0000:00:08.0 中可以构建出来:
```txt
 iommu_group -> ../../../kernel/iommu_groups/8
```

### /sys/devices/pci0000:00/0000:00:08.0 下的文件都是如何创建的

在 bus_add_device 中:
```c
	error = device_add_groups(dev, sp->bus->dev_groups);
	if (error)
		goto out_put;
```

不同的 driver 注册的地方不一样，例如:
```c
const struct bus_type pci_bus_type = {
	.name		= "pci",
	.match		= pci_bus_match,
	.uevent		= pci_uevent,
	.probe		= pci_device_probe,
	.remove		= pci_device_remove,
	.shutdown	= pci_device_shutdown,
	.dev_groups	= pci_dev_groups,
	.bus_groups	= pci_bus_groups,
	.drv_groups	= pci_drv_groups,
	.pm		= PCI_PM_OPS_PTR,
	.num_vf		= pci_bus_num_vf,
	.dma_configure	= pci_dma_configure,
	.dma_cleanup	= pci_dma_cleanup,
};
```

- [ ] 这里的 pci_dev_groups  pci_bus_groups 和 pci_drv_groups 的细看一下，有趣的设计
## 问题

### 1. struct device 中的 kobj 是如何被利用的?
```c
struct device {
	struct kobject kobj;
```
### 2. sysfs 中所有的目录都是有对应的 kobj 的吗?

### 4. 找到顶级目录的形成过程

 block   bus   class   dev   devices   firmware   fs   kernel   module   power

```c
fs_kobj = kobject_create_and_add("fs", NULL);
power_kobj = kobject_create_and_add("power", NULL);
kernel_kobj = kobject_create_and_add("kernel", NULL);
dev_kobj = kobject_create_and_add("dev", NULL);
firmware_kobj = kobject_create_and_add("firmware", NULL);
block_depr = kobject_create_and_add("block", NULL);

devices_kset = kset_create_and_add("devices", &device_uevent_ops, NULL);
class_kset = kset_create_and_add("class", NULL, NULL);
module_kset = kset_create_and_add("module", &module_uevent_ops, NULL);
bus_kset = kset_create_and_add("bus", &bus_uevent_ops, NULL);
```


在 include/linux/kobject.h 中定义一些:
```c
/* The global /sys/kernel/ kobject for people to chain off of */
extern struct kobject *kernel_kobj;
/* The global /sys/kernel/mm/ kobject for people to chain off of */
extern struct kobject *mm_kobj;
/* The global /sys/hypervisor/ kobject for people to chain off of */
extern struct kobject *hypervisor_kobj;
/* The global /sys/power/ kobject for people to chain off of */
extern struct kobject *power_kobj;
/* The global /sys/firmware/ kobject for people to chain off of */
extern struct kobject *firmware_kobj;
```

经典定义:
- devices_init


[sysfs 和 proc 的区别](https://unix.stackexchange.com/questions/4884/what-is-the-difference-between-procfs-and-sysfs)，更加的结构化。

sysfs 应该都是各种 kobject 或者 device 之类的暴露 !

### filesystem part

2. 基于 kernfs，将 vfs 的操作 和 attribute 提供的参数关联起来。

> 从文件系统的角度分析，为什么可以创建出来这些内容

```c
int sysfs_create_file(struct kobject * kobj, const struct attribute * attr);
void sysfs_remove_file(struct kobject * kobj, const struct attribute * attr);
```

Documentation/filesystems/sysfs.txt : 说对于每一个文件，其 show 和 store 函数存在一个 page 作为 buffer
的，关于这一点验证，以及说明很多规则，关于这一点的说明，应该在 kernfs 中间 :

```c
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
```

### 关键文档
- https://www.kernel.org/doc/html/latest//driver-api/driver-model/index.html 多是泛泛而谈
- https://www.kernel.org/doc/html/latest//filesystems/sysfs.html
- https://docs.kernel.org/core-api/kobject.html : kobject kset 真正的关键

### function

The kernel provides a representation of its model in userspace through the sysfs virtual file system. It is usually mounted in the /sys directory and contains the following subdirectories:

block - all block devices available in the system (disks, partitions)
bus - types of bus to which physical devices are connected (pci, ide, usb).两个子目录 devices 和 drivers 表示关联到该 bus 的设备和驱动
class - drivers classes that are available in the system (net, sound, usb)
devices - the hierarchical structure of devices connected to the system.
firmware - information from system firmware (ACPI)
fs - information about mounted file systems: 提供文件系统以及设备的信息。
kernel - kernel status information (logged-in users, hotplug)
modules - the list of modules currently loaded
power - information related to the power management subsystem
dev - It can be used to automatically create entries in the /dev directory_. 包含 char 和 block 两个文件夹，其中都是 symbol link
dev/ contains two directories char/ and block/. Inside these two
directories there are symlinks named <major>:<minor>. These symlinks
point to the sysfs directory for the given device. /sys/dev provides a
quick way to lookup the sysfs interface for a device from the result of
a stat(2) operation.

As you can see, there is a correlation between the kernel data structures within the described model and the subdirectories in the sysfs virtual file system. Although this likeness may lead to confusion between the two concepts, they are different. The kernel device model can work without the sysfs file system, but the reciprocal is not true.

1. For every kobject that is registered with the system, a directory is created for it in sysfs.
      > 每一个 kobject 在 sysfs 中间对应一个文件夹
2. That directory is created as a subdirectory of the kobject's parent, expressing internal object hierarchies to userspace.
      > 一个 kobject 对应的文件夹在其 parent 对应的文件夹中间
3. Top-level directories in sysfs represent the common ancestors of object hierarchies; i.e. the subsystems the objects belong to.

4. Attributes can be exported for _kobjects_ in the form of _regular files_ in the filesystem.
   并且提供文件属性
      > 属性在 sfs 中间的存在形式是 文件

### kobject

kobject --- sysfs ---- kernfs


```plain
kobject创建：
-----------
struct kobject *kobject_create_and_add(const char *name, struct kobject *parent)
	int kobject_add(struct kobject *kobj, struct kobject *parent, const char *fmt, ...)
		static int object_add_varg (vargs=...
			static int kobject_add_internal(struct kobject *kobj)
				static int create_dir(struct kobject *kobj)
					int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)  //sysfs_root_kn
						struct kernfs_node *kernfs_create_dir_ns(struct kernfs_node *parent,
							struct kernfs_node *kernfs_new_node(struct kernfs_node *parent,

kset创建：
--------
struct kset *kset_create_and_add(const char *name,
	int kset_register(struct kset *k)
		static int kobject_add_internal(struct kobject *kobj)
```

bus_register 中

```c
	priv->devices_kset = kset_create_and_add("devices", NULL, bus_kobj);
	if (!priv->devices_kset) {
		retval = -ENOMEM;
		goto bus_devices_fail;
	}

	priv->drivers_kset = kset_create_and_add("drivers", NULL, bus_kobj);
	if (!priv->drivers_kset) {
		retval = -ENOMEM;
		goto bus_drivers_fail;
	}
```

最后得到:
- /sys/bus/pci/drivers/
- /sys/bus/pci/devices

## 笑死，以前的 config 也加的太随意了

通过这个方法，让所有的 pr_debug 变成 pr_info
```txt
ifeq ($(CONFIG_DEBUG_KOBJECT),y)
CFLAGS_kobject.o += -DDEBUG
CFLAGS_kobject_uevent.o += -DDEBUG
endif
```

## 笔记
1. `bus_find_device_by_name` : 存在这种函数 !
2. **Other possible operations on a bus are browsing the drivers or devices attached to it**.
Although we can not directly access them (lists of drives and devices being stored in the private data of the driver, the subsys_private * p field ),
these can be scanned using the *bus_for_each_dev* and *bus_for_each_drv* macrodefines .


## 资料

```c
static inline const char *dev_name(const struct device *dev)
{
  /* Use the init name until the kobject becomes available */
  if (dev->init_name)
    return dev->init_name;

  return kobject_name(&dev->kobj);
}

static inline const char *kobject_name(const struct kobject *kobj)
{
  return kobj->name;
}
```

```c
/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int dev_set_name(struct device *dev, const char *fmt, ...)
{
  va_list vargs;
  int err;

  va_start(vargs, fmt);
  err = kobject_set_name_vargs(&dev->kobj, fmt, vargs); // 调用kvasprintf_const，然后给 kobj 赋值
  va_end(vargs);
  return err;
}
```


```c

/**
 * A bus is a channel between the processor and one or more devices. For the
 * purposes of the device model, all devices are connected via a bus, even if
 * it is an internal, virtual, "platform" bus. Buses can plug into each other.
 * A USB controller is usually a PCI device, for example. The device model
 * represents the actual connections between buses and the devices they control.
 * A bus is represented by the bus_type structure. It contains the name, the
 * default attributes, the bus' methods, PM operations, and the driver core's
 * private data.
 */
struct bus_type {
  const char    *name;
  const char    *dev_name;
  struct device   *dev_root;
  const struct attribute_group **bus_groups;
  const struct attribute_group **dev_groups;
  const struct attribute_group **drv_groups;

  int (*match)(struct device *dev, struct device_driver *drv);
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  int (*probe)(struct device *dev);
  int (*remove)(struct device *dev);
  void (*shutdown)(struct device *dev);

  int (*online)(struct device *dev);
  int (*offline)(struct device *dev);

  int (*suspend)(struct device *dev, pm_message_t state);
  int (*resume)(struct device *dev);

  int (*num_vf)(struct device *dev);

  int (*dma_configure)(struct device *dev);

  const struct dev_pm_ops *pm; // 电池管理

  const struct iommu_ops *iommu_ops;

  struct subsys_private *p;
  struct lock_class_key lock_key;

  bool need_parent_lock;
};
```

[](http://www.staroceans.org/kernel-and-driver/The%20Linux%20Kernel%20Driver%20Model.pdf)


```c
struct attribute {
  const char    *name;
  umode_t     mode;
};

/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name: Optional: Attribute group name
 *    If specified, the attribute group will be created in
 *    a new subdirectory with this name.
 * @is_visible: Optional: Function to return permissions associated with an
 *    attribute of the group. Will be called repeatedly for each
 *    non-binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if an attribute is not visible. The returned value
 *    will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *    Optional: Function to return permissions associated with a
 *    binary attribute of the group. Will be called repeatedly
 *    for each binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if a binary attribute is not visible. The returned
 *    value will replace static permissions defined in
 *    struct bin_attribute.
 * @attrs:  Pointer to NULL terminated list of attributes.
 * @bin_attrs:  Pointer to NULL terminated list of binary attributes.
 *    Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
  const char    *name;
  umode_t     (*is_visible)(struct kobject *,
                struct attribute *, int);
  umode_t     (*is_bin_visible)(struct kobject *,
              struct bin_attribute *, int);
  struct attribute  **attrs;
  struct bin_attribute  **bin_attrs;
};
```

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

  const struct dev_pm_ops *pm;
};
```

```c
/**
 * The device driver-model tracks all of the drivers known to the system.
 * The main reason for this tracking is to enable the driver core to match
 * up drivers with new devices. Once drivers are known objects within the
 * system, however, a number of other things become possible. Device drivers
 * can export information and configuration variables that are independent
 * of any specific device.
 */
struct device_driver {
  const char    *name;
  struct bus_type   *bus;

  struct module   *owner;
  const char    *mod_name;  /* used for built-in modules */

  bool suppress_bind_attrs; /* disables bind/unbind via sysfs */
  enum probe_type probe_type;

  const struct of_device_id *of_match_table;
  const struct acpi_device_id *acpi_match_table;

  int (*probe) (struct device *dev);
  int (*remove) (struct device *dev);
  void (*shutdown) (struct device *dev);
  int (*suspend) (struct device *dev, pm_message_t state);
  int (*resume) (struct device *dev);
  const struct attribute_group **groups;

  const struct dev_pm_ops *pm;
  void (*coredump) (struct device *dev);

  struct driver_private *p;
};
```


```c
  /*
   * device_add does this:
   *    bus_add_device(dev)
   *    ->device_attach(dev)
   *      ->for each driver drv registered on the bus that dev is on
   *          if (dev.drv)  **  device already has a driver **
   *            ** not sure we could ever get here... **
   *          else
   *            if (bus.match(dev,drv)) [visorbus_match]
   *              dev.drv = drv
   *              if (!drv.probe(dev))  [visordriver_probe_device]
   *                dev.drv = NULL
   *
   * Note that device_add does NOT fail if no driver failed to claim the
   * device.  The device will be linked onto bus_type.klist_devices
   * regardless (use bus_for_each_dev).
   */
```

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
 */
int device_add(struct device *dev)
```

## kobject kset
主要参考文档 : Documentation/kobject.txt 介绍了几个 API ，其实也没有啥。

1. kset : A kset is a group of kobjects.  These kobjects can be of the same ktype
   or belong to different ktypes.  The kset is the basic container type for
   collections of kobjects. Ksets contain their own kobjects, but you can
   safely ignore that implementation detail as the kset core code handles
   this kobject automatically.
   When you see a sysfs directory full of other directories, generally each
   of those directories corresponds to a kobject in the same kset.
2. ktype 控制 kobject 的创建和销毁。(@todo 那么为什么叫做 type ，不叫 initializer 呀 ?)

kset 内部本身也包含一个 kobj 对象，在 sysfs 中也表现为目录；所不同的是，kset 要承担 kobj 状态变动消息的发送任务

- [ ] [kobject](https://www.kernel.org/doc/Documentation/kobject.txt)
- [ ] [The zen of kobjects](https://lwn.net/Articles/51437/)


```c
struct kobject {
  const char    *name; // 用于在 sysfs 中间显示 ?
  struct list_head  entry; // 这个用于 kset 将 kobject 连接起来
  struct kobject    *parent;
  struct kset   *kset;
  struct kobj_type  *ktype;
  struct kernfs_node  *sd; /* sysfs directory entry */
  struct kref   kref; // 引用计数的作用 : bus 不能在挂到其上的设备还没有 release 的情况下就 release, 只有没有人设备以及驱动关联的时候，才可以
  unsigned int state_initialized:1;
  unsigned int state_in_sysfs:1;
  unsigned int state_add_uevent_sent:1;
  unsigned int state_remove_uevent_sent:1;
  unsigned int uevent_suppress:1;
};
```
name : 同时也是 sysfs 中的目录名称。由于 Kobject 添加到 Kernel 时，需要根据名字注册到 sysfs 中，之后就不能再直接修改该字段。如果需要修改 Kobject 的名字，需要调用 kobject_rename 接口，该接口会主动处理 sysfs 的相关事宜


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
  const char    *name;
  umode_t     mode;
};
```

2. attribute 和 attribute group 说明 : attribute 的架构是什么样子的 ?
```c
/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name: Optional: Attribute group name
 *    If specified, the attribute group will be created in
 *    a new subdirectory with this name.
 * @is_visible: Optional: Function to return permissions associated with an
 *    attribute of the group. Will be called repeatedly for each
 *    non-binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if an attribute is not visible. The returned value
 *    will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *    Optional: Function to return permissions associated with a
 *    binary attribute of the group. Will be called repeatedly
 *    for each binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if a binary attribute is not visible. The returned
 *    value will replace static permissions defined in
 *    struct bin_attribute.
 * @attrs:  Pointer to NULL terminated list of attributes.
 * @bin_attrs:  Pointer to NULL terminated list of binary attributes.
 *    Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
  const char    *name;
  umode_t     (*is_visible)(struct kobject *,
                struct attribute *, int);
  umode_t     (*is_bin_visible)(struct kobject *,
              struct bin_attribute *, int);
  struct attribute  **attrs;
  struct bin_attribute  **bin_attrs;
};
```

3. attribute 可以关联到那些对象上 :

4. bin_attribute 其作用体现在 ?

```c
struct bin_attribute {
  struct attribute  attr;
  size_t      size;
  void      *private;
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
  struct attribute  attr;
  ssize_t (*show)(struct bus_type *bus, char *buf);
  ssize_t (*store)(struct bus_type *bus, const char *buf, size_t count);
};

/* interface for exporting device attributes */
struct device_attribute {
  struct attribute  attr;
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

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
