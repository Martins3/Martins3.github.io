# Linux Device Driver : PCI Driver
A bus is made up of both an
electrical interface and a programming interface. 

> 总线，难道不就是一根线吗 ?
> 为什么会变得这么复杂 ?


## 12.1 The PCI Interface
Although many computer users think of PCI as a way of laying out electrical wires, it
is actually **a complete set of specifications defining how different parts of a computer
should interact**.

in this section, we are mainly concerned with how a PCI
driver can find its hardware and gain access to it.

The probing techniques discussed
in the sections “Module Parameters” in Chapter 2 and “Autodetecting the IRQ
Number” in Chapter 10 can be used with PCI devices, but the specification offers an
alternative that is preferable to probing.
> 补齐这些东西

The PCI architecture was designed as a replacement for the ISA standard, with three
main goals: 
1. to get better performance when transferring data between the computer and its peripherals
2. to be as platform independent as possible 
3. to simplify adding and removing peripherals to the system.

**What is most relevant to the driver writer, however, is PCI’s support for autodetection of interface boards.** 
PCI devices are jumperless (unlike most older peripherals)
and are automatically configured at boot time. Then, the device driver must be able
to access configuration information in the device in order to complete initialization.
This happens without the need to perform any probing.

#### 12.1.1 PCI Addressing
**Each PCI peripheral is identified by a bus number, a device number, and a function
number.** The PCI specification permits a single system to host up to 256 buses, but
because 256 buses are not sufficient for many large systems, Linux now supports PCI
domains.

Device drivers written for Linux, though, don’t need to deal with those binary addresses,
because they use a specific data structure, called`pci_dev`, to act on the devices.
> check it `pci_dev`

```
lspci
cat /proc/bus/pci/devices | cut -f1
```

#### 12.1.2 Boot Time
When power is applied to a PCI device, the hardware remains inactive. In other
words, **the device responds only to configuration transactions.**
**At power on, the device has no memory and no I/O ports mapped in the computer’s address space;**
every other device-specific feature, such as interrupt reporting, is disabled as well.

