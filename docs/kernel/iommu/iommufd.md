# iommufd

## 文档
- https://lpc.events/event/17/contributions/1418/attachments/1297/2607/LPC2023_iommufd.pdf
- https://lpc.events/event/18/contributions/1789/

## 为什么不是继续用 VFIO container，而是引入 iommufd?

先区分两件事:

- VFIO 仍然是设备直通的核心框架。`VFIO_DEVICE_GET_REGION_INFO`、
  `VFIO_DEVICE_SET_IRQS`、`VFIO_DEVICE_RESET` 这类“怎么访问设备”的接口还在 VFIO
  里。
- iommufd 替换的是 VFIO 里负责 IOMMU/DMA 地址空间管理的那层，也就是 legacy
  `vfio_iommu_type1`、`/dev/vfio/vfio` container、group fd、`VFIO_IOMMU_MAP_DMA`
  这一套。

所以更准确的说法不是“VFIO 不用，改用 iommufd”，而是:

> VFIO 继续负责 device ABI，iommufd 负责 I/O page table / IOAS / HWPT / vIOMMU
> 这些 IOMMU 语义。

旧 VFIO container 模型的问题在于，它最初是为了“把一个 IOMMU group 放进一个
container，然后给这个 container 设置 type1 IOMMU，再 map/unmap DMA”设计的。这个模型
能很好覆盖传统 PCI passthrough，但是扩展现代 IOMMU 能力时很别扭:

1. **group/container 是 VFIO 私有抽象，不是通用 IOMMU uAPI**

   `VFIO_GROUP_SET_CONTAINER`、`VFIO_SET_IOMMU`、`VFIO_IOMMU_MAP_DMA` 把设备隔离、
   DMA owner、I/O page table 生命周期都揉进 VFIO 自己的接口里。vDPA、用户态驱动、
   future accelerator driver 如果也要把 DMA 映射交给内核管理，要么重复一套类似
   VFIO type1 的逻辑，要么被迫依赖 VFIO 的概念。

   iommufd 把这部分抽出来，提供 `/dev/iommu`，让任何需要 userspace DMA 的驱动都可以
   绑定设备、创建 IOAS、映射 IOVA、分配 HWPT。VFIO 只是其中一个 consumer。

2. **legacy VFIO 是 group-centric，iommufd 是 device-centric**

   VFIO 传统路径是:

   ```txt
   /dev/vfio/$group -> VFIO_GROUP_SET_CONTAINER -> VFIO_SET_IOMMU
                    -> VFIO_GROUP_GET_DEVICE_FD
   ```

   新路径是:

   ```txt
   /dev/iommu
   /dev/vfio/devices/vfioX -> VFIO_DEVICE_BIND_IOMMUFD
                           -> VFIO_DEVICE_ATTACH_IOMMUFD_PT
   ```

   IOMMU group 的安全约束仍然存在，只是被内核在 bind/attach 阶段处理。这样用户态看到
   的主对象是 device 和它 attach 的 IOAS/HWPT，不需要把 group/container 作为外部 ABI
   的中心。

3. **一个 container 不足以表达现代 IOMMU 对象**

   VFIO type1 基本表达的是“一个 DMA 地址空间 + 一批 iommu_domain”。而 iommufd 明确拆成
   这些对象:

   - `IOAS`: userspace 看到的 IOVA 地址空间，功能上接近 VFIO container。
   - `DEVICE`: 被某个 userspace DMA 子系统绑定并取得 DMA ownership 的设备。
   - `HWPT_PAGING`: 真实硬件页表，也就是 `iommu_domain`，可以自动分配，也可以手工分配。
   - `HWPT_NESTED`: guest 管 stage-1、host 管 stage-2 的 nested translation。
   - `FAULT`: PRI/page request fault 上报与响应队列。
   - `VIOMMU` / `VDEVICE` / `VEVENTQ`: 给 vIOMMU 加速、guest 可见 IOMMU 事件和设备虚拟 ID
     准备的对象。

   这些不是简单给 `VFIO_IOMMU_MAP_DMA` 加几个 flag 就能自然表达的东西。尤其是 nested
   translation、PASID/SSID、PRI、vIOMMU 事件队列，本质上都需要让 userspace 管理多个硬件
   页表对象及其关系。

4. **复用和计费更清晰**

   iommufd 的 `IOAS` 可以被多个子系统共享，例如 VFIO 和 vDPA 绑定到同一个 iommufd 后共享
   同一个地址空间。内核侧的 `io_pagetable` / `iopt_pages` 还可以避免同一批页被多次 pin、
   多次 accounting。旧 VFIO type1 的这套逻辑嵌在 VFIO 内部，难以让其他子系统复用。

5. **QEMU 的模型也更贴近“地址空间后端”**

   QEMU 里 legacy VFIO backend 和 iommufd backend 都挂在 `VFIOAddressSpace` 下面，memory
   listener 仍然负责把 guest RAM 的 add/del 变成 DMA map/unmap。区别只是后端调用:

   - legacy VFIO: 对 container fd 做 `VFIO_IOMMU_MAP_DMA`。
   - iommufd: 对 `/dev/iommu` 做 `IOMMU_IOAS_MAP`，对 device fd 做
     `VFIO_DEVICE_BIND_IOMMUFD` / `VFIO_DEVICE_ATTACH_IOMMUFD_PT`。

   所以 iommufd 对 QEMU 来说不是推翻 VFIO device 模型，而是换掉 VFIO IOMMU backend。
   一些新功能也已经只走 iommufd，例如 QEMU 文档里 `intel_iommu,x-flts=on` 要求 VFIO 设备
   使用 iommufd backend。

6. **迁移路径是兼容旧应用，但新能力走原生 iommufd**

   内核提供 `CONFIG_IOMMUFD_VFIO_CONTAINER` 和 `/dev/vfio/vfio` 到 `/dev/iommu` 的兼容思路，
   让旧 VFIO container ioctl 可以映射到 iommufd 的 `io_pagetable` 操作。但内核文档也说明，
   兼容层并不等价覆盖所有 legacy VFIO type1 能力，也不覆盖 `VFIO_SPAPR_TCE_IOMMU`。

   因此长期方向不是“老应用无感 symlink 一下就完事”，而是 VFIO 用户逐步迁移到 device cdev
   和 native iommufd 接口。新功能优先围绕 `IOAS/HWPT/VIOMMU` 这些对象设计。

一句话总结:

> VFIO container/type1 把 IOMMU 管理绑死在 VFIO group 模型里；iommufd 把 IOMMU 页表管理
> 变成独立、设备中心、可复用、可表达 nested/PASID/PRI/vIOMMU 的通用 uAPI。VFIO 仍然管设备，
> 但不再适合作为所有 userspace DMA/IOMMU 能力的承载层。

## viommu 可以用起来吗?

##  https://docs.kernel.org/userspace-api/iommufd.html

## kvm forum : IOMMUFD Integration in QEMU
https://www.youtube.com/watch?v=PlEzLywexHE

操作 device 的接口:

- VFIO_DEVICE_GET_REGION_INFO
- VFIO_DEVICE_GET_INFO
- VFIO_DEVICE_GET_IRQ_INFO
- VFIO_DEVICE_SET_IRQS
- VFIO_DEVICE_RESET

操作 group 的接口:
- VFIO_GROUP_SET_CONTAINER
- VFIO_GROUP_GET_STATUS
- VFIO_GROUP_GET_DEVICE_FD

container 的接口:
- VFIO_SET_IOMMU
- VFIO_IOMMU_GET_INFO
- VFIO_IOMMU_MAP_DMA
- VFIO_CHECK_EXTENSION
- VFIO_GET_API_VERSION

相关会议可以看看:
- LPC 17 : vSVM IOMMU extension Highlevel component break down and Tech challenge
- vIOMMU implementation using hardware nested paging
- PASID Management in KVM

- IOMMU_IOAS_MAP
- IOMMU_IOAS_COPY
- IOMMU_IOAS_UNMAP
- IOMMU_IOAS_ALLOC : 创建一个 ioas
- IOMMU_IOAS_IOVA_RANGES
- IOMMU_IOAS_ALLOW_IOVAS
- IOMMU_IOAS_COPY : 让映射在不同的 ioas 中拷贝

```c
static const struct iommufd_ioctl_op iommufd_ioctl_ops[] = {
	IOCTL_OP(IOMMU_DESTROY, iommufd_destroy, struct iommu_destroy, id),
	IOCTL_OP(IOMMU_IOAS_ALLOC, iommufd_ioas_alloc_ioctl, struct iommu_ioas_alloc, out_ioas_id),
	IOCTL_OP(IOMMU_IOAS_ALLOW_IOVAS, iommufd_ioas_allow_iovas, struct iommu_ioas_allow_iovas, allowed_iovas),
	IOCTL_OP(IOMMU_IOAS_COPY, iommufd_ioas_copy, struct iommu_ioas_copy, src_iova),
	IOCTL_OP(IOMMU_IOAS_IOVA_RANGES, iommufd_ioas_iova_ranges, struct iommu_ioas_iova_ranges, out_iova_alignment),
	IOCTL_OP(IOMMU_IOAS_MAP, iommufd_ioas_map, struct iommu_ioas_map, iova),
	IOCTL_OP(IOMMU_IOAS_UNMAP, iommufd_ioas_unmap, struct iommu_ioas_unmap, length),
	IOCTL_OP(IOMMU_OPTION, iommufd_option, struct iommu_option, val64),
	IOCTL_OP(IOMMU_VFIO_IOAS, iommufd_vfio_ioas, struct iommu_vfio_ioas, __reserved),
};
```

## 简要的代码分析

- vfio_iommufd_physical_attach_ioas
  - iommufd_device_attach
    - iommufd_device_auto_get_domain
      - iommufd_hw_pagetable_alloc
        - iopt_table_add_domain
          - iopt_fill_domain

## 用 drgn 观察 QEMU/VFIO/iommufd 的实际关系

本地环境里没有 `yyfs-nv`，实际使用的是 `yyds-nv`。这个 VM 的配置中已经启用了
`opt/iommufd`，QEMU 启动参数里可以看到:

```txt
-object iommufd,id=iommufd0
-device vfio-pci,host=0000:02:00.0,rombar=0,iommufd=iommufd0
-object iommufd,id=iommufd1
-device vfio-pci,host=0000:01:00.0,rombar=0,iommufd=iommufd1
-device vfio-pci,host=0000:01:00.1,rombar=0,iommufd=iommufd1
```

host 上 QEMU 进程也确实打开了 iommufd 和 VFIO cdev:

```txt
/proc/$qemu_pid/fd/68 -> /dev/iommu
/proc/$qemu_pid/fd/69 -> /dev/vfio/devices/vfio2
/proc/$qemu_pid/fd/77 -> /dev/iommu
/proc/$qemu_pid/fd/78 -> /dev/vfio/devices/vfio0
/proc/$qemu_pid/fd/87 -> /dev/vfio/devices/vfio1
```

可以使用下面的 drgn 脚本观察核心对象关系:

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -k ./iommufd_relationship.py --pid $(cat ~/data/hack/vm/yyds-nv/s/pid)
```

这次观察到两个独立的 iommufd ctx:

```txt
fd 68: /dev/iommu
  DEVICE(0000:02:00.0)
    -> iommufd_group(group 17)
    -> pasid_attach[NO_PASID]
    -> HWPT_PAGING(id 3)
    -> iommu_domain(cookie_type=IOMMUFD)
    -> IOAS(id 2)

fd 77: /dev/iommu
  DEVICE(0000:01:00.0)
  DEVICE(0000:01:00.1)
    -> same iommufd_group(group 16)
    -> same pasid_attach[NO_PASID]
    -> same HWPT_PAGING(id 3)
    -> same iommu_domain(cookie_type=IOMMUFD)
    -> same IOAS(id 2)
```

这里有几个结论:

1. **一个 `/dev/iommu` fd 对应一个 `struct iommufd_ctx`**

   QEMU 为 `iommufd0` 和 `iommufd1` 各打开了一个 `/dev/iommu`，因此内核中有两个
   `iommufd_ctx`。每个 ctx 维护自己的 `objects` xarray 和 `groups` xarray。

2. **一个 VFIO cdev bind 后会在 iommufd 里出现 `IOMMUFD_OBJ_DEVICE`**

   `/dev/vfio/devices/vfioX` 仍然是 VFIO 的设备访问 fd，但执行
   `VFIO_DEVICE_BIND_IOMMUFD` 后，iommufd 里会分配 `struct iommufd_device`。这个对象连接:

   ```txt
   iommufd_device
     -> struct device
     -> iommufd_group
     -> iommu_group
   ```

3. **attach IOAS 后会得到 `IOAS + HWPT_PAGING + iommu_domain`**

   QEMU 创建 IOAS，再把 device attach 到 IOAS。内核路径会创建或复用一个
   `struct iommufd_hwpt_paging`，里面包着真正的 `struct iommu_domain`:

   ```txt
   iommufd_ioas
     -> io_pagetable
     -> domains xarray
     -> iommu_domain
     -> iommufd_hwpt_paging
   ```

   同时 `iommufd_ioas.hwpt_list` 也会反向列出挂在这个 IOAS 上的 HWPT。

4. **同一个 IOMMU group 内的多个 device 共享同一个 attach/HWPT**

   GPU 的 `0000:01:00.0` 和 `0000:01:00.1` 在同一个 IOMMU group 16 里。drgn 看到二者在同一个
   `iommufd_ctx` 中分别是两个 `IOMMUFD_OBJ_DEVICE`，但是共享:

   ```txt
   iommufd_group(group 16)
     -> pasid_attach[NO_PASID]
     -> iommufd_attach.hwpt
     -> HWPT_PAGING(id 3)
   ```

   这说明 iommufd 的 uAPI 是 device-centric，但 IOMMU group 的安全语义仍然存在。用户态逐个
   bind/attach device，内核在 group 层保证同一 group 的 DMA owner 和 HWPT 关系一致。

5. **`pasid_attach[NO_PASID]` 是普通 RID DMA 路径**

   当前 passthrough 没有使用 PASID，所以 attach 存在 `igroup->pasid_attach[NO_PASID]` 下。
   后续如果使用 PASID/PRI/SVA，这个 xarray 才会出现其他 PASID index，对应更细粒度的地址
   空间绑定。

把这次运行时对象关系压缩成一张图:

```txt
QEMU fd(/dev/iommu)
  -> iommufd_ctx
     -> objects:
        DEVICE  -> iommufd_group -> iommu_group
        IOAS    -> io_pagetable  -> domains[] -> iommu_domain
        HWPT    -> iommu_domain  -> iommufd_hwpt
     -> groups:
        group_id -> iommufd_group
                    -> pasid_attach[NO_PASID]
                       -> iommufd_attach.hwpt
                       -> device_array[] -> iommufd_device
```

因此前面“VFIO 管设备，iommufd 管 IOMMU 页表”的说法可以在运行时对象上对应起来:

- VFIO cdev fd 让 QEMU 拿到设备访问能力。
- `/dev/iommu` fd 让 QEMU 创建 IOAS/HWPT，并把 VFIO device attach 到这个地址空间。
- `iommu_domain.cookie_type=IOMMUFD` 和 `domain->iommufd_hwpt` 说明这个 domain 已经不再是
  legacy VFIO type1 管出来的 domain，而是由 iommufd 的 HWPT 对象承载。

## https://www.phoronix.com/news/IOMMUFD-Linux-6.2

> Further, we have advanced PCI features like Process Address Space ID (PASID) and Page Request Interface (PRI)
> that rely on the IOMMU HW to implement them.
> In particular PASID & PRI are used to create something called Shared Virtual Addressing (SVA or SVM)
> where DMA from a device can be directly delivered to a process virtual memory address by having the IOMMU HW
> directly walk the CPU's page table for the process, and trigger faults for DMA to non-present pages.

## 代码的简单阅读

| Files          | Lines | Code | Comments | Blanks | 主要内容 |
|----------------|-------|------|----------|--------|----------|
| pages.c        | 1991  | 1435 | 321      | 235    |
| io_pagetable.c | 1216  | 903  | 146      | 167    |
| selftest.c     | 1006  | 810  | 58       | 138    |
| device.c       | 721   | 445  | 188      | 88     |
| vfio_compat.c  | 539   | 385  | 91       | 63     |
| main.c         | 463   | 338  | 74       | 51     |
| ioas.c         | 398   | 322  | 18       | 58     |
| hw_pagetable.c | 105   | 64   | 23       | 18     |

## Documentation/userspace-api/iommufd.rst

```c
static struct miscdevice iommu_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "iommu",
	.fops = &iommufd_fops,
	.nodename = "iommu",
	.mode = 0660,
};


static struct miscdevice vfio_misc_dev = {
	.minor = VFIO_MINOR,
	.name = "vfio",
	.fops = &iommufd_fops,
	.nodename = "vfio/vfio",
	.mode = 0666,
};
```

字符设备的目录这个显示应该有点问题吧，都是 10,196 ?
```txt
🧀  ls -la /dev/vfio/vfio
crw-rw-rw- 10,196 root 16 6月  22:55  /dev/vfio/vfio
vn on  master [!+]
🧀  ls -la /dev/vfio
crw-rw-rw- 10,196 root 16 6月  22:55  vfio
```

## qemu 的文档: docs/devel/vfio-iommufd.rst

主要是介绍 memory region 相关的

为什么 vfio 需要注册 memory listerner ，似乎启动的时候，在不断的 map 和 remap iommu 的 table

## 现在没有配置也是这样的吗?
```txt
/dev/vfio
├── 13
├── 14
├── devices
│   ├── vfio0
│   └── vfio1
└── vfio
```

不知道为什么，物理机中没有这个现象:

```txt
🧀  tree /dev/vfio
/dev/vfio
├── 16
└── vfio

1 directory, 2 files
```

进一步导致如下错误:
```txt
qemu-system-x86_64: -device vfio-pci,host=0000:01:00.0,iommufd=iommufd0: vfio 0000:01:00.0: vfio /sys/bus/pci/devices/0000:01:00.0/vfio-dev: failed to load "/sys/bus/pci/devices/0000:01:00.0/vfio-dev/vfio0/dev"
```

## 什么是 vfio-ap 和 vfio-ccw ？
``vfio-ap`` and ``vfio-ccw`` devices don't have same issue as their backend
devices are always mdev and RAM discarding is force enabled.


## 先到虚拟机中测试下，使用 intel 的接口

继续吧
```txt
@[
    iopt_table_add_domain+5
    iommufd_hwpt_paging_alloc+543
    iommufd_hwpt_alloc+318
    iommufd_fops_ioctl+399
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## iommufd 可以满足一个设备可以切分为多个 domain 使用吗?

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
