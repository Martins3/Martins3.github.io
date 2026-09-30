# Linux Device Driver : TTY Drivers

## 18.1 A Small TTY Driver
`struct tty_driver` it used to register and unregister a tty driver with the tty core.

To create a `struct tty_driver`, the function `alloc_tty_driver` must be called with the
number of tty devices this driver supports as the paramater.
> excuse me, 支持的设备数量 ?

using the `tty_set_operations` function to help copy over the set of function
operations that is defined in the driver.

To register this driver with the tty core, the `struct tty_driver` must be passed to the
`tty_register_driver` function.
> malloc init register 三步走

The flag(`TTY_DRIVER_NO_DEVFS`) may be specified if you want to call `tty_register_device`
only for the devices that actually exist on the system,
so the user always has an up-todate view of the devices present in the kernel, which is what devfs users expect

After registering itself, the driver registers the devices it controls through the `tty_register_device` function.
> 注册driver 和　注册device　有什么区别 ?

#### 18.1.1 struct termios
The `init_termios` variable in the struct `tty_driver` is a `struct termios`. This variable
is used to provide a sane set of line settings *if the port is used before it is initialized by
a user*.
> port 在本上下文中间到底指什么东西


The `struct termios` structure is used to hold all of the current line settings for a specific port on the tty device.
These line settings control the current baud rate, data size, data flow settings, and many other values.

 The `owner` field is necessary in order to prevent the tty driver
from being unloaded while the tty port is open.

The `driver_name` and `name` fields look very similar, yet are used for different purposes.
1. The `driver_name` variable should be set to something short, descriptive, and unique
among all tty drivers in the kernel. This is because it shows up in the `/proc/tty/`
drivers file to describe the driver to the user and in the sysfs tty class directory of tty
drivers currently loaded.
2. The `name` field is used to define a name for the individual tty
nodes assigned to this tty driver in the `/dev` tree. This string is used to create a tty
device by appending the number of the tty device being used at the end of the string.
It is also used to create the device name in the sysfs `/sys/class/tty/` directory. If `devfs` is
enabled in the kernel, this name should include any subdirectory that the tty driver
wants to be placed into.

As an example, the serial driver in the kernel sets the name
field to `tts/` if devfs is enabled and ttyS if it is not. This string is also displayed in the
`/proc/tty/drivers` file.
> 我去，这个例子可不简单啊!

## 18.2 `tty_driver` Function Pointers
Finally, the `tiny_tty` driver declares four function pointers.

#### 18.2. 1 open and close
The open function is `called by the tty core` when a user calls open on the device node
the tty driver is assigned to.
*The tty core calls this with a pointer to the `tty_struct`
structure assigned to this device, and a file pointer. The open field must be set by a tty
driver for it to work properly; otherwise, -ENODEV is returned to the user when open is
called.*
```c
static int tiny_open(struct tty_struct *tty, struct file *file)
```
> 由于采用tty_port 之类的函数，实际上表述的内容有点不同了
> 但是port 到底是什么东西啊 ?

save the data within a static array that can be referenced based on the minor number of the port.
> minor 可以定位port函数?

#### 18.2.2 Flow of Data
It is much easier for this
check to be done in user space than it is for a kernel driver to sit and sleep until all of
the requested data is able to be sent out.
> kernel 态为什么就是需要进行等待的


The `write` function can be called from both interrupt context and user context.
This is important to know, as the tty driver should not call any functions that might sleep
when it is in interrupt context.
> 哇, interrupt context

This can happen if the tty driver does not implement the
put_char function in the tty_struct. In that case, the tty core uses the write function
callback with a data size of 1.
> 接着下面解释了一些莫名奇妙的东西

The `write_room` function is called when the tty core wants to know how much room
in the write buffer the tty driver has available.




#### 18.2.3 Other Buffering Functions
`chars_in_buffer` is called when the tty
core wants to know how many characters are still remaining in the tty driver’s write
buffer to be sent out.

Three functions callbacks in the tty_driver structure can be used to flush any
remaining data that the driver is holding on to.

#### 18.2.4 No read Function?
With only these functions, the `tiny_tty` driver can be registered, a device node
opened, data written to the device, the device node closed, and the driver unregistered and unloaded from the kernel.

no function callback exists to get data from
the driver to the tty core.
> driver 是硬件和tty core 之间的桥梁， tty core 调用这些函数进行对于硬件的操作

Because of the `buffering logic` the tty core provides,
it is not necessary for every tty driver to implement its own buffering logic. The tty core
notifies the tty driver when a user wants the driver to stop and start sending data, but if
the internal tty buffers are full, no such notification occurs.
> tty core 提供缓冲技术

The tty core buffers the data received by the tty drivers in a structure called struct
`tty_flip_buffer`.
> 实际上没有找到这一个结构体

## 18.3 TTY Line Settings
When a user wants to change the line settings of a tty device or retrieve the current
line settings, he makes one of the many different termios user-space library function
calls or directly makes an ioctl call on the tty device node. The tty core converts both
of these interfaces into a number of different tty driver function callbacks and ioctl
calls.


#### 18.3.1 `set_termios`
```c
static void tiny_set_termios(struct uart_port *port,
			     struct ktermios *new, struct ktermios *old)
{
```
> 本section主要是描述这一个函数，主要是如何从ktermios 中间拆解信息出来
> 感觉是 tcsetattr 的支持函数

#### 18.3.2 tiocmget and tiocmset
The `tiocmget` function in the tty driver is called by the tty core when the core wants
to know the current physical values of the control lines of a specific tty device.

The `tiocmset` function in the tty driver is called by the tty core when the core wants to
set the values of the control lines of a specific tty device.
> 无法知道这两个函数设置的变量 和 termios　控制的内容有什么不同啊!

## 18.4 `ioctls`
```c
struct tiny_serial *tiny = tty->driver_data;
```
> Woooo! 哪里定义的tiny_serial这一个变量


`tiny_ioctl_tiocmiwait`:
Be careful when implementing this ioctl, and do not use the interruptible_sleep_on
call, as it is unsafe (there are lots of nasty race conditions involved with it).
Instead, a `wait_queue` should be used to avoid these problems.
> 1. sleep interupt 我的一生之敌
> 2. 为什么wait_queue 的内容也是这么麻烦的


## 18.5 `proc` and `sysfs` Handling of TTY Devices
The tty core provides a very easy way for any tty driver to maintain a file in the `/proc/tty/driver` directory. If the driver defines the `read_proc` or `write_proc` functions, this
file is created. Then, any read or write call on this file is sent to the driver. The formats of these functions are just like the standard `/proc` file-handling functions.

The tty core handles all of the sysfs directory and device creation when the tty
driver is registered, or when the individual tty devices are created, depending on
the `TTY_DRIVER_NO_DEVFS` flag in the struct tty_driver.

The individual directory
always contains the dev file, which allows user-space tools to determine the major
and minor number assigned to the device. It also contains a device and driver `symlink`, if a pointer to a valid struct device is passed in the call to `tty_register_device`.
Other than these *three files*, it is not possible for individual tty drivers to create new sysfs files in this location
> 为什么总是需要将sysfs proc 和 dev 这些文件的关系是什么 ?
> 演示代码中间　使用的都是默认函数
> 运行代码会导致出现/proc/tty/driver 这一个目录无法访问



## 18.6 The `tty_driver` Structure in Detail
The `tty_driver` structure is used to register a tty driver with the tty core.

| Field                                | Description                                                                                                                                                                                            |
|--------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `struct module *owner`               | The module owner for this driver.                                                                                                                                                                      |
| `int magic;`                         | The “magic” value for this structure. Should always be set to TTY_DRIVER_MAGIC. Is initialized in the alloc_tty_driver function.                                                                       |
| `const char *driver_name;`           | Name of the driver, used in /proc/tty and sysfs.                                                                                                                                                       |
| `const char *name;`                  | Node name of the driver.                                                                                                                                                                               |
| `int name_base;`                     | Starting number to use when creating names for devices. This is used when the kernel creates a string representation of a specific tty device assigned to the tty driver.                              |
| `short major;`                       | Major number for the driver.                                                                                                                                                                           |
| `short minor_start;`                 | Starting minor number for the driver. This is usually set to the same value as name_base. Typically, this value is set to 0.                                                                           |
| `short num;`                         | Number of minor numbers assigned to the driver. If an entire major number range is used by the driver, this value should be set to 255. This variable is initialized in the alloc_tty_driver function. |
| `short type;`                        | Describe what kind of tty driver is being registered with the tty core. The value of subtype depends on the type.                                                                                      |
| `short subtype;`                     |                                                                                                                                                                                                        |
| `struct termios init_termios;`       | Initial struct termios values for the device when it is created                                                                                                                                        |
| `int flags;`                         | Driver flags, as described earlier in this chapter.                                                                                                                                                    |
| `struct proc_dir_entry *proc_entry;` | This driver’s /proc entry structure. It is created by the tty core if the driver implements the `write_proc` or `read_proc` functions. This field should not be set by the tty driver itself.              |
| `struct tty_driver *other;`          | Pointer to a tty slave driver. This is used only by the `pty` driver and should not be used by any other tty driver.                                                                                     |
| `void *driver_state;`                | Internal state of the tty driver. Should be used only by the `pty` driver.                                                                                                                               |
| `struct tty_driver *next;`           |                                                                                                                                                                                                        |
| `struct tty_driver *prev;`           | Linking variables. These variables are used by the tty core to chain all of the different tty drivers together, and should not be touched by any tty driver.                                           |

> 莫名奇妙:
> 1. magic 主要做什么的,谁来管理magic 数值的分配的
> 2. pty driver 又是什么东西 ?  参考plki 的第63chapter，tty driver 和pty driver 应该不是两个东西，而是同一个东西放置到一起的 ？
> 3. flags 是做什么的 ? 什么位置讲过的


## 18.7 The `tty_operations` Structure in Detail
The `tty_operations` structure contains all of the function callbacks that can be set by
a tty driver and called by the tty core.
> set by tty driver
> call by tty core

> 似乎所有的操作都是将 将内容输出到硬件的，但是整章中间都是没有看到如何和硬件打交道的内容

```c
void (*throttle)(struct tty_struct * tty);
void (*unthrottle)(struct tty_struct * tty);
void (*stop)(struct tty_struct *tty);
void (*start)(struct tty_struct *tty);
```
Data-throttling functions. These functions are used to help control overruns of
the tty core’s input buffers. The throttle function is called when the tty core’s
input buffers are getting full. The tty driver should try to signal to the device that
no more characters should be sent to it. The unthrottle function is called when
the tty core’s input buffers have been emptied out, and it can now accept more
data. The tty driver should then signal to the device that data can be received.
The stop and start functions are much like the throttle and unthrottle functions,
but they signify that the tty driver should stop sending data to the device and
then later resume sending data.

```c
void (*break_ctl)(struct tty_struct *tty, int state);
```
The line break control function.
> 似乎tlpi 中间有提到过

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
