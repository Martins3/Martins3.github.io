# sysfs iommu

## mac

```txt
🧀  ls -la /sys/class/iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.22c4a8000.iommu -> ../../devices/platform/soc/22c4a8000.iommu/iommu/apple-dart.22c4a8000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.22c4b4000.iommu -> ../../devices/platform/soc/22c4b4000.iommu/iommu/apple-dart.22c4b4000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.22c4bc000.iommu -> ../../devices/platform/soc/22c4bc000.iommu/iommu/apple-dart.22c4bc000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.24e808000.iommu -> ../../devices/platform/soc/24e808000.iommu/iommu/apple-dart.24e808000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.382f00000.iommu -> ../../devices/platform/soc/382f00000.iommu/iommu/apple-dart.382f00000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.382f80000.iommu -> ../../devices/platform/soc/382f80000.iommu/iommu/apple-dart.382f80000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.502f00000.iommu -> ../../devices/platform/soc/502f00000.iommu/iommu/apple-dart.502f00000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.502f80000.iommu -> ../../devices/platform/soc/502f80000.iommu/iommu/apple-dart.502f80000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.23130c000.iommu -> ../../devices/platform/soc/23130c000.iommu/iommu/apple-dart.23130c000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.228304000.iommu -> ../../devices/platform/soc/228304000.iommu/iommu/apple-dart.228304000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.231304000.iommu -> ../../devices/platform/soc/231304000.iommu/iommu/apple-dart.231304000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.235004000.iommu -> ../../devices/platform/soc/235004000.iommu/iommu/apple-dart.235004000.iommu
lrwxrwxrwx 0 root  9 May 08:00  apple-dart.681008000.iommu -> ../../devices/platform/soc/681008000.iommu/iommu/apple-dart.681008000.iommu
```

```txt
/sys/kernel/iommu_groups
├── 0
│   ├── devices
│   │   └── 228200000.display-pipe -> ../../../../devices/platform/soc/228200000.display-pipe
│   ├── reserved_regions
│   └── type
├── 1
│   ├── devices
│   │   └── 238200000.dma-controller -> ../../../../devices/platform/soc/238200000.dma-controller
│   ├── reserved_regions
│   └── type
├── 2
│   ├── devices
│   │   ├── 24e400000.mtp -> ../../../../devices/platform/soc/24e400000.mtp
│   │   └── 24eb30000.input -> ../../../../devices/platform/soc/24eb14000.fifo/24eb30000.input
│   ├── reserved_regions
│   └── type
├── 3
│   ├── devices
│   │   └── 231c00000.dcp -> ../../../../devices/platform/soc/231c00000.dcp
│   ├── reserved_regions
│   └── type
├── 4
│   ├── devices
│   │   └── soc:display-subsystem -> ../../../../devices/platform/soc/soc:display-subsystem
│   ├── reserved_regions
│   └── type
├── 5
│   ├── devices
│   │   └── 382280000.usb -> ../../../../devices/platform/soc/382280000.usb
│   ├── reserved_regions
│   └── type
├── 6
│   ├── devices
│   │   └── 231c00000.dcp:piodma -> ../../../../devices/platform/soc/231c00000.dcp/231c00000.dcp:piodma
│   ├── reserved_regions
│   └── type
├── 7
│   ├── devices
│   │   └── 502280000.usb -> ../../../../devices/platform/soc/502280000.usb
│   ├── reserved_regions
│   └── type
├── 8
│   ├── devices
│   │   └── 22a000000.isp -> ../../../../devices/platform/soc/22a000000.isp
│   ├── reserved_regions
│   └── type
└── 9
    ├── devices
    │   ├── 0000:01:00.0 -> ../../../../devices/platform/soc/690000000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0
    │   └── 0000:01:00.1 -> ../../../../devices/platform/soc/690000000.pcie/pci0000:00/0000:00:00.0/0000:01:00.1
    ├── reserved_regions
    └── type
```

## virtio

```txt
.
├── 0
│   ├── devices
│   │   └── 0000:00:0b.0 -> ../../../../devices/pci0000:00/0000:00:0b.0
│   ├── reserved_regions
│   └── type
├── 1
│   ├── devices
│   │   └── 0000:00:0c.0 -> ../../../../devices/pci0000:00/0000:00:0c.0
│   ├── reserved_regions
│   └── type
├── 10
│   ├── devices
│   │   └── 0000:00:0a.0 -> ../../../../devices/pci0000:00/0000:00:0a.0
│   ├── reserved_regions
│   └── type
├── 11
│   ├── devices
│   │   └── 0000:00:0e.0 -> ../../../../devices/pci0000:00/0000:00:0e.0
│   ├── reserved_regions
│   └── type
├── 12
│   ├── devices
│   │   └── 0000:00:0f.0 -> ../../../../devices/pci0000:00/0000:00:0f.0
│   ├── reserved_regions
│   └── type
├── 13
│   ├── devices
│   │   └── 0000:00:10.0 -> ../../../../devices/pci0000:00/0000:00:10.0
│   ├── reserved_regions
│   └── type
├── 14
│   ├── devices
│   │   └── 0000:00:11.0 -> ../../../../devices/pci0000:00/0000:00:11.0
│   ├── reserved_regions
│   └── type
├── 15
│   ├── devices
│   │   └── 0000:00:12.0 -> ../../../../devices/pci0000:00/0000:00:12.0
│   ├── reserved_regions
│   └── type
├── 2
│   ├── devices
│   │   └── 0000:00:0d.0 -> ../../../../devices/pci0000:00/0000:00:0d.0
│   ├── reserved_regions
│   └── type
├── 3
│   ├── devices
│   │   └── 0000:00:01.1 -> ../../../../devices/pci0000:00/0000:00:01.1
│   ├── reserved_regions
│   └── type
├── 4
│   ├── devices
│   │   └── 0000:00:03.0 -> ../../../../devices/pci0000:00/0000:00:03.0
│   ├── reserved_regions
│   └── type
├── 5
│   ├── devices
│   │   └── 0000:00:04.0 -> ../../../../devices/pci0000:00/0000:00:04.0
│   ├── reserved_regions
│   └── type
├── 6
│   ├── devices
│   │   └── 0000:00:05.0 -> ../../../../devices/pci0000:00/0000:00:05.0
│   ├── reserved_regions
│   └── type
├── 7
│   ├── devices
│   │   └── 0000:00:06.0 -> ../../../../devices/pci0000:00/0000:00:06.0
│   ├── reserved_regions
│   └── type
├── 8
│   ├── devices
│   │   └── 0000:00:07.0 -> ../../../../devices/pci0000:00/0000:00:07.0
│   ├── reserved_regions
│   └── type
└── 9
    ├── devices
    │   └── 0000:00:09.0 -> ../../../../devices/pci0000:00/0000:00:09.0
    ├── reserved_regions
    └── type

49 directories, 32 files
```


```txt
➜  iommu_groups ls /sys/class/iommu
0000:00:08.0
➜  iommu_groups lspci -s 0000:00:08.0

00:08.0 Unclassified device [00ff]: Virtio: Device 1057 (rev 01)
```

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
