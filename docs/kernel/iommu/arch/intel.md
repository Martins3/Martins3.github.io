# intel

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

## svm

![Shared Virtual Memory in KVM](https://events19.linuxfoundation.cn/wp-content/uploads/2017/11/Shared-Virtual-Memory-in-KVM_Yi-Liu.pdf)

https://archive.fosdem.org/2016/schedule/event/intel_svm/attachments/slides/1269/export/events/attachments/intel_svm/slides/1269/FOSDEM_2016___SVM_on_Intel_Graphics.pdf

## 比想象的还要复杂啊
https://docs.kernel.org/next/x86/sva.html

https://www.zhihu.com/people/yun-zhong-18-19/posts

## 几个模式到时候可以配合 qemu 测试一下
```c
#define sm_supported(iommu)	(intel_iommu_sm && ecap_smts((iommu)->ecap))
#define pasid_supported(iommu)	(sm_supported(iommu) &&			\
				 ecap_pasid((iommu)->ecap))
#define ssads_supported(iommu) (sm_supported(iommu) &&                 \
				ecap_slads((iommu)->ecap))
#define nested_supported(iommu)	(sm_supported(iommu) &&			\
				 ecap_nest((iommu)->ecap))
```

sm_supported : 原来是 scale mode 啊

## [ ] struct device_domain_info 是做什么的?

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
