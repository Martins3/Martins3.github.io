# Linux Device Driver : Communicating with Hardware
The driver is the abstraction layer **between** software concepts and hardware circuitry; as such, it needs to talkwith both of them.


## 9.1 I/O Ports and I/O Memory
**Every** peripheral device is controlled by writing and reading its registers. Most of the
time a device has several registers, and they are accessed at consecutive addresses,
either in the **memory address space** or in the **I/O address space**.
> 1. Every 真的没有其他的例外吗 ? 那么GPU 又是如何解释的 ?

At the hardware level, there is **no conceptual difference** between memory regions and
I/O regions: both of them are accessed by asserting electrical signals on the address
bus and control bus (i.e., the read and write signals) and by reading from or writing
to the data bus.
> what's next to read : 总线驱动

Linux implements the concept of I/O ports on all computer
platforms it runs on, even on platforms where the CPU implements a single address
space.

Even if the peripheral bus has a separate address space for I/O ports, not all devices
map their registers to I/O ports.
> register 和 io port 的关系到底是什么 ?
> register 既可以映射到 io port 上，也可以映射到 io memory 上，不同的设备会采用不同的方法。

While use of I/O ports is common for **ISA** peripheral boards, most **PCI** devices map registers into a memory address region. This I/O
memory approach is generally preferred, because it doesn’t require the use of special purpose processor instructions; CPU cores access memory much more efficiently,
and the compiler has much more freedom in register allocation and addressing-mode
selection when accessing memory
> ISA 是什么 PCI是什么东西 ?

## 9.2 I/O Registers and Conventional Memory
Despite the strong **similarity** between hardware registers and memory,
a programmer accessing I/O registers must be careful to avoid being tricked by CPU (or compiler) optimizations that can modify the expected I/O behavior.

The main difference between I/O registers and RAM is that I/O operations have side
effects, while memory operations have none the no-side-effects case has been optimized in several ways: values are cached and read/write instructions are reordered

## 9.3 Using I/O Ports

#### 9.3.1 I/O Registers and Conventional Memory

```
➜  short git:(52b506e) ✗ sudo cat /proc/ioports
0000-0cf7 : PCI Bus 0000:00
  0000-001f : dma1
  0020-0021 : pic1
  0040-0043 : timer0
  0050-0053 : timer1
  0060-0060 : keyboard
  0064-0064 : keyboard
  0070-0077 : rtc0
  0080-008f : dma page reg
  00a0-00a1 : pic2
  00c0-00df : dma2
  00f0-00ff : fpu
  0378-037f : short
  0400-041f : iTCO_wdt
    0400-041f : iTCO_wdt
  0680-069f : pnp 00:02
0cf8-0cff : PCI conf1
0d00-ffff : PCI Bus 0000:00
  164e-164f : pnp 00:02
  1800-18fe : pnp 00:02
    1800-1803 : ACPI PM1a_EVT_BLK
    1804-1805 : ACPI PM1a_CNT_BLK
    1808-180b : ACPI PM_TMR
    1810-1815 : ACPI CPU throttle
    1830-1833 : iTCO_wdt
      1830-1833 : iTCO_wdt
    1850-1850 : ACPI PM2_CNT_BLK
    1854-1857 : pnp 00:04
    1880-189f : ACPI GPE0_BLK
  2000-20fe : pnp 00:01
  3000-3fff : PCI Bus 0000:01
    3000-307f : 0000:01:00.0
  4000-403f : 0000:00:02.0
  4040-405f : 0000:00:1f.4
    4040-405f : i801_smbus
  ffff-ffff : pnp 00:02
    ffff-ffff : pnp 00:02
      ffff-ffff : pnp 00:02
```


内核中间的代码定义(kernel/resource.c)
```
#define request_region(start,n,name)		__request_region(&ioport_resource, (start), (n), (name), 0)
#define release_region(start,n)	__release_region(&ioport_resource, (start), (n))


/**
 * __request_region - create a new busy resource region
 * @parent: parent resource descriptor
 * @start: resource start address
 * @n: resource region size
 * @name: reserving caller's ID string
 * @flags: IO resource flags
 */
struct resource * __request_region(struct resource *parent,
				   resource_size_t start, resource_size_t n,
				   const char *name, int flags)


/**
 * __release_region - release a previously reserved resource region
 * @parent: parent resource descriptor
 * @start: resource start address
 * @n: resource region size
 *
 * The described resource region must match a currently busy region.
 */
void __release_region(struct resource *parent, resource_size_t start,
			resource_size_t n)
```
#### 9.3.2 Manipulating I/O ports
The Linux kernel headers (specifically,
the architecture-dependent header `<asm/io.h>)`
define the following inline functions to access I/O ports:
`inb`
`inw`
`outw`
...

#### 9.3.3 I/O Port Access from User Space
The sample sources `misc-progs/inp.c` and `misc-progs/outp.c` are a minimal tool for
reading and writing ports from the command line, in user space. They expect to be
installed under multiple names (e.g., inb, inw, and inl and manipulates byte, word, or
long ports depending on which name was invoked by the user). They use `ioperm` or
`iopl` under x86, /dev/port on other platforms.

The programs can be made `setuid` root, if you want to live dangerously and play with
your hardware without acquiring explicit privileges. Please do not install them `setuid` on a production system, however; they are a security hole by design.
> setuid ? excuse me ?

#### 9.3.4 String Operations
> skip, too easy

#### 9.3.5 Pausing I/O
he pausing functions are exactly like
those listed previously, but their names end in `_p`; they are called inb_p, outb_p, and
so on.

#### 9.3.6 Platform Dependencies
I/O instructions are, by their nature, highly processor dependent. Because they work
with the details of how the processor handles moving data in and out, it is very hard
to hide the differences between systems. As a consequence, much of the source code
related to port I/O is platform-dependent.

## 9.4 An I/O Port Example

#### 9.4.1 An Overview of the Parallel Port
并口的介绍

#### 9.4.2 A Sample Driver
> 实际上 使用 dd 读取内容并不正常 需要进一步的分析源代码

 Interested readers may want to look
at the source for the parport and parport_pc modules to see how complicated this
device can get in real life in order to support a range of devices (printers, tape
backup, network interfaces) on the parallel port.

## 9.5 Using I/O Memory
## 总结
1. 想要使用LED等进行演示
2. 实现io port　和 io memory 的底层机制是什么 ? x86 到底是如何实现的 ?
