# How vhost-user came into being: Virtio-networking and DPDK
- url : https://www.redhat.com/en/blog/how-vhost-user-came-being-virtio-networking-and-dpdk

The vhost-net/virtio-net architecture provides a working solution which has been widely deployed over the years.
> 难道说 vhost-net 和 virtio-net 配合使用的吗?

In order to address the performance issues we will introduce the vhost-user/virtio-pmd architecture.

To understand the details we will review
- the data plane development kit (DPDK),
- how OVS can connect to DPDK (OVS-DPDK)
- and how does virtio fit into the story both on the backend side and the frontend side.

In practice DPDK offers a series of poll mode drivers (PMDs) that enable direct transfer of packets between user space
and the physical interfaces which bypass the kernel network stack all together.

- [ ] 它说上一篇文章分析过 OVS 的

- [ ] 需要看看 vhost-net 的实现是如何操作的?

# A journey to the vhost-users realm
- url: https://www.redhat.com/en/blog/journey-vhost-users-realm

The main difference between the vhost-user library and the vhost-net kernel driver is the communication channel. While the vhost-net kernel driver implements this channel using ioctls, the vhost-user library defines the structure of messages that are sent over a unix socket.

When a device that is being emulated in QEMU attempts to DMA to the guest’s virtio I/O space it will use the vIOMMU TLB to look up the page mapping and perform a secured DMA access.
Question is what happens if the actual DMA was being offloaded to an external process such as a DPDK application using vhost-user library?
- 为什么这回成为一个 Question 啊?

When the vhost-user library tries to directly access the shared memory, it has to translate all the addresses (I/O virtual addresses) into its own memory. It does that by asking QEMU’s vIOMMU for the translation through the Device TLB API.

- [ ] 原来，的确是可以存在 vIOMMU 的，当 Guest 需要使用 VFIO 的时候。

- [ ] 这篇内容很长的

# Deep dive into Virtio-networking and vhost-net
- url : https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net

The vhost API is a message based protocol that allows the hypervisor to offload the data plane to another component (handler) that performs data forwarding more efficiently.

- [ ]  another component

Using this protocol, the master sends the following configuration information to the handler:
- The hypervisor’s memory layout.
- A pair of file descriptors that are used for the handler to send and receive the notifications defined in the virtio spec.

After this process, the hypervisor will no longer process packets (read or write to/from the virtqueues).
- [ ] 这两段话，喵喵喵

A tap device is still used to communicate the VM with the host but now the worker thread handles the I/O events i.e. it polls for driver notifications or tap events, and forwards data.

# Achieving network wirespeed in an open standard manner: introducing vDPA
- url : https://www.redhat.com/en/blog/achieving-network-wirespeed-open-standard-manner-introducing-vdpa

In the past few posts we have discussed the existing virtio-networking architecture both the kernel based (vhost-net/virtio-net) and the userspace/DPDK based (vhost-user/virtio-pmd).

In the vhost-net/virtio-net and vhost-user/virto-pmd architectures we had a software switch (OVS or other) which could take a single NIC on a phy-facing port and distribute it to several VMs with a VM-facing port per VM.

Let’s see how SR-IOV can be mapped to the guest kernel, userspace DPDK or directly to the host kernel:

- [ ] 为什么当时分析 VFIO 的时候，完全没有看到 SR-IOV 啊
- [ ] 找到关于 SR-IOV 让 Host 内核认为自己又多个 NIC 的代码的位置，但是

Virtio full HW offloading

* 使用 vDPA 将 NIC 驱动差别屏蔽。

- [ ] 为什么可以使用 virtio data path 直接访问物理设备啊

# Deep dive into Virtio-networking and vhost-net
- url : https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net

---> 未完待续。

# Achieving network wirespeed in an open standard manner: introducing vDPA
- url : https://www.redhat.com/en/blog/achieving-network-wirespeed-open-standard-manner-introducing-vdpa

# Introduction to vDPA kernel framework
- url : https://www.redhat.com/en/blog/introduction-vdpa-kernel-framework

A "vDPA device" means a type of device whose datapath complies with the virtio specification, but whose control path is vendor specific.

---> 未完待续。
# vDPA kernel framework part 1: vDPA bus for abstracting hardware
- url : https://www.redhat.com/en/blog/vdpa-kernel-framework-part-1-vdpa-bus-abstracting-hardware

---> 未开始。

# vDPA kernel framework part 2: vDPA bus drivers for kernel subsystem interactions
- url : https://www.redhat.com/en/blog/vdpa-kernel-framework-part-2-vdpa-bus-drivers-kernel-subsystem-interactions

---> 未开始。

# Introducing VDUSE: a software-defined datapath for virtio
- url : https://www.redhat.com/en/blog/introducing-vduse-software-defined-datapath-virtio

vDPA was originally developed to help implement virtio (an open standard control and dataplane) in dedicated hardware (such as smartNICs).
