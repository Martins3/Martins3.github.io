## 虚拟机中为什么会有 rescan 的操作

这个虚拟机在外部是没有观察到热插的，但是当时在内部观察到了很多 pci 结果

echo 1 | sudo tee /sys/bus/pci/rescan
就可以触发一下

相关代码在 pci_setup_cardbus

```txt
Aug 10 21:00:39 localhost kernel: Initializing cgroup subsys cpuset
Dec 12 12:01:39 localhost kdumpctl: kexec: loaded kdump kernel
Dec 12 12:01:39 localhost systemd: Started Crash recovery kernel arming.
Dec 12 12:01:39 localhost systemd: Startup finished in 1.115s (kernel) + 1.127s (initrd) + 11.069s (userspace) = 13.313s.
Dec 12 12:01:40 localhost ntpd[1749]: kernel reports TIME_ERROR: 0x41: Clock Unsynchronized
Dec 12 12:01:40 localhost ntpd[1749]: kernel reports TIME_ERROR: 0x41: Clock Unsynchronized
Dec 12 12:06:25 localhost kernel: pci 0000:00:0b.0: BAR 6: assigned [mem 0xc0000000-0xc003ffff pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:0b.0: BAR 4: assigned [mem 0x42800000000-0x42800003fff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:0b.0: BAR 1: assigned [mem 0xc0040000-0xc0040fff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:0b.0: BAR 0: assigned [io  0x1000-0x103f]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: virtio-pci 0000:00:0b.0: enabling device (0000 -> 0003)
Dec 12 12:06:25 localhost kernel: ACPI: PCI Interrupt Link [LNKC] enabled at IRQ 11
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0: PCI bridge to [bus 01]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [io  0xc000-0xcfff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe800000-0xfe9fffff]
Dec 12 12:06:25 localhost kernel: pci 0000:00:03.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
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
