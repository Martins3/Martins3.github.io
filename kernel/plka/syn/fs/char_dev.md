# fs/char_dev.c 分析
char_dev 的内容相比较而言，没有 address_space_operations 和 super_operations 的功能，很显然的事情。

## TODO
1. 分析一下 char_dev.c 和 block_dev.c 的对称性在哪里 ? 两者应该是对称的吧 !(处理的主要是注册问题和设备查找)

2. why chardev registered will appear on /dev/char/1:1

3. mknod /dev/kbd c 42 0

## misc
```c
// 1. 通过char的device设备查找，注册分析block设备的查找问题
// 2. char 提供一个蛇皮设备，也是值的分析的好东西

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
const struct file_operations def_chr_fops = {
	.open = chrdev_open,
	.llseek = noop_llseek,
};
```

## register_chrdev_region
1. alloc a char_device_struct into `chrdevs`

```c
/**
 * register_chrdev_region() - register a range of device numbers
 * @from: the first in the desired range of device numbers; must include
 *        the major number.
 * @count: the number of consecutive device numbers required
 * @name: the name of the device or driver.
 *
 * Return value is zero on success, a negative error code on failure.
 */
int register_chrdev_region(dev_t from, unsigned count, const char *name)


/*
 * Register a single major with a specified minor range.
 *
 * If major == 0 this function will dynamically allocate an unused major.
 * If major > 0 this function will attempt to reserve the range of minors
 * with given major.
 *
 */
static struct char_device_struct *
__register_chrdev_region(unsigned int major, unsigned int baseminor,
			   int minorct, const char *name)
// only one char_device_struct will allocated
```


```c
static struct char_device_struct {
	struct char_device_struct *next;
	unsigned int major;
	unsigned int baseminor;
	int minorct;
	char name[64];
	struct cdev *cdev;		/* will die */
} *chrdevs[CHRDEV_MAJOR_HASH_SIZE]; // chrdevs is global array containing all the char_device_struct pointers
```

## 

```c
/**
 * cdev_add() - add a char device to the system
 * @p: the cdev structure for the device
 * @dev: the first device number for which this device is responsible
 * @count: the number of consecutive minor numbers corresponding to this
 *         device
 *
 * cdev_add() adds the device represented by @p to the system, making it
 * live immediately.  A negative error code is returned on failure.
 */
int cdev_add(struct cdev *p, dev_t dev, unsigned count)
{
	int error;

	p->dev = dev;
	p->count = count;

	error = kobj_map(cdev_map, dev, count, NULL,
			 exact_match, exact_lock, p);
	if (error)
		return error;

	kobject_get(p->kobj.parent);

	return 0;
}
```

## register_chrdev : register a chrdev is easy
1. `__register_chrdev_region` : alloc char_device_struct to `chrdevs`
2. cdev_alloc : alloc cdev and init
3. cdev_add : add a char device to the system

```c
static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
{
	return __register_chrdev(major, 0, 256, name, fops);
}
```
