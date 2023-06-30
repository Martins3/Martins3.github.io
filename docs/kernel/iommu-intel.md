# 如何理解 dmar

使用这个参数，可以在虚拟机中观测到:
```sh
	arg_machine="-machine q35,accel=kvm,kernel-irqchip=split"
	arg_machine+=" -device intel-iommu,intremap=on,caching-mode=on"
```

```txt
➜  /sys find . -name "*dmar*"
./kernel/debug/iommu/intel/dmar_perf_latency
./kernel/debug/iommu/intel/dmar_translation_struct
./class/iommu/dmar0
./devices/virtual/iommu/dmar0
```
所有的设备都是在 dmar0 中的


直接可以在 13900K 中可以看到:
```txt
./class/iommu/dmar0
./class/iommu/dmar1
./devices/virtual/iommu/dmar0
./devices/virtual/iommu/dmar1
```

/sys/devices/virtual/iommu/dmar0
```txt
├── devices
│   └── 0000:00:02.0 -> ../../../../pci0000:00/0000:00:02.0
├── intel-iommu
│   ├── address
│   ├── cap
│   ├── domains_supported
│   ├── domains_used
│   ├── ecap
│   └── version
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   └── runtime_usage
├── subsystem -> ../../../../class/iommu
└── uevent
```

/sys/devices/virtual/iommu/dmar1

```txt
├── devices
│   ├── 0000:00:00.0 -> ../../../../pci0000:00/0000:00:00.0
│   ├── 0000:00:01.0 -> ../../../../pci0000:00/0000:00:01.0
│   ├── 0000:00:06.0 -> ../../../../pci0000:00/0000:00:06.0
│   ├── 0000:00:0a.0 -> ../../../../pci0000:00/0000:00:0a.0
│   ├── 0000:00:14.0 -> ../../../../pci0000:00/0000:00:14.0
│   ├── 0000:00:14.2 -> ../../../../pci0000:00/0000:00:14.2
│   ├── 0000:00:14.3 -> ../../../../pci0000:00/0000:00:14.3
│   ├── 0000:00:15.0 -> ../../../../pci0000:00/0000:00:15.0
│   ├── 0000:00:15.1 -> ../../../../pci0000:00/0000:00:15.1
│   ├── 0000:00:15.2 -> ../../../../pci0000:00/0000:00:15.2
│   ├── 0000:00:16.0 -> ../../../../pci0000:00/0000:00:16.0
│   ├── 0000:00:17.0 -> ../../../../pci0000:00/0000:00:17.0
│   ├── 0000:00:1a.0 -> ../../../../pci0000:00/0000:00:1a.0
│   ├── 0000:00:1c.0 -> ../../../../pci0000:00/0000:00:1c.0
│   ├── 0000:00:1c.2 -> ../../../../pci0000:00/0000:00:1c.2
│   ├── 0000:00:1f.0 -> ../../../../pci0000:00/0000:00:1f.0
│   ├── 0000:00:1f.3 -> ../../../../pci0000:00/0000:00:1f.3
│   ├── 0000:00:1f.4 -> ../../../../pci0000:00/0000:00:1f.4
│   ├── 0000:00:1f.5 -> ../../../../pci0000:00/0000:00:1f.5
│   ├── 0000:01:00.0 -> ../../../../pci0000:00/0000:00:01.0/0000:01:00.0
│   ├── 0000:01:00.1 -> ../../../../pci0000:00/0000:00:01.0/0000:01:00.1
│   ├── 0000:02:00.0 -> ../../../../pci0000:00/0000:00:06.0/0000:02:00.0
│   ├── 0000:03:00.0 -> ../../../../pci0000:00/0000:00:1a.0/0000:03:00.0
│   └── 0000:05:00.0 -> ../../../../pci0000:00/0000:00:1c.2/0000:05:00.0
├── intel-iommu
│   ├── address
│   ├── cap
│   ├── domains_supported
│   ├── domains_used
│   ├── ecap
│   └── version
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   └── runtime_usage
├── subsystem -> ../../../../class/iommu
└── uevent
```
奇怪的分配规则，一个只有一个，一个是全部。

## 参考: https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/08/10/iommu-driver-analysis
