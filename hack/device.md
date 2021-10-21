# linux 设备驱动


<!-- vim-markdown-toc GitLab -->

- [overview](#overview)
- [TODO](#todo)
- [Questions](#questions)
- [tty](#tty)
- [下面是阅读 ldd3 的 tiny_serial.c 和 tiny_tty.c 的结果](#下面是阅读-ldd3-的-tiny_serialc-和-tiny_ttyc-的结果)
  - [tiny_serial.c](#tiny_serialc)
- [usb](#usb)
- [serial](#serial)
- [i8042](#i8042)
- [platform_driver](#platform_driver)
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
- [pty pts tty](#pty-pts-tty)
- [问题](#问题)
  - [需要回答的问题](#需要回答的问题)
  - [可能有用的问题](#可能有用的问题)
  - [细节问题](#细节问题)
  - [具体内容](#具体内容)
  - [各种结构体](#各种结构体)
  - [习题](#习题)
- [资源(整理成为 footnote)](#资源整理成为-footnote)

<!-- vim-markdown-toc -->

## overview
> Let's start with a device[^4]
> - How does a driver program a device ?
> - How does a device signal the driver ?
> - How does a device transfer data ?

And, this page will contains anything related device except pcie, mmio, pio, interupt and dma.

- [ ] maybe tear this page into device model and concrete device
  - [ ] lack understanding of char_dev.c and block_dev.c

## TODO
*大致的探索了一下，感觉 keyboard 使用的是另一个体系的东西来连接 CPU, 不是 pcie 的，或者不是直接链接到 pcie 上的，ldd3 和 essential linux device driver : Input Device Drivers 都是可以好好看看的*
- [ ] ldd3 的 tty 存在一个巨大的仓库
- [ ] eldd 的部分章节也是可以看看的 http://www.embeddedlinux.org.cn/essentiallinuxdevicedrivers/final/ch06lev1sec3.html
- [ ] 微机原理的书可以看看的
- [ ] TTY 到底是什么？https://www.kawabangga.com/posts/4515

- [ ] tlpi : chapter 62

1. 阅读 /lib/modules(shell uname -a)/build 下的makefile 中间的内容
  1. 包含的头文件有点不对 asm 下头文件似乎没有被包含下来
  2. 应该不会指向glibc 的文件
2. 修改Makefile　产生的文件放置到指定的文件夹中间去
3. /dev 和 /proc/devices 两者的关系是什么
4. 为什么ebbchar是没有 手动mknod 的操作, **显然**应该含有对应的操作

6. 修改scull\_load 文件中间的内容
7. klogd syslogd

## Questions
- echo "shit" > /dev/pts/3
  - 所以，pts 到底是说个啥，tmux / screen 如何利用

## tty
- [ ] line ldisc
```c
static struct tty_ldisc_ops n_tty_ops = {
	.magic           = TTY_LDISC_MAGIC,
	.name            = "n_tty",
	.open            = n_tty_open,
	.close           = n_tty_close,
	.flush_buffer    = n_tty_flush_buffer,
	.read            = n_tty_read,
	.write           = n_tty_write,
	.ioctl           = n_tty_ioctl,
	.set_termios     = n_tty_set_termios,
	.poll            = n_tty_poll,
	.receive_buf     = n_tty_receive_buf,
	.write_wakeup    = n_tty_write_wakeup,
	.receive_buf2	 = n_tty_receive_buf2,
};
```



## 下面是阅读 ldd3 的 tiny_serial.c 和 tiny_tty.c 的结果

### tiny_serial.c
- [x] TINY_SERIAL_MAJOR 的数值设置不要和 /proc/devices 的数值冲突了

uart_register_driver + uart_add_one_port

insmod tiny_serial.ko 之后 ： 多出来了 devices/virtual/tty/ttytiny0

```c
➜  ttytiny0 tree
.
├── close_delay
├── closing_wait
├── custom_divisor
├── dev
├── flags
├── iomem_base
├── iomem_reg_shift
├── io_type
├── irq
├── line
├── port
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   ├── runtime_usage
│   ├── wakeup
│   ├── wakeup_abort_count
│   ├── wakeup_active
│   ├── wakeup_active_count
│   ├── wakeup_count
│   ├── wakeup_expire_count
│   ├── wakeup_last_time_ms
│   ├── wakeup_max_time_ms
│   └── wakeup_total_time_ms
├── subsystem -> ../../../../class/tty
├── type
├── uartclk
├── uevent
└── xmit_fifo_size
```

- [ ] 所以如何使用呀 ?
- [ ] termios 的作用 ?

insmod tiny_tty.ko
```c
devices/virtual/tty/ttty0
devices/virtual/tty/ttty1
devices/virtual/tty/ttty2
devices/virtual/tty/ttty3
```

为什么都需要 set_termios 之类的东西 ?

从 /dev/tty 看， tiny_serial 和 tiny_tty 无区别 ?


## usb
测试辅助模块 dummy_hcd 和 g_zero

- [ ] https://github.com/gregkh/usbutils : 用户态工具

## serial
https://en.wikibooks.org/wiki/Serial_Programming
- https://www.kernel.org/doc/html/latest/admin-guide/serial-console.html : 似乎可以找到为什么 qemu 运行 linux 在 host 终端里面有消息了
- https://unix.stackexchange.com/questions/60641/linux-difference-between-dev-console-dev-tty-and-dev-tty0 : 对比 /dev/console /dev/tty /dev/ttyS0

- [ ] Documentation/driver-api/serial/driver.rst

- [ ] /home/maritns3/core/linux/drivers/tty/serial/8250/8250_core.c
  - [ ] port 指的是 ?
  - [x] 对比 16550, 找到证据 都是 uart 的一种。(虽然没有对比 16550，但是分析 uart_ops 的注册函数的赋值就可以知道，实际上的 uart 设备比想想的多很多)

uart_add_one_port ==> uart_configure_port

serial8250_request_port ==> serial8250_request_std_resource ==> request_mem_region(`port->membase` 初始化)

- [ ] eldd chapter 6

```c
static const struct uart_ops serial8250_pops = {
	.tx_empty	= serial8250_tx_empty,
	.set_mctrl	= serial8250_set_mctrl,
	.get_mctrl	= serial8250_get_mctrl,
	.stop_tx	= serial8250_stop_tx,
	.start_tx	= serial8250_start_tx,
	.throttle	= serial8250_throttle,
	.unthrottle	= serial8250_unthrottle,
	.stop_rx	= serial8250_stop_rx,
	.enable_ms	= serial8250_enable_ms,
	.break_ctl	= serial8250_break_ctl,
	.startup	= serial8250_startup,
	.shutdown	= serial8250_shutdown,
	.set_termios	= serial8250_set_termios,
	.set_ldisc	= serial8250_set_ldisc,
	.pm		= serial8250_pm,
	.type		= serial8250_type,
	.release_port	= serial8250_release_port,
	.request_port	= serial8256_request_port,
	.config_port	= serial8250_config_port,
	.verify_port	= serial8250_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = serial8250_get_poll_char,
	.poll_put_char = serial8250_put_poll_char,
#endif
};
```
- [ ] 使用 serial8250_stop_tx 为例子:
  - 参数是 uart_port
  - `__stop_tx`
    - `__do_stop_tx`
      - serial8250_clear_THRI : 向8250 的 UART_IER 寄存器写入 UART_IER_THRI (**微机原理与接口**原理真不错)
        - [ ] 从这里来说，port 才是真正在直接控制具体的 8250 芯片，而 8250_core.c 处理是 platform_driver 之类的事情吧！
```c
static inline void serial_out(struct uart_8250_port *up, int offset, int value)
{
	up->port.serial_out(&up->port, offset, value);
}
```



```c
static struct uart_driver serial8250_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serial",
	.dev_name		= "ttyS",
	.major			= TTY_MAJOR,
	.minor			= 64,
	.cons			= SERIAL8250_CONSOLE,
};

static struct console univ8250_console = {
	.name		= "ttyS",
	.write		= univ8250_console_write,
	.device		= uart_console_device,
	.setup		= univ8250_console_setup,
	.exit		= univ8250_console_exit,
	.match		= univ8250_console_match,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME,
	.index		= -1,
	.data		= &serial8250_reg,
};

static struct platform_driver serial8250_isa_driver = {
	.probe		= serial8250_probe,
	.remove		= serial8250_remove,
	.suspend	= serial8250_suspend,
	.resume		= serial8250_resume,
	.driver		= {
		.name	= "serial8250",
	},
};

/*
 * This "device" covers _all_ ISA 8250-compatible serial devices listed
 * in the table in include/asm/serial.h
 */
static struct platform_device *serial8250_isa_devs;
```

```c
static int __init serial8250_init(void)

int uart_register_driver(struct uart_driver *drv)
```

- 在 serial_core.c 中间，几乎所有的函数都是 `uart_*` 的，所以 uart 是 serial 的协议基础:

- [ ] serial8250_isa_devs 和 serial8250_reg 的关系是什么 ?


## i8042

- [ ] https://wiki.osdev.org/%228042%22_PS/2_Controller
  - [ ] 在哪里可以找到 intel 的手册
- [x] i8042 为什么在 /proc/interrupts 下存在两个项 ? (同时有键盘和鼠标)
- [ ] drivers/input/serio/i8042.c
- [ ] drivers/input/keyboard/atkbd.c

/home/maritns3/core/linux/Documentation/driver-api/driver-model/platform.rst


## platform_driver
A platform is a pseudo bus usually used to tie lightweight devices integrated into SoCs, with the Linux device model. A platform consists of
1. A platform device
2. A platform driver


## device model
- [ ]  [A fresh look at the kernel's device model](https://lwn.net/Articles/645810/)




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
register_chrdev_region 之类的函数, 都是出现在 `fs/char_dev.c` 中间，
调用者是各种驱动，比如 tty, 似乎 `fs/char_dev.c` 是一个通用函数的总结，
而 `fs/block_dev.c` 更像是提供了标准访问 block 的接口。

- [ ] 应该对比一下一般的 char dev 和 tty 的区别

- [ ] ldd3 的 chapter 3

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
## pty pts tty
- https://unix.stackexchange.com/questions/21280/difference-between-pts-and-tty/21294
- https://stackoverflow.com/questions/4426280/what-do-pty-and-tty-mean

- tty : teletype. Usually refers to the serial ports of a computer, to which terminals were attached.
- tpy : pseudo tty
- pts : psuedo terminal slave (an xterm or an ssh connection).
> 是的，还是让人非常的疑惑啊!

## 问题
- [x] 到底一共存在多少总线类型 ? PCI PCIE I2C  ()
- [ ] major:minor ?
- [ ] 到底一共存在多少总线类型 ? PCI PCIE I2C


### 需要回答的问题
2. 如果抛弃历史原因，如何设计tty程序
3. tty设备驱动可以实现什么功能
4. tty设备驱动主要包括什么东西 ?
5. tty 和 键盘驱动 以及 是如何沟通联系起来的


### 可能有用的问题
1. 为什么采用fs(everything is file 真的正确吗)的方法，难道真的没有更好的办法吗?
2. 似乎设备驱动的步骤的总是那几个 : malloc init register driver/ register device
3. driver 和 device 的区别是什么，为什么需要进行两次注册 ? 此外mknod 和 insmod 各自实现的功能是什么 ?
4. /dev /sys /proc 这些文件夹和设备驱动的关系是什么?
5. 为什么在interrupt context 不能调用可能导致睡眠的函数?
6. 那些机制实现内核可以动态的装载模块的 ? 除了export_symbols 之类的东西


### 细节问题
1. 到底tty core 对应的代码是什么，函数，文件，变量 ?
2. tty core和driver 如何沟通 ?
3. tty 架构是什么样子的，提供证据
4. port 到底在上下文中间指的是什么东西 ?

### 具体内容
1. 介绍三个结构体
2. 介绍重要函数
3. 介绍整个驱动工作的流程
4. 说明一下有什么特殊的东西的
5. 提供习题以及参考资料

### 各种结构体
1. tty_struct
2. tty_operation
3. tty_driver
4. tty_port
4. termios

1. 三者的联系是什么 ?

似乎初始化的内容就是在其中主要将 driver 和 termios ,op, port 挂钩起来, 其中port 的挂钩似乎含有单独的说明!

而tty_struct 几乎是所有的函数调用的时候参数，但是是谁提供该参数目前并不清楚。

2. 自己定义的 `struct tiny_serial` 的作用是什么
### 习题
1. 如何修改以实现支持kernel 5.1的内核
2. 实现终端模拟器




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
[^4]: [An Introduction with PCI Device Assignment with VFIO](http://events17.linuxfoundation.org/sites/events/files/slides/An%20Introduction%20to%20PCI%20Device%20Assignment%20with%20VFIO%20-%20Williamson%20-%202016-08-30_0.pdf)
