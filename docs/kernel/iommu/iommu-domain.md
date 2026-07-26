## iommu domain 基本概念

iommu domain type 一共三种:

drivers/iommu/Kconfig
```txt
choice
	prompt "IOMMU default domain type"
	depends on IOMMU_API
	default IOMMU_DEFAULT_DMA_LAZY if X86 || S390
	default IOMMU_DEFAULT_DMA_STRICT
	help
	  Choose the type of IOMMU domain used to manage DMA API usage by
	  device drivers. The options here typically represent different
	  levels of tradeoff between robustness/security and performance,
	  depending on the IOMMU driver. Not all IOMMUs support all options.
	  This choice can be overridden at boot via the command line, and for
	  some devices also at runtime via sysfs.

	  If unsure, keep the default.
```

对应的内核属性
```txt
iommu.passthrough=1
```

## iommu domain type

```sh
cd /sys/kernel/iommu_groups && for i in ./*;do cat $i/type &&  ls $i/devices ;done
for i in /sys/kernel/iommu_groups/*;do cat $i/type ;done
```

原来这三个 demain 都是可以选的

```txt
[root@nixos:/sys/kernel/iommu_groups]#cd /sys/kernel/iommu_groups && for i in ./*;do cat $i/type &&  ls $i/devices ;done
DMA
0000:00:00.0
DMA
0000:00:01.0
DMA
0000:00:1c.0
DMA
0000:00:1c.2
DMA
0000:00:1d.0
identity
0000:00:1f.0  0000:00:1f.3  0000:00:1f.4  0000:00:1f.5
identity
0000:01:00.0  0000:01:00.1
identity
0000:02:00.0
DMA
0000:03:00.0
identity
0000:05:00.0
identity
0000:06:00.0
DMA
0000:00:06.0
identity
0000:00:0a.0
identity
0000:00:14.0  0000:00:14.2
identity
0000:00:14.3
identity
0000:00:15.0  0000:00:15.1  0000:00:15.2
identity
0000:00:16.0
identity
0000:00:17.0
DMA
0000:00:1a.0
```

那么最后 iommu=pt 的实现在什么地方?
```c
static int __init iommu_set_def_domain_type(char *str)
{
	bool pt;
	int ret;

	ret = kstrtobool(str, &pt);
	if (ret)
		return ret;

	iommu_def_domain_type = pt ? IOMMU_DOMAIN_IDENTITY : IOMMU_DOMAIN_DMA;
	return 0;
}
early_param("iommu.passthrough", iommu_set_def_domain_type);
```


### 那么 domain 是什么意思
`vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义


### 将一个 device attach 到 domain 到底是什么含义?

- iommu_device_register
  - bus_iommu_probe
    - iommu_setup_default_domain
      - iommu_group_alloc_default_domain
      - __iommu_group_set_domain_internal
        - __iommu_device_set_domain
          - __iommu_attach_device



例如，在 __iommu_attach_device 中，传递的就是 device :
```c
static int __iommu_attach_device(struct iommu_domain *domain,
				 struct device *dev)
{
	int ret;

	if (unlikely(domain->ops->attach_dev == NULL))
		return -ENODEV;

	ret = domain->ops->attach_dev(domain, dev);
	if (ret)
		return ret;
	dev->iommu->attach_deferred = 0;
	trace_attach_device_to_domain(dev);
	return 0;
}
```

## DMA-FQ
CONFIG_IOMMU_DEFAULT_DMA_LAZY 详解

原理: 这是「翻译模式 + 延迟刷新」的组合：

```c
  // drivers/iommu/iommu.c
  if (IS_ENABLED(CONFIG_IOMMU_DEFAULT_DMA_LAZY)) {
      iommu_set_default_translated(false);   // iommu_def_domain_type = IOMMU_DOMAIN_DMA
      ...
  }
  if (!iommu_default_passthrough() && !iommu_dma_strict)
      iommu_def_domain_type = IOMMU_DOMAIN_DMA_FQ;
```

IOMMU_DOMAIN_DMA_FQ 的 FQ = Flush Queue。

DMA map 时：
• 通过 iommu_dma_alloc_iova() 分配 IOVA。
• 通过 iommu_map() 建立 IOVA → physical page 的页表映射。

DMA unmap 时（__iommu_dma_unmap()）：
• 先 iommu_unmap_fast() 拆掉页表项。
• 不立即 flush IOTLB，而是把要释放的 IOVA 和页框塞进一个 per-CPU/single 的 ring queue：

```c
  // drivers/iommu/dma-iommu.c
  static void __iommu_dma_unmap(...)
  {
      iommu_iotlb_gather_init(&iotlb_gather);
      iotlb_gather.queued = READ_ONCE(cookie->fq_domain);
      unmapped = iommu_unmap_fast(domain, dma_addr, size, &iotlb_gather);

      if (!iotlb_gather.queued)
          iommu_iotlb_sync(domain, &iotlb_gather);   // strict 模式才同步 flush
      iommu_dma_free_iova(domain, dma_addr, size, &iotlb_gather);
  }

  static void iommu_dma_free_iova(...)
  {
      if (gather && gather->queued)
          queue_iova(domain->iova_cookie, iova_pfn(...), size, &gather->freelist);
      else
          free_iova_fast(...);
  }
```

flush queue 的关键结构：

```c
  struct iova_fq {
      spinlock_t lock;
      unsigned int head, tail;
      unsigned int mod_mask;
      struct iova_fq_entry entries[];
  };

  struct iova_fq_entry {
      unsigned long iova_pfn;
      unsigned long pages;
      struct iommu_pages_list freelist;
      u64 counter;   // 记录入队时的 flush 计数
  };
```

触发真正 flush 的两种情况：
1. 队列满：fq_full() 时直接 fq_flush_iotlb()，然后释放已 flush 的条目。
2. 定时器超时：默认 10 ms / 1000 ms，由 fq_flush_timeout() 定期刷队列。

```c
  static void fq_flush_iotlb(struct iommu_dma_cookie *cookie)
  {
      atomic64_inc(&cookie->fq_flush_start_cnt);
      cookie->fq_domain->ops->flush_iotlb_all(cookie->fq_domain);
      atomic64_inc(&cookie->fq_flush_finish_cnt);
  }
```

释放条目时检查 counter >= fq_flush_finish_cnt，保证只在 TLB flush 完成后才回收 IOVA 和页框。

好处

• 有隔离：设备只能访问 dma_map_*() 建立的 IOVA 范围，防止越界 DMA。
• 性能接近直通：通过批量 flush 和延迟回收，大幅减少昂贵的 IOTLB/TLB invalidation 次数。
• 支持 untrusted 设备：PCIe 外接设备等可被限制在已映射区域内。
• 虚拟化友好：VFIO、设备直通给 VM 通常需要翻译模式。

代价

• 延迟回收窗口：unmap 后的一段时间内（最长 FQ timeout，默认 10 ms），设备 TLB
里可能还留着旧映射。这段时间内如果物理页被重新分配给其他用途，设备仍可能通过 stale TLB 访问。
• CPU/内存开销：需要维护 iova_domain、页表、per-CPU flush queue、定时器。
• 复杂度高：smp barrier、spinlock、timer、flush counter 等协调逻辑比 pt 复杂得多。
• 某些设备不兼容：对 TLB 一致性要求极高或需要同步 unmap 保证安全的设备，lazy 可能出问题（此时可用 iommu.strict=1 切到 strict）。

适用场景

• 服务器默认配置、现代 Linux 发行版（Fedora、RHEL、Ubuntu 等）的默认选择。
• 需要 DMA 隔离但不想承受 strict 模式每次 unmap 都 flush 的性能损失。

## iommu 等于 pt 的时候，iommu 的作用是什么?
<!-- d38f04ed-291f-4187-8c81-06c93b3c11fd -->

这是 intel_iommu=on iommu=pt 的结果:

```txt
-    5.76%     0.72%  fio  [kernel.kallsyms]  [k] nvme_queue_rqs
   - 5.03% nvme_queue_rqs
      - 3.66% nvme_prep_rq
           1.13% nvme_pci_setup_data_simple
           0.91% blk_mq_start_request
         - 0.77% nvme_setup_cmd
              0.52% nvme_setup_rw
      - 1.21% nvme_submit_cmds.part.0
           0.83% _raw_spin_lock
```
这个只能说明在说映射的时候，没有 iommu table 的重建，不能说明没用，
也不能说明当时正在使用一个一一映射的东西。


## type 是不可以随便动态修改的

对正在使用的 group 5（USB xHCI，绑定了 xhci_hcd）执行：

```bash
echo DMA | sudo tee /sys/kernel/iommu_groups/5/type
```

原因是 drivers/iommu/iommu.c:iommu_group_store_type 的检查：

   ```c
     if (list_empty(&group->devices) || group->owner_cnt) {
         ret = -EPERM;
         goto out_unlock;
     }
   ```

   驱动绑定时不允许切换，防止把正在跑 DMA 的设备搞挂。这是合理的设计。

## 高级话题
### struct iommu_group

l1 虚拟机中没有配置 iommu ，但是可以观测到:
```txt
/sys/kernel/iommu_groups/0/devices:
 82a31eeb-094b-4464-9a0e-69c416f76cd4

/sys/kernel/iommu_groups/1/devices:
 bd52f358-1920-48ed-9f85-920b58859259
```

```txt
kernel/iommu_groups/1🔒 🌳
🧀  tree
.
├── devices
│   └── bd52f358-1920-48ed-9f85-920b58859259 -> ../../../../devices/virtual/mdpy/mdpy/bd52f358-1920-48ed-9f85-920b58859259
├── name
├── reserved_regions
└── type
```

检查其中的内容:
```txt
🧀  cat type
unknown
kernel/iommu_groups/1🔒 🌳
🧀  cat name
vfio-noiommu
```

## 几个关键结构体

drivers/iommu/iommu.c

```c
struct iommu_group {
	struct kobject kobj;
	struct kobject *devices_kobj;
	struct list_head devices;
	struct xarray pasid_array;
	struct mutex mutex;
	void *iommu_data;
	void (*iommu_data_release)(void *iommu_data);
	char *name;
	int id;
	struct iommu_domain *default_domain;
	struct iommu_domain *blocking_domain;
	struct iommu_domain *domain;
	struct list_head entry;
	unsigned int owner_cnt;
	/*
	 * Number of devices in the group undergoing or awaiting recovery.
	 * If non-zero, concurrent domain attachments are rejected.
	 */
	unsigned int recovery_cnt;
	void *owner;
};

struct group_device {
	struct list_head list;
	struct device *dev;
	char *name;
	/*
	 * Device is blocked for a pending recovery while its group->domain is
	 * retained. This can happen when:
	 *  - Device is undergoing a reset
	 */
	bool blocked;
	unsigned int reset_depth;
};

struct iommu_domain {
	unsigned type;
	enum iommu_domain_cookie_type cookie_type;
	bool is_iommupt;
	const struct iommu_domain_ops *ops;
	const struct iommu_dirty_ops *dirty_ops;
	const struct iommu_ops *owner; /* Whose domain_alloc we came from */
	unsigned long pgsize_bitmap;	/* Bitmap of page sizes in use */
	struct iommu_domain_geometry geometry;
	int (*iopf_handler)(struct iopf_group *group);

	union { /* cookie */
		struct iommu_dma_cookie *iova_cookie;
		struct iommu_dma_msi_cookie *msi_cookie;
		struct iommufd_hw_pagetable *iommufd_hwpt;
		struct {
			iommu_fault_handler_t handler;
			void *handler_token;
		};
		struct {	/* IOMMU_DOMAIN_SVA */
			struct mm_struct *mm;
			int users;
			/*
			 * Next iommu_domain in mm->iommu_mm->sva-domains list
			 * protected by iommu_sva_lock.
			 */
			struct list_head next;
		};
	};
};
```

## 哦，原来是还存在 iommu group 啊

结论：VFIO group 与 IOMMU group 基本一一对应，但与 IOMMU domain 不是一一对应。

更准确的关系是：

设备 ──属于──> IOMMU group ──整体 attach──> IOMMU domain
                         多个 group 可以共享同一个 domain

- iommu_group：最小 DMA 隔离/所有权单元。
- iommu_domain：DMA 地址空间及页表/映射上下文。
- vfio_group：VFIO 对 iommu_group 的用户态所有权接口。

### 为什么需要 group

根本原因不是“domain 把设备放在一起”，而是硬件无法保证这些设备彼此隔离。

例如：

- 多个 PCI function 存在 DMA alias。
- PCIe bridge 不支持 ACS，允许 P2P DMA 绕过 IOMMU。
- PCIe-to-PCI bridge 后面的设备共享 Requester ID。
- 内核策略认为某个多功能设备必须整体管理。

文档直接定义 group 为“能够与系统其他设备隔离的一组设备”，并明确它是 VFIO 的所有权单位：Documentation/driver-api/vfio.rst:52。

PCI 分组代码也确实根据 DMA alias、PCI 拓扑和 ACS 决定哪些设备必须在一起：drivers/iommu/iommu.c:1598。

因此：

> IOMMU group 描述的是硬件/拓扑允许的最小隔离边界，而 domain 描述的是给设备配置的地址空间。

### VFIO group 和 IOMMU group

传统 VFIO 路径中，两者基本是一一对应的：

1. VFIO 从设备取得 iommu_group。
2. 查找是否已有对应的 vfio_group。
3. 没有就创建一个。

代码见 drivers/vfio/group.c:635。vfio_group 内部直接保存对应的 iommu_group：drivers/vfio/group.c:512。

所以 /dev/vfio/26 中的 26 就是 IOMMU group ID，而不是 domain ID。

### Group 和 domain 的数量关系

对于普通、非 PASID DMA：

- 一个 group 在某一时刻整体位于一个 domain。
- 同一个 group 不能拆开挂到不同 domain。
- 一个 domain 可以挂多个 group。

iommu_group 中保存当前 domain：drivers/iommu/iommu.c:52。切换 domain 时，内核遍历并 attach group 中的所有设备，最后更新 group->domain：drivers/iommu/
iommu.c:2417。

VFIO type1 更直接地证明了“一 domain 多 group”：

- struct vfio_domain 包含 group_list：drivers/vfio/vfio_iommu_type1.c:81。
- 加入新 group 时，VFIO 会尝试把它 attach 到已有兼容 domain：drivers/vfio/vfio_iommu_type1.c:2359。

这意味着：

group A ─┐
group B ─┼──> domain X：共享同一套 IOVA 映射/页表上下文
group C ─┘

它们本来是三个可以独立隔离的 group，只是 VFIO/container 主动让它们共享地址空间。

### 为什么 VFIO 必须以整个 group 为所有权单位

假设设备 A、B 无法彼此隔离，却只把 A 交给虚拟机：

- 虚拟机可以控制 A 发起 DMA。
- A/B 的事务可能无法被 IOMMU 可靠地区分。
- 仍由宿主机控制的 B 和虚拟机控制的 A 就不再具有安全边界。

所以 VFIO 必须要求整个 group 处于同一个 DMA owner 下。传统 container 会调用 iommu_group_claim_dma_owner()：drivers/vfio/container.c:436。

即使现代 VFIO cdev 不再暴露传统 VFIO group/container 接口，仍然保留 IOMMU group 的所有权约束：Documentation/driver-api/vfio.rst:294。

一句话概括：

> VFIO group 的确源自 IOMMU group 的隔离语义；但不是因为 iommu domain 将设备聚合，而是因为这些设备不能被安全拆分。Domain 是随后挂载到这个最小隔离单元上
> 的地址空间。

## 我想要观察 iommu 相关的结果

可以观察 group，但通用 sysfs 只能看到 default_domain 的类型，看不到当前 iommu_domain 的 ID/指针。

### sysfs 看不到什么

type 读取的是：

group->default_domain->type

见 drivers/iommu/iommu.c:978。

它不是：

group->domain->type

因此设备交给 VFIO 后，即使当前已经挂到 VFIO 创建的 unmanaged domain，sysfs 中仍可能显示原来的 DMA default domain。

内核没有通用的：

/sys/kernel/iommu_groups/26/domain

也没有通用 domain ID。domain 是动态内核对象，不属于稳定 sysfs ABI。

### yyds-nv 上的 drgn 实测

yyds-nv 当前 QEMU 参数里有三个 VFIO PCI function ，分别是 nvme 和 nvidia:

```txt
-device vfio-pci,host=0000:02:00.0,rombar=0
-device vfio-pci,host=0000:01:00.0,rombar=0
-device vfio-pci,host=0000:01:00.1,rombar=0
```

它们对应两个 IOMMU group：

```txt
0000:01:00.0 -> /sys/kernel/iommu_groups/16
0000:01:00.1 -> /sys/kernel/iommu_groups/16
0000:02:00.0 -> /sys/kernel/iommu_groups/17
```

这说明 01:00.0 和 01:00.1 是同一个隔离单元，必须作为一个 group 被 VFIO 管理；02:00.0 是另一个隔离单元。

用 drgn 可以直接观察内核对象：

```bash
sudo drgn -c /proc/kcore \
  /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts/vfio_iommu_relationship.py \
  0000:01:00.0 0000:01:00.1 0000:02:00.0
```

关键输出：

```txt
PCI 0000:01:00.0
  pci_dev                  0xffff888104cd9000
  struct device            0xffff888104cd90d0
  driver                   vfio-pci
  driver_data              0xffff888116ca3800
  iommu_group              0xffff8881037dd300 id=16
    owner_cnt              1 owner=0xffff88810f4b6800
    default_domain         0xffffffff83f76680 type=4 cookie_type=0 ops=0xffffffff829cd820 owner=0x0
    active_domain          0xffff888803ed2a00 type=1 cookie_type=0 ops=0xffffffff829cd760 owner=0xffffffff829cd920
    blocking_domain        0xffffffff83f76720 type=0 cookie_type=0 ops=0xffffffff829cd8a0 owner=0x0
    group devices
      0000:01:00.0 group_device=0xffff888104d71c40 dev=0xffff888104cd90d0 blocked=False
      0000:01:00.1 group_device=0xffff888104d71e80 dev=0xffff888104cdd0d0 blocked=False
  vfio_pci_core_device     0xffff888116ca3800
    vfio_device            0xffff888116ca3800
    vfio_device.dev        0xffff888104cd90d0
    vfio_group             0xffff88810f4b6800
    state                  index=0 open_count=1 kvm=0xffff888e937ee000
    iommufd                device=0x0 attached=False cdev_opened=False
    vfio_group detail      iommu_group=0xffff8881037dd300 container=0xffff88810b15c780 container_users=3 type=0 iommufd=0x0

PCI 0000:01:00.1
  pci_dev                  0xffff888104cdd000
  struct device            0xffff888104cdd0d0
  driver                   vfio-pci
  driver_data              0xffff888114dfe800
  iommu_group              0xffff8881037dd300 id=16
    owner_cnt              1 owner=0xffff88810f4b6800
    default_domain         0xffffffff83f76680 type=4 cookie_type=0 ops=0xffffffff829cd820 owner=0x0
    active_domain          0xffff888803ed2a00 type=1 cookie_type=0 ops=0xffffffff829cd760 owner=0xffffffff829cd920
    blocking_domain        0xffffffff83f76720 type=0 cookie_type=0 ops=0xffffffff829cd8a0 owner=0x0
    group devices
      0000:01:00.0 group_device=0xffff888104d71c40 dev=0xffff888104cd90d0 blocked=False
      0000:01:00.1 group_device=0xffff888104d71e80 dev=0xffff888104cdd0d0 blocked=False
  vfio_pci_core_device     0xffff888114dfe800
    vfio_device            0xffff888114dfe800
    vfio_device.dev        0xffff888104cdd0d0
    vfio_group             0xffff88810f4b6800
    state                  index=1 open_count=1 kvm=0xffff888e937ee000
    iommufd                device=0x0 attached=False cdev_opened=False
    vfio_group detail      iommu_group=0xffff8881037dd300 container=0xffff88810b15c780 container_users=3 type=0 iommufd=0x0

PCI 0000:02:00.0
  pci_dev                  0xffff888104cef000
  struct device            0xffff888104cef0d0
  driver                   vfio-pci
  driver_data              0xffff88810f568800
  iommu_group              0xffff8881037dc900 id=17
    owner_cnt              1 owner=0xffff88810d634800
    default_domain         0xffffffff83f76680 type=4 cookie_type=0 ops=0xffffffff829cd820 owner=0x0
    active_domain          0xffff888803ed2a00 type=1 cookie_type=0 ops=0xffffffff829cd760 owner=0xffffffff829cd920
    blocking_domain        0xffffffff83f76720 type=0 cookie_type=0 ops=0xffffffff829cd8a0 owner=0x0
    group devices
      0000:02:00.0 group_device=0xffff888104d71f80 dev=0xffff888104cef0d0 blocked=False
  vfio_pci_core_device     0xffff88810f568800
    vfio_device            0xffff88810f568800
    vfio_device.dev        0xffff888104cef0d0
    vfio_group             0xffff88810d634800
    state                  index=2 open_count=1 kvm=0xffff888e937ee000
    iommufd                device=0x0 attached=False cdev_opened=False
    vfio_group detail      iommu_group=0xffff8881037dc900 container=0xffff88810b15c780 container_users=2 type=0 iommufd=0x0

VFIO container 0xffff88810b15c780
  iommu_driver             0xffff88810b14a080
  iommu_data               0xffff88812b6f69c0
  noiommu                  False
  container groups
    vfio_group=0xffff88810f4b6800 iommu_group=0xffff8881037dd300 id=16 container_users=3 iommufd=0x0
    vfio_group=0xffff88810d634800 iommu_group=0xffff8881037dc900 id=17 container_users=2 iommufd=0x0
  vfio_iommu_type1         0xffff88812b6f69c0
    state                  dma_avail=65522 pgsize_bitmap=0x40201000 num_non_pinned_groups=2 v2=True
    vfio_domain list
      vfio_domain=0xffff888102ce7d80 iommu_domain=0xffff888803ed2a00 type=1 cookie_type=0 ops=0xffffffff829cd760 owner=0xffffffff829cd920 enforce_cache_coherency=True
        vfio_iommu_group=0xffff8898d445d700 iommu_group=0xffff8881037dd300 id=16 pinned_page_dirty_scope=False
        vfio_iommu_group=0xffff8886e210ba40 iommu_group=0xffff8881037dc900 id=17 pinned_page_dirty_scope=False
```

这台机器上的实际关系是：

```txt
0000:01:00.0 ┐
             ├── IOMMU group 16 ── VFIO group 16  ┐
0000:01:00.1 ┘                                    │
                                                  ├── VFIO container 0xffff88810b15c780
0000:02:00.0 ── IOMMU group 17 ── VFIO group 17   ┘
                                                  │
                                                  └── vfio_iommu_type1
                                                       └── vfio_domain
                                                            └── iommu_domain 0xffff888803ed2a00
```

所以这里不是“三个设备三个 domain”，也不是“两个 group 两个 domain”。当前是：

- 两个 IOMMU group：16、17。
- 两个 VFIO group：分别包装 group 16、17。
- 一个传统 VFIO container：`0xffff88810b15c780`。
- 一个 VFIO type1 `vfio_domain`。
- 一个当前 active `iommu_domain`：`0xffff888803ed2a00`，被 group 16 和 group 17 共享。

同时可以看到每个 `vfio_device` 都是传统 VFIO group/container 路径：

```txt
iommufd_device=0x0 iommufd_attached=False cdev_opened=False
```

也就是说，虽然系统里有 `/dev/iommu` 和 `/dev/vfio/devices/vfioN` 这类现代接口节点，这个 QEMU 进程实际没有走 iommufd attach，而是走传统 `/dev/vfio/vfio` container + `/dev/vfio/16`、`/dev/vfio/17` group 路径。

这里的 `default_domain type=4` 是 `IOMMU_DOMAIN_IDENTITY`，来自启动参数 `iommu=pt` 下的默认 identity domain。sysfs 里：

```txt
/sys/kernel/iommu_groups/16/type -> identity
/sys/kernel/iommu_groups/17/type -> identity
```

读到的是 `group->default_domain->type`，所以仍然显示 `identity`。但是设备被 VFIO 接管后，当前正在使用的是：

```txt
group->domain = 0xffff888803ed2a00 type=1
```

也就是 VFIO type1 创建并 attach 的 unmanaged domain。这个对象没有稳定 sysfs ID，只能通过 drgn、crash、tracepoint/kprobe 或 IOMMU debugfs 这类方式观察。

“将 device attach 到 domain”的含义在这个例子里可以具体化为：

1. VFIO 以 IOMMU group 为最小所有权单元接管设备。
2. VFIO type1 为 container 准备一个 `vfio_domain`，其中保存底层 `struct iommu_domain *`。
3. group 16、17 都 attach 到同一个底层 `iommu_domain`。
4. 这个 domain 表示给直通设备使用的 IOVA 地址空间和 IOMMU 页表上下文。
5. QEMU 通过 VFIO DMA map ioctl 把 guest RAM 映射进这个 IOVA 地址空间，两个 group 里的设备 DMA 时共享这一套映射语义。

## debugfs 分析

Intel IOMMU 开启 CONFIG_IOMMU_DEBUGFS 后可以进一步看硬件页表：

sudo mount -t debugfs none /sys/kernel/debug
sudo less /sys/kernel/debug/iommu/intel/dmar_translation_struct
sudo less /sys/kernel/debug/iommu/intel/0000:06:0d.0/domain_translation_struct

对应实现见 drivers/iommu/intel/debugfs.c:751。这是 Intel 专用 debugfs，不是通用接口。

也可以用 bpftrace观察 attach 时的 domain 指针：

```txt
sudo bpftrace -e '
kprobe:iommu_attach_group,
kprobe:iommu_attach_group_handle
{
    $group = (struct iommu_group *)arg1;
    printf("%s domain=%p group=%p id=%d\n",
           probe, arg0, arg1, $group->id);
}'
```

需要内核 BTF 和相应函数可探测。两个 group 最终成功 attach 时出现相同 domain=%p，说明它们共享同一个内核 domain。

### 多个 group 放进同一个 domain

不能通过写 sysfs type 实现。推荐两种用户态接口。

#### 1. 传统 VFIO container

打开一个 container，把多个 group 都加入它：

container = open("/dev/vfio/vfio", O_RDWR);
group1 = open("/dev/vfio/10", O_RDWR);
group2 = open("/dev/vfio/11", O_RDWR);

ioctl(group1, VFIO_GROUP_SET_CONTAINER, &container);
ioctl(group2, VFIO_GROUP_SET_CONTAINER, &container);
ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1v2_IOMMU);

前提是两个 group 中的设备均已解绑宿主驱动并绑定 VFIO。

VFIO type1 会尝试把兼容 group attach 到已有 domain：drivers/vfio/vfio_iommu_type1.c:2359。

但要注意：

- 同一个 container 保证共享相同 IOVA 映射语义。
- 不保证底层始终只有一个 struct iommu_domain。
- 如果 group 使用不同或不兼容的 iommu_ops，VFIO 会保留多个 domain，并向它们同步相同映射。

文档也明确说只有 IOMMU driver 支持时才能共享 IOMMU context：Documentation/driver-api/vfio.rst:99。

#### 2. 现代 iommufd/HWPT

如果要求明确共享同一个底层 domain，iommufd 的 HWPT 接口更直接：

1. 所有设备绑定同一个 /dev/iommu fd。
2. 创建一个 IOAS。
3. 第一个设备 attach 到 IOAS，取得实际 HWPT ID。
4. 其他 group 的设备显式 attach 到这个 HWPT ID。

group 10 device ─┐
                 ├── HWPT ID 42 ── struct iommu_domain
group 11 device ─┘

VFIO_DEVICE_ATTACH_IOMMUFD_PT 的 pt_id 可以传 IOAS ID 或明确的 HWPT ID：include/uapi/linux/vfio.h:943。

如果直接把多个设备 attach 到相同 IOAS，内核会优先复用兼容 HWPT；不兼容时则创建另一个 domain：Documentation/userspace-api/iommufd.rst:210。

如果一个 group 包含多个设备，所有设备都应绑定同一个 iommufd 并分别执行 attach；硬件层面只 attach group 一次，但每个设备的 reserved IOVA 信息都需要单独
处理：drivers/iommu/iommufd/device.c:650。


## 原来 iommu pt 不等于不存在

• 这是预期行为。iommu=pt 下，IOMMU 不参与每一次 dma_map_*()；它只在设备绑定 domain 时被配置为硬件 passthrough。

  调用关系是：

  启动/设备初始化：
  iommu=pt
    → IOMMU_DOMAIN_IDENTITY
    → Intel VT-d context 设置为 PASS_THROUGH

  每次 DMA 映射：
  dma_map_single()
    → CPU VA 转成 PA
    → dma_map_phys()
    → dma_direct_map_phys()
    → 返回设备 DMA 地址（x86 上通常等于 PA）

  关键原因在这里：

  dev->dma_iommu = iommu_is_dma_domain(domain);

  identity domain 不是 DMA translation domain，所以 dev->dma_iommu == false：drivers/iommu/dma-iommu.c:2113。

  随后 dma_map_phys() 选择 direct 路径，而不会调用 iommu_dma_map_phys()：kernel/dma/mapping.c:155。

  if (dma_map_direct(dev, ops))
          addr = dma_direct_map_phys(...);
  else if (use_dma_iommu(dev))
          addr = iommu_dma_map_phys(...);

  dma_direct_map_phys() 最终主要执行：

  dma_addr = phys_to_dma(dev, phys);

  在普通 x86 PCI 系统上通常就是：

  DMA address == host physical address

  但这不表示 VT-d 被关闭。设备挂载 identity domain 时，Intel 驱动已经把 VT-d context 设置为 passthrough：drivers/iommu/intel/iommu.c:3824。

  设备真正发起 DMA 时：

  Device emits DMA address P
          ↓
  VT-d 根据 Requester ID 找到 context
          ↓
  context translation type = PASS_THROUGH
          ↓
  不查 IOMMU 页表，输出地址仍为 P
          ↓
  内存控制器

  因此：

  - 软件映射阶段：不调用 IOMMU map/unmap。
  - 硬件传输阶段：DMA 请求仍经过启用的 VT-d remapping 单元，但地址不转换。
  - 性能上接近没有 IOMMU。
  - 该设备没有按 buffer 建立的 DMA 隔离；dma_unmap_*() 也不会撤销页表权限。
  - IOMMU 仍可服务其他 translation domain、VFIO 设备及 interrupt remapping。

  另外，dma_addr_t 不是 CPU 虚拟地址。dma_map_single() 的输入才是 CPU VA，它先经 virt_to_page()、page_to_phys() 转为物理地址：include/linux/dma-
  mapping.h:509。返回值是设备使用的总线地址，不能作为 CPU 指针解引用。少数平台还可能通过 dma-ranges 加偏移，或者因 DMA mask/SWIOTLB 返回 bounce buffer 地
  址。

## NVMe 4 KiB randread 实测

测试日期：2026-07-07。

测试环境：

- CPU/PCIe 平台：Intel VT-d，启动参数包含 `intel_iommu=on iommu=pt`
- 内核：Fedora `7.0.13-200.fc44.x86_64`
- NVMe：ZHITAI TiPro7000 1TB，`0000:02:00.0`，IOMMU group 17
- fio：3.41

每个 domain type 都执行相同的切换流程，避免驱动初始状态影响对比：

```bash
./collei/scripts/vfio.py unbind 0000:02:00.0
echo "$type" | sudo tee /sys/kernel/iommu_groups/17/type
./collei/scripts/vfio.py default 0000:02:00.0 --driver nvme
```

fio 只读测试参数：

```bash
sudo fio --name=randread --filename=/dev/nvme0n1 --readonly \
    --rw=randread --bs=4k --ioengine=libaio --iodepth=128 \
    --direct=1 --numjobs=1 --time_based=1 --ramp_time=5 \
    --runtime=30 --randrepeat=1 --norandommap=1 --group_reporting=1
```

每种 type 运行 3 次，下表为均值：

| Domain type | IOPS | 带宽 (MiB/s) | 平均 clat (us) | P99 clat (us) | IOPS 相对 identity |
|---|---:|---:|---:|---:|---:|
| `identity` | 435,252 | 1,700.2 | 293.27 | 839.68 | 0.00% |
| `DMA` | 339,646 | 1,326.8 | 374.39 | 831.49 | -21.96% |
| `DMA-FQ` | 433,342 | 1,692.8 | 294.15 | 836.95 | -0.44% |

本机上 `DMA-FQ` 的吞吐量与 `identity` 基本相同，而 strict `DMA` 在该高队列深度负载下下降约 22%。固定 `iodepth=128` 时，平均完成延迟与吞吐量呈反向变化；三种类型的 P99 接近，主要差异体现在持续吞吐量和平均延迟。

首次在开机以来未重绑的 `identity` 状态下仅测得约 135k IOPS；完成一次 unbind/rebind 后稳定为 435k IOPS。该首轮数据与其他 domain type 的前置条件不一致，因此未纳入上表。

显然，但是不要被误导了，那是由于盘的性能极限:
```txt
- 57.56% __do_sys_io_uring_enter
   - 47.07% io_submit_sqes
      - 46.29% io_submit_sqe
         - 44.94% io_issue_sqe
            - 43.56% __io_issue_sqe
               - 43.24% io_read
                  - 42.84% __io_read
                     - 41.61% blkdev_read_iter
                        - 40.63% __blkdev_direct_IO_async
                           - 30.80% submit_bio_noacct_nocheck
                              - 30.01% __submit_bio
                                 - 22.01% __blk_flush_plug
                                    - 21.69% blk_mq_flush_plug_list
                                       - 21.53% blk_mq_dispatch_queue_requests
                                          - 21.28% nvme_queue_rqs
                                             - 19.40% nvme_prep_rq.part.0
                                                - 18.49% nvme_map_data
                                                   - 17.87% dma_map_phys
                                                      + 17.67% iommu_dma_map_phys
                                                  0.81% blk_mq_start_request
                                             - 0.86% nvme_submit_cmds.part.0
                                                  0.59% _raw_spin_lock
                                               0.56% nvme_setup_cmd
                                 + 7.66% blk_mq_submit_bio
                           + 5.51% bio_iov_iter_get_pages
                           + 2.28% bio_alloc_bioset
                             1.14% bio_set_pages_dirty
```

kunpeng 上测试非 pt 结果，大致为:
```txt
- 34.64% submit_bio_noacct
   - 34.21% submit_bio_noacct_nocheck
      - 32.36% __submit_bio
         - 20.59% __blk_flush_plug
            - 20.30% blk_mq_flush_plug_list
               - 19.91% blk_mq_dispatch_queue_requests
                  - 19.41% nvme_queue_rqs
                     - 12.67% nvme_prep_rq
                        - 9.35% nvme_pci_setup_data_simple
                           - 9.13% dma_map_page_attrs
                              - 8.68% iommu_dma_map_page
                                 - 7.95% __iommu_dma_map
                                    - 4.33% iommu_map
                                       - 4.01% iommu_map_nosync
                                          - 3.44% arm_smmu_map_pages
                                             - 3.40% arm_lpae_map_pages
                                                - __arm_lpae_map
                                                   - __arm_lpae_map
                                                      - __arm_lpae_map
                                                         - 1.13% __arm_lpae_map
                                                              arm_lpae_init_pte
                                    - 3.05% iommu_dma_alloc_iova
                                         2.14% _raw_spin_unlock_irqrestore
                          1.33% blk_mq_start_request
                        - 1.00% nvme_setup_cmd
                             nvme_setup_rw
                       2.83% _raw_spin_unlock
                       1.22% nvme_submit_cmds.part.0
                       0.62% nvme_pci_setup_data_simple
                       0.54% __pi_memset_generic
                       0.52% _raw_spin_lock
```


## 最后的总结

1. iommu group 和 iommu domain 的关系
2. iommu pt 的意义是什么
3. 为什么直通之后，就看不到了

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
