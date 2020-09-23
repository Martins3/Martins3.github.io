# Linux Device Driver : TTY Drivers
A tty device gets its name from the very old abbreviation of teletypewriter and was
originally associated only with the physical or virtual terminal connection to a Unix
machine.
> 历史

Over time, the name also came to mean any serial port style device, as terminal connections could also be created over such a connection.
> 实际上的演化

The Linux tty driver core lives right below the standard character driver level and
provides a range of features focused on providing an interface for *terminal style*
devices to use.

The **core** is responsible for controlling both the flow of data across a
tty device and the format of the data.

As Figure 18-1 shows, the tty core takes data from a user that is to be sent to a tty
device. It then passes it to a tty line discipline driver, which then passes it to the tty
driver.
The tty driver converts the data into a format that can be sent to the hardware.
> 这个描述和blog 中间的有点不同啊. 多出来了一个tty core 的内容


The tty driver never sees the tty line discipline. The driver cannot communicate
directly with the line discipline, nor does it realize it is even present. The driver’s job
is to format data that is sent to it in a manner that the hardware can understand, and
receive data from the hardware.
> tty driver 和 hardware 打交道

There are three different types of tty drivers: console, serial port, and pty.
> 三种tty 驱动!

The console and pty drivers have already been written and probably are the only ones needed
of these types of tty drivers. This leaves any new drivers using the tty core to interact
with the user and the system as serial port drivers.

To determine what kind of tty drivers are currently loaded in the kernel and what tty
devices are currently present, look at the /proc/tty/drivers file. This file consists of a
list of the different tty drivers currently present, showing the name of the driver, the
default node name, the major number for the driver, the range of minors used by the
driver, and the type of the tty driver.

```
➜  Vn git:(master) ✗ cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
rfcomm               /dev/rfcomm   216 0-255 serial
usbserial            /dev/ttyUSB   188 0-511 serial
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
unknown              /dev/tty        4 1-63 console
```
The default serial driver creates a file in this directory that shows a lot of serial-port-specific information about the hardware.
*Information on how to create a file in this directory is described later.*
> /proc/tty/driver 下面还有很多内容

All of the tty devices currently registered and present in the kernel have their own
subdirectory under /sys/class/tty.

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

> skip
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


## 18.8 The `tty_struct` Structure in Detail
The `tty_struct` variable is used by the tty core to keep the current state of a specific
tty port. Almost all of its fields are to be used only by the tty core, with a few exceptions.
> 1. 再一次，什么是tty port
> 2. `tty_struct` 和 `tty_driver` 的不同的位置




## What's the fucking tty ?
Early user terminals connected to computers were electromechanical teleprinters or teletypewriters
(TeleTYpewriter, TTY), and since then TTY has continued to be used as the name for the text-only console although
now this text-only console is a virtual console not a physical console.

In essence, tty is short for teletype, but it's more popularly **known as terminal**.
It's basically a device (implemented in software nowadays) that allows you to interact with the system by passing on the data (you input) to the system,
and displaying the output produced by the system.

ttys can be of different types.
For example, graphical consoles that you can access with the Ctrl+Alt+Fn key combination,
or terminal *emulators* like Gnome terminal that run inside an X session. To learn more about tty
> 为什么称之为 emulator 而不是 就是terminal

#### [The TTY demystified](http://www.linusakesson.net/programming/tty/index.php)
In present time, we find ourselves in a world where physical teletypes and video terminals are practically extinct. Unless you visit a museum or a hardware enthusiast, all the TTYs you're likely to see will be emulated video terminals — software simulations of the real thing.

But as we shall see, the legacy from the old cast-iron beasts is still lurking beneath the surface.
> 优秀的比喻句

Incidentally, the kernel provides several different line disciplines.
Only one of them is attached to a given serial device at a time. The default discipline, which provides line editing, is called N_TTY (drivers/char/n_tty.c, if you're feeling adventurous). Other disciplines are used for other purposes, such as managing packet switched data (ppp, IrDA, serial mice), but that is outside the scope of this article.
> 多种line编辑模式, 默认支持line editing

Session management. The user probably wants to run several programs simultaneously, and interact with them one at a time. If a program goes into an endless loop, the user may want to kill it or suspend it. Programs that are started in the background should be able to execute until they try to write to the terminal, at which point they should be suspended. Likewise, user input should be directed to the foreground program only. The operating system implements these features in the TTY driver (drivers/char/tty_io.c).
> 又一份源代码可以阅读

The TTY driver is not alive; in object oriented terminology, the TTY driver is a passive object. It has some data fields and some methods, but the only way it can actually do something is when one of its methods gets called from the context of a process or a kernel interrupt handler. The line discipline is likewise a passive entity.

Together, a particular triplet of UART driver, line discipline instance and TTY driver may be referred to as a TTY device, or sometimes just TTY. A user process can affect the behaviour of any TTY device by manipulating the corresponding device file under /dev.

从原来的tty 演化到如今的结果:
![](http://www.linusakesson.net/programming/tty/case1.png)
![](http://www.linusakesson.net/programming/tty/case2.png)
![](http://www.linusakesson.net/programming/tty/case3.png)


To facilitate moving the terminal emulation into userland, while still keeping the TTY subsystem (session management and line discipline) intact, the pseudo terminal or pty was invented.

If you look inside the kernel source code,
you will find that any kernel code which is *waiting for an event* must check if a *signal is pending* after *schedule()* returns,
and abort the syscall in that case.
> excuse me ?

Internal shell commands like jobs, fg and bg can be used to manipulate the existing jobs within a session. Each session is managed by a session leader, the shell, which is cooperating tightly with the kernel using a complex protocol of signals and system calls.
> 所以job session process 之间是什么关系啊?

> 后面还有关于signal 的内容，但是一个测试


#### [](https://www.ibm.com/support/knowledgecenter/en/ssw_aix_71/com.ibm.aix.networkcomm/asynch_tty_hungport.htm)
Determine whether the tty is currently handling any processes by typing the following
```sh
ps -lt tty0
```


Determine if any process is attempting to use the tty by typing the following:
```sh
ps -ef | grep tty0
```

#### [](https://www.tldp.org/HOWTO/Serial-HOWTO-10.html)

#### [](https://itstillworks.com/tty-port-6887781.html)
Over the years, the meaning of the term "TTY port" has evolved from describing a *physical connection* on a computer to describing a *virtual connection*.
*Along the way, the term has always referred to the communication between a computer and a remote user*

The connections to the remote users from a computer were provided by teletypewriters and the term TTY originally evolved as an acronym of sorts to refer to these devices.
> 开始的时候用于远程连接，现在泛指这一类设备

TTY devices were originally connected by serial ports to the main computer

#### [](https://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them)

#### [](http://www.tldp.org/HOWTO/Text-Terminal-HOWTO-7.html)
/dev/tty stands for the controlling terminal (if any) for the current process.
To find out which tty's are attached to which processes use the "ps -a" command at the shell prompt (command line).


`/dev/tty` is something like a link to the actually terminal device name with some additional features for C-programmers: see the manual page tty(4).
#### [](https://www.tldp.org/HOWTO/Serial-HOWTO-10.html)

#### [](https://en.wikipedia.org/wiki/Baud)

#### [](https://en.wikipedia.org/wiki/Pseudoterminal)


## 问题
1. tty 和 键盘驱动含有什么关系啊 ? 上面的图已经说明的非常的清楚了。
2. 如何实现一个自己的最基本的终端 ?
3. 难道普通的驱动开发者真的需要了解tty 编程吗 ? 难道哪一个东西 只有一份吗 ?
