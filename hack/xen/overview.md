Domain U HVM Guests 利用 Qemu-DM[^1]

A new feature in Xen designed to improve overall performance and reduce the load on
the Dom 0 Guest is PCI Passthru which allows the Domain U Guest to have direct access
to local hardware without using the Domain 0 for hardware access.[^1]

Domain 0 has responsibility for all devices on the system.
Normally, as it discovers PCI devices, it passes those to drivers within the Linux kernel.
In order for a device to be accessed by a guest, the device must instead be assigned to a special domain 0 driver.
This driver is called xen-pciback in pvops kernels, and called pciback in classic kernels.
PV guests access the device via a kernel driver in the guest called xen-pcifront (pcifront in classic xen kernels),
which connects to pciback. HVM guests see the device on the emulated PCI bus presented by QEMU.[^2]

[^1]: http://www-archive.xenproject.org/files/Marketing/HowDoesXenWork.pdf
[^2]: https://wiki.xenproject.org/wiki/Xen_PCI_Passthrough
