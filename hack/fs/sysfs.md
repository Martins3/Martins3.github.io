# sysfs  

<!-- vim-markdown-toc GitLab -->

- [Implementation](#implementation)
    - [filesystem part](#filesystem-part)
    - [device model part](#device-model-part)
- [function](#function)
    - [](#)
- [kobject](#kobject)

<!-- vim-markdown-toc -->

[sysfs 和 proc 的区别](https://unix.stackexchange.com/questions/4884/what-is-the-difference-between-procfs-and-sysfs)，更加的结构化。

https://www.ibm.com/developerworks/cn/linux/l-cn-sysfs/

## Implementation

#### filesystem part
1. sysfs 也是和 ns 相关的，其中的 ns 如何使用 ?

2. 基于 kernfs，将 vfs 的操作 和 attribute 提供的参数关联起来。


> 从文件系统的角度分析，为什么可以创建出来这些内容
```c
struct attribute {
        char                    * name;
        umode_t                 mode;
};


int sysfs_create_file(struct kobject * kobj, const struct attribute * attr);
void sysfs_remove_file(struct kobject * kobj, const struct attribute * attr);

static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}

/**
 * device_create_file - create sysfs attribute file for device.
 * @dev: device.
 * @attr: device attribute descriptor.
 */
int device_create_file(struct device *dev,
		       const struct device_attribute *attr) // 只是功能变的复杂了，但是内容并没有什么变化
{
	int error = 0;

	if (dev) {
		WARN(((attr->attr.mode & S_IWUGO) && !attr->store),
			"Attribute %s: write permission without 'store'\n",
			attr->attr.name);
		WARN(((attr->attr.mode & S_IRUGO) && !attr->show),
			"Attribute %s: read permission without 'show'\n",
			attr->attr.name);
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	}

	return error;
}
```

Documentation/filesystems/sysfs.txt : 说对于每一个文件，其 show 和 store 函数存在一个 page 作为 buffer
的，关于这一点验证，以及说明很多规则，关于这一点的说明，应该在 kernfs 中间 :

```c
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
```


#### device model part
> 到底提供了那些帮助

## function

1. 为什么需要 sfsfs ? 
2. 如何将设备的信息通过 sfsfs export 出来 ?
> 分析一下 /sys/ 的内容


#### 
More information can driver-model specific features can be found in
Documentation/driver-api/driver-model/.

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
dev -  *It can be used to automatically create entries in the /dev directory*. 包含char 和 block 两个文件夹，其中都是 symbol link
dev/ contains two directories char/ and block/. Inside these two
directories there are symlinks named <major>:<minor>.  These symlinks
point to the sysfs directory for the given device.  /sys/dev provides a
quick way to lookup the sysfs interface for a device from the result of
a stat(2) operation.

As you can see, there is a correlation between the kernel data structures within the described model and the subdirectories in the sysfs virtual file system. Although this likeness may lead to confusion between the two concepts, they are different. The kernel device model can work without the sysfs file system, but the reciprocal is not true.



sysfs 是可有可无的，
sysfs is always compiled in if CONFIG_SYSFS is defined. You can access
it by doing:

    mount -t sysfs sysfs /sys 

1. For every kobject that is registered with the system, a directory is created for it in sysfs.
> 每一个 kobject 在 sysfs 中间对应一个文件夹
2. That directory is created as a subdirectory of the kobject's parent, expressing internal object hierarchies to userspace. 
> 一个 kobject 对应的文件夹在其 parent 对应的文件夹中间
3. Top-level directories in sysfs represent the common ancestors of object hierarchies; i.e. the subsystems the objects belong to. 

4. Attributes can be exported for *kobjects* in the form of *regular files* in the filesystem.
并且提供文件属性
> 属性在 sfs 中间的存在形式是 文件


## kobject
kobject --- sysfs ---- kernfs

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


```
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

