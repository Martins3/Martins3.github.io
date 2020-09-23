# 10
For the driver developer, the PCI family offers an attractive advantage: a system of automatic device
configuration. Unlike drivers for the older ISA generation, PCI drivers need not implement complex probing
logic. During boot, the BIOS-type boot firmware (or the kernel itself if so configured) walks the PCI bus and
assigns resources such as interrupt levels and I/O base addresses. **The device driver gleans this assignment by
peeking at a memory region called the PCI configuration space.** PCI devices possess 256 bytes of configuration
memory. The top 64 bytes of the configuration space is standardized and holds registers that contain details
such as the status, interrupt line, and I/O base addresses.
> bios 对于设备进行探测，然后将 configuration 的类容放到 configuration region.
> 然后让设备读取。

To operate on a memory region such as the frame buffer on the above PCI video card, follow these steps:
1.  pci_resource_start
2.  request_mem_region (TODO linux kernle labs 中间使用了这个，就是采用写死的方法，根本没有申请!)
3. 


[^1] 解释 request_mem_region 只是预留空间的作用。


[^1]: https://stackoverflow.com/questions/7682422/what-does-request-mem-region-actually-do-and-when-it-is-needed

