## 相关链接
- https://www.youtube.com/watch?v=NT-325hgXjY

## 显然这个是需要的
https://github.com/danobi/vmtest

而 virtme-ng 似乎用的是 virtiofs 来启动的。

用 vmtest 测试了一下，发现 kernel 启动总是需要 1s 左右，那些号称毫秒启动的虚拟机，
计算的是那一段时间，到底什么是性能瓶颈。

1. 如何使用 rootfs 的功能?

## 为什么启动这么慢

最开始的时候，需要 3s 才可以才系统中。
```txt
[    3.009282] Run /init as init process
```

将 arg_storage 和 usb 去掉，并且调整 smp 和 ram 的大小
```txt
[    1.421856] Run /tmp/martins3/init.sh as init process
```

作为对比:
```txt
[    1.249944] Run /tmp/vmtest-initWdUnl.sh as init process
```

然后，似乎 serial 也不可以瞎配置?
```txt
[    0.042431] printk: legacy console [ttyS0] enabled
[    0.305660] mempolicy: Enabling automatic NUMA balancing. Configure with numa_balancing= or the kernel.numa_balancing sysctl
[    0.307555] ACPI: Core revision 20240322
```

## 标准参考
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

qemu-system-x86_64 \
	-nodefaults \
	-display none \
	-serial mon:stdio \
	-enable-kvm \
	-cpu host \
	-qmp unix:/tmp/qmp-359046.sock,server=on,wait=off \
	-chardev socket,path=/tmp/qga-765727.sock,server=on,wait=off,id=qga0 \
	-device virtio-serial \
	-device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
	-device virtio-serial \
	-chardev socket,path=/tmp/cmdout-438442.sock,server=on,wait=off,id=cmdout \
	-device virtserialport,chardev=cmdout,name=org.qemu.virtio_serial.0 \
	-virtfs local,id=root,path=/,mount_tag=/dev/root,security_model=none,multidevs=remap \
	-kernel /home/martins3/data/linux-build/arch/x86/boot/bzImage \
	-no-reboot \
	-append "rootfstype=9p rootflags=trans=virtio,cache=mmap,msize=1048576 rw earlyprintk=serial,0,115200 printk.devkmsg=on console=0,115200 loglevel=7 raid=noautodetect init=/tmp/vmtest-initEfT0y.sh panic=-1" \
	-virtfs local,id=shared,path=/home/martins3/core/vn,mount_tag=vmtest-shared,security_model=none,multidevs=remap \
	-smp 2 \
	-m 4G
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
